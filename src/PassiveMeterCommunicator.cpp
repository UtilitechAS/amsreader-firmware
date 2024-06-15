/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "PassiveMeterCommunicator.h"
#include "IEC6205675.h"
#include "IEC6205621.h"
#include "LNG.h"
#include "LNG2.h"

#if defined(ESP32)
#include <driver/uart.h>
#endif

#if defined(AMS_REMOTE_DEBUG)
PassiveMeterCommunicator::PassiveMeterCommunicator(RemoteDebug* debugger) {
    this->debugger = debugger;
}
#else
PassiveMeterCommunicator::PassiveMeterCommunicator(Stream* debugger) {
    this->debugger = debugger;
}
#endif

void PassiveMeterCommunicator::configure(MeterConfig& meterConfig, Timezone* tz) {
    this->meterConfig = meterConfig;
	if(meterConfig.baud == 0) {
		autodetect = true;
	}
    this->configChanged = false;
    this->tz = tz;
    setupHanPort(meterConfig.baud, meterConfig.parity, meterConfig.invert);
    if(gcmParser != NULL) {
        delete gcmParser;
        gcmParser = NULL;
    }
}

bool PassiveMeterCommunicator::loop() {
	if(hanBufferSize == 0) return false;

    unsigned long now = millis();
    if(autodetect) handleAutodetect(now);

	unsigned long start, end;
	if(!hanSerial->available()) {
		return false;
	}

	// Before reading, empty serial buffer to increase chance of getting first byte of a data transfer
	if(!serialInit) {
		hanSerial->readBytes(hanBuffer, hanBufferSize);
		serialInit = true;
		return false;
	}

	dataAvailable = false;
	ctx = {0,0,0,0};
	memset(ctx.system_title, 0, 8);
    pos = DATA_PARSE_INCOMPLETE;
	// For each byte received, check if we have a complete frame we can handle
	start = millis();
	while(hanSerial->available() && pos == DATA_PARSE_INCOMPLETE) {
		// If buffer was overflowed, reset
		if(len >= hanBufferSize) {
			hanSerial->readBytes(hanBuffer, hanBufferSize);
			len = 0;
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Buffer overflow, resetting\n"));
			return false;
		}
		hanBuffer[len++] = hanSerial->read();
		ctx.length = len;
		pos = unwrapData((uint8_t *) hanBuffer, ctx);
		if(ctx.type > 0 && pos >= 0) {
			switch(ctx.type) {
				case DATA_TAG_DLMS:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid DLMS at %d\n"), pos);
					break;
				case DATA_TAG_DSMR:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid DSMR at %d\n"), pos);
					break;
				case DATA_TAG_SNRM:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid SNMR at %d\n"), pos);
					break;
				case DATA_TAG_AARE:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid AARE at %d\n"), pos);
					break;
				case DATA_TAG_RES:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid Get Response at %d\n"), pos);
					break;
				case DATA_TAG_HDLC:
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Received valid HDLC at %d\n"), pos);
					break;
				default:
					// TODO: Move this so that payload is sent to MQTT
					#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Unknown tag %02X at pos %d\n"), ctx.type, pos);
					len = 0;
					return false;
			}
		}
		yield();
	}
	end = millis();
	if(end-start > 1000) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Used %dms to unwrap HAN data\n"), end-start);
	}

	if(pos == DATA_PARSE_INCOMPLETE) {
		return false;
	} else if(pos == DATA_PARSE_UNKNOWN_DATA) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Unknown data received\n"));
        lastError = pos;
		len = len + hanSerial->readBytes(hanBuffer+len, hanBufferSize-len);
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
{
			debugger->printf_P(PSTR("  payload:\n"));
			debugPrint(hanBuffer, 0, len);
		}
		len = 0;
		return false;
	}
	if(pos == DATA_PARSE_INTERMEDIATE_SEGMENT) {
		len = 0;
		return false;
	} else if(pos < 0) {
        lastError = pos;
		printHanReadError(pos);
		len += hanSerial->readBytes(hanBuffer+len, hanBufferSize-len);
        if(pt != NULL) {
            pt->publishBytes(hanBuffer, len);
        }
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
{
			debugger->printf_P(PSTR("  payload:\n"));
			debugPrint(hanBuffer, 0, len);
		}
		while(hanSerial->available()) hanSerial->read(); // Make sure it is all empty, in case we overflowed buffer above
		len = 0;
		return false;
	}

	if(ctx.type == 0) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Ended up with context type %d, return code %d and length: %lu/%lu\n"), ctx.type, pos, ctx.length, len);
        lastError = pos;
		len = len + hanSerial->readBytes(hanBuffer+len, hanBufferSize-len);
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
{
			debugger->printf_P(PSTR("  payload:\n"));
			debugPrint(hanBuffer, 0, len);
		}
		len = 0;
		return false;
	}

	// Data is valid, clear the rest of the buffer to avoid tainted parsing
	for(int i = pos+ctx.length; i<hanBufferSize; i++) {
		hanBuffer[i] = 0x00;
	}
    dataAvailable = true;
	lastError = DATA_PARSE_OK;

    return true;
}

AmsData* PassiveMeterCommunicator::getData(AmsData& meterState) {
    if(!dataAvailable) return NULL;
	if(ctx.length > hanBufferSize) {
        debugger->printf_P(PSTR("Invalid context length\n"));
		dataAvailable = false;
		return NULL;
	}
    
    AmsData* data = NULL;
	char* payload = ((char *) (hanBuffer)) + pos;
	if(maxDetectedPayloadSize < pos) maxDetectedPayloadSize = pos;
	if(ctx.type == DATA_TAG_DLMS) {
        if(pt != NULL) {
            pt->publishBytes((uint8_t*) payload, ctx.length);
        }

		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("Using application data:\n"));
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugPrint((byte*) payload, 0, ctx.length);

		// Rudimentary detector for L&G proprietary format, this is terrible code... Fix later
		if(payload[0] == CosemTypeStructure && payload[2] == CosemTypeArray && payload[1] == payload[3]) {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("LNG\n"));
			LNG lngData = LNG(meterState, payload, meterState.getMeterType(), &meterConfig, ctx);
			if(lngData.getListType() >= 1) {
				data = new AmsData();
				data->apply(meterState);
				data->apply(lngData);
			}
		} else if(payload[0] == CosemTypeStructure && 
			payload[2] == CosemTypeLongUnsigned && 
			payload[5] == CosemTypeLongUnsigned && 
			payload[8] == CosemTypeLongUnsigned && 
			payload[11] == CosemTypeLongUnsigned && 
			payload[14] == CosemTypeLongUnsigned && 
			payload[17] == CosemTypeLongUnsigned
		) {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("LNG2\n"));
			LNG2 lngData = LNG2(meterState, payload, meterState.getMeterType(), &meterConfig, ctx);
			if(lngData.getListType() >= 1) {
				data = new AmsData();
				data->apply(meterState);
				data->apply(lngData);
			}
		} else {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("DLMS\n"));
			// TODO: Split IEC6205675 into DataParserKaifa and DataParserObis. This way we can add other means of parsing, for those other proprietary formats
			data = new IEC6205675(payload, meterState.getMeterType(), &meterConfig, ctx, meterState);
		}
	} else if(ctx.type == DATA_TAG_DSMR) {
		data = new IEC6205621(payload, tz, &meterConfig);
	}
	len = 0;
    if(data != NULL) {
        if(data->getListType() > 0) {
            validDataReceived = true;
            if(rxBufferErrors > 0) rxBufferErrors--;
        }
    }
	dataAvailable = false;
    return data;
}

int PassiveMeterCommunicator::getLastError() {
	#if defined ESP8266
	if(hwSerial != NULL) {
		if(hwSerial->hasRxError()) {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Serial RX error\n"));
			lastError = 96;
		}
		if(hwSerial->hasOverrun()) {
			rxerr(2);
		}
	} else if(swSerial != NULL) {
		if(swSerial->overflow()) {
			rxerr(2);
		}
	}
	#endif
    return lastError;
}

bool PassiveMeterCommunicator::isConfigChanged() {
    return configChanged;
}

void PassiveMeterCommunicator::getCurrentConfig(MeterConfig& meterConfig) {
    meterConfig = this->meterConfig;
}


int16_t PassiveMeterCommunicator::unwrapData(uint8_t *buf, DataParserContext &context) {
	int16_t ret = 0;
	bool doRet = false;
	uint16_t end = hanBufferSize;
	uint8_t tag = (*buf);
	uint8_t lastTag = DATA_TAG_NONE;
	while(tag != DATA_TAG_NONE) {
		int16_t curLen = context.length;
		int8_t res = 0;
		switch(tag) {
			case DATA_TAG_HDLC:
				if(hdlcParser == NULL) hdlcParser = new HDLCParser();
				res = hdlcParser->parse(buf, context);
				if(context.length < 3) doRet = true;
				break;
			case DATA_TAG_MBUS:
				if(mbusParser == NULL) mbusParser =  new MBUSParser();
				res = mbusParser->parse(buf, context);
				break;
			case DATA_TAG_GBT:
				if(gbtParser == NULL) gbtParser = new GBTParser();
				res = gbtParser->parse(buf, context);
				break;
			case DATA_TAG_GCM:
				if(gcmParser == NULL) gcmParser = new GCMParser(meterConfig.encryptionKey, meterConfig.authenticationKey);
				res = gcmParser->parse(buf, context);
				break;
			case DATA_TAG_LLC:
				if(llcParser == NULL) llcParser = new LLCParser();
				res = llcParser->parse(buf, context);
				break;
			case DATA_TAG_DLMS:
				if(dlmsParser == NULL) dlmsParser = new DLMSParser();
				res = dlmsParser->parse(buf, context);
				if(res >= 0) doRet = true;
				break;
			case DATA_TAG_DSMR:
				if(dsmrParser == NULL) dsmrParser = new DSMRParser();
				res = dsmrParser->parse(buf, context, lastTag != DATA_TAG_NONE);
				if(res >= 0) doRet = true;
				break;
			case DATA_TAG_SNRM:
			case DATA_TAG_AARE:
			case DATA_TAG_RES:
				res = DATA_PARSE_OK;
				doRet = true;
				break;
			default:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Ended up in default case while unwrapping...(tag %02X)\n"), tag);
				return DATA_PARSE_UNKNOWN_DATA;
		}
		lastTag = tag;
		if(res == DATA_PARSE_INCOMPLETE) {
			return res;
		}
		if(context.length > end) {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("Context length %lu > %lu:\n"), context.length, end);
			context.type = 0;
			context.length = 0;
			return false;
		}
        switch(tag) {
            case DATA_TAG_HDLC:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("HDLC frame:\n"));
                if(pt != NULL) {
                    pt->publishBytes(buf, curLen);
                }
                break;
            case DATA_TAG_MBUS:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("MBUS frame:\n"));
                if(pt != NULL) {
                    pt->publishBytes(buf, curLen);
                }
                break;
            case DATA_TAG_GBT:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("GBT frame:\n"));
                break;
            case DATA_TAG_GCM:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("GCM frame:\n"));
                break;
            case DATA_TAG_LLC:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("LLC frame:\n"));
                break;
            case DATA_TAG_DLMS:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("DLMS frame:\n"));
                break;
            case DATA_TAG_DSMR:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("DSMR frame:\n"));
                if(pt != NULL) {
                    pt->publishString((char*) buf);
                }
                break;
			case DATA_TAG_SNRM:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("SNMR frame:\n"));
                break;
			case DATA_TAG_AARE:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("AARE frame:\n"));
                break;
			case DATA_TAG_RES:
                #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugger->printf_P(PSTR("RES frame:\n"));
                break;
        }
        #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::VERBOSE))
#endif
debugPrint(buf, 0, curLen);
		if(res == DATA_PARSE_FINAL_SEGMENT) {
			if(tag == DATA_TAG_MBUS) {
				res = mbusParser->write(buf, context);
			}
		}

		if(res < 0) {
			return res;
		}
		buf += res;
		end -= res;
		ret += res;

		// If we are ready to return, do that
		if(doRet) {
			context.type = tag;
			return ret;
		}

		// Use start byte of new buffer position as tag for next round in loop
		tag = (*buf);
	}
	#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Got to end of unwrap method...\n"));
	return DATA_PARSE_UNKNOWN_DATA;
}

void PassiveMeterCommunicator::debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			debugger->print(F("0"));
		debugger->print(buffer[i], HEX);
		debugger->print(F(" "));
		if ((i - start + 1) % 16 == 0)
			debugger->println(F(""));
		else if ((i - start + 1) % 4 == 0)
			debugger->print(F(" "));

		yield(); // Let other get some resources too
	}
	debugger->println(F(""));
}

void PassiveMeterCommunicator::printHanReadError(int pos) {
	#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
{
		switch(pos) {
			case DATA_PARSE_BOUNDRY_FLAG_MISSING:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Boundry flag missing\n"));
				break;
			case DATA_PARSE_HEADER_CHECKSUM_ERROR:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Header checksum error\n"));
				break;
			case DATA_PARSE_FOOTER_CHECKSUM_ERROR:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Frame checksum error\n"));
				break;
			case DATA_PARSE_INCOMPLETE:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Received frame is incomplete\n"));
				break;
			case GCM_AUTH_FAILED:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Decrypt authentication failed\n"));
				break;
			case GCM_ENCRYPTION_KEY_FAILED:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Setting decryption key failed\n"));
				break;
			case GCM_DECRYPT_FAILED:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Decryption failed\n"));
				break;
			case MBUS_FRAME_LENGTH_NOT_EQUAL:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Frame length mismatch\n"));
				break;
			case DATA_PARSE_INTERMEDIATE_SEGMENT:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Intermediate segment received\n"));
				break;
			case DATA_PARSE_UNKNOWN_DATA:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Unknown data format %02X\n"), hanBuffer[0]);
				break;
			default:
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Unspecified error while reading data: %d\n"), pos);
		}
	}
}

void PassiveMeterCommunicator::setupHanPort(uint32_t baud, uint8_t parityOrdinal, bool invert, bool passive) {
	int8_t rxpin = meterConfig.rxPin;
	int8_t txpin = passive ? -1 : meterConfig.txPin;

	if(baud == 0) {
		baud = 2400;
	}

	#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("(setupHanPort) Setting up HAN on pin %d/%d with baud %d and parity %d\n"), rxpin, txpin, baud, parityOrdinal);

	if(parityOrdinal == 0) {
		parityOrdinal = 3; // 8N1
	}

	if(rxpin == 3 || rxpin == 113) {
		#if ARDUINO_USB_CDC_ON_BOOT
			hwSerial = &Serial0;
		#else
			hwSerial = &Serial;
		#endif
	}

	uint8_t uart_num = 0;
	#if defined(ESP32)
		hwSerial = &Serial1;
		uart_num = UART_NUM_1;
		#if defined(CONFIG_IDF_TARGET_ESP32)
			if(rxpin == 16) {
				hwSerial = &Serial2;
				uart_num = UART_NUM_2;
			}
		#endif
	#endif

	if(rxpin == 0) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Invalid GPIO configured for HAN\n"));
		return;
	}

	if(meterConfig.bufferSize < 1) meterConfig.bufferSize = 1;
	if(meterConfig.bufferSize > 64) meterConfig.bufferSize = 64;

	if(hwSerial != NULL) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Hardware serial\n"));
		Serial.flush();
		#if defined(ESP8266)
			SerialConfig serialConfig;
		#elif defined(ESP32)
			uint32_t serialConfig;
		#endif
		switch(parityOrdinal) {
			case 2:
				serialConfig = SERIAL_7N1;
				break;
			case 3:
				serialConfig = SERIAL_8N1;
				break;
			case 7:
				serialConfig = SERIAL_8N2;
				break;
			case 10:
				serialConfig = SERIAL_7E1;
				break;
			default:
				serialConfig = SERIAL_8E1;
				break;
		}
		if(meterConfig.bufferSize < 4) meterConfig.bufferSize = 4; // 64 bytes (1) is default for software serial, 256 bytes (4) for hardware

		hwSerial->setRxBufferSize(64 * meterConfig.bufferSize);
		#if defined(ESP32)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
			uart_set_pin(uart_num, txpin, rxpin, -1, -1);
		#else
			hwSerial->begin(baud, serialConfig, SERIAL_FULL, 1, invert);
		#endif
		
		#if defined(ESP8266)
			if(rxpin == 3) {
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Switching UART0 to pin 1 & 3\n"));
				Serial.pins(1,3);
			} else if(rxpin == 113) {
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Switching UART0 to pin 15 & 13\n"));
				Serial.pins(15,13);
			}
		#endif

 		// Prevent pullup on TX pin if not uart0
		#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
			if(txpin != 17) pinMode(17, INPUT);
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
			if(txpin != 7) pinMode(7, INPUT);
		#elif defined(ESP32)
			if(rxpin == 9 && txpin != 10) {
				pinMode(10, INPUT);
			} else if(rxpin == 16 && txpin != 17) {
				pinMode(17, INPUT);
			}
		#elif defined(ESP8266)
			if(rxpin == 113) {
				pinMode(15, INPUT);
			}
		#endif

		hanSerial = hwSerial;
		#if defined(ESP8266)
		if(swSerial != NULL) {
			swSerial->end();
			delete swSerial;
			swSerial = NULL;
		}
		#endif
	} else {
		#if defined(ESP8266)
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Software serial\n"));
			Serial.flush();
			
			if(swSerial == NULL) {
				swSerial = new SoftwareSerial(rxpin, txpin, invert);
			} else {
				swSerial->end();
			}

			SoftwareSerialConfig serialConfig;
			switch(parityOrdinal) {
				case 2:
					serialConfig = SWSERIAL_7N1;
					break;
				case 3:
					serialConfig = SWSERIAL_8N1;
					break;
				case 7:
					serialConfig = SWSERIAL_8N2;
					break;
				case 10:
					serialConfig = SWSERIAL_7E1;
					break;
				default:
					serialConfig = SWSERIAL_8E1;
					break;
			}

			uint8_t bufferSize = meterConfig.bufferSize;
			#if defined(ESP8266)
			if(bufferSize > 2) bufferSize = 2;
			#endif
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Using serial buffer size %d\n"), 64 * bufferSize);
			swSerial->begin(baud, serialConfig, rxpin, txpin, invert, meterConfig.bufferSize * 64);
			hanSerial = swSerial;

			Serial.end();
			Serial.begin(115200);
			hwSerial = NULL;
		#else
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Software serial not available\n"));
			return;
		#endif
	}

	if(hanBuffer != NULL) {
		free(hanBuffer);
	}
	hanBufferSize = max(64 * meterConfig.bufferSize * 2, 512);
	hanBuffer = (uint8_t*) malloc(hanBufferSize);

	// The library automatically sets the pullup in Serial.begin()
	if(!meterConfig.rxPinPullup) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("HAN pin pullup disabled\n"));
		pinMode(meterConfig.rxPin, INPUT);
	}

	if(meterConfig.txPin != 0xFF && passive) {
		pinMode(meterConfig.txPin, OUTPUT);
		digitalWrite(meterConfig.txPin, LOW);
	}

	hanSerial->setTimeout(250);

	// Empty buffer before starting
	while (hanSerial->available() > 0) {
		hanSerial->read();
	}
	#if defined(ESP8266)
	if(hwSerial != NULL) {
		hwSerial->hasOverrun();
	} else if(swSerial != NULL) {
		swSerial->overflow();
	}
	#endif
}

HardwareSerial* PassiveMeterCommunicator::getHwSerial() {
    return hwSerial;
}

void PassiveMeterCommunicator::rxerr(int err) {
	if(err == 0) return;
	switch(err) {
		case 2:
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Serial buffer overflow\n"));
			rxBufferErrors++;
			if(rxBufferErrors > 1 && meterConfig.bufferSize < 8) {
				meterConfig.bufferSize += 2;
				#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Increasing RX buffer to %d bytes\n"), meterConfig.bufferSize * 64);
                configChanged = true;
				rxBufferErrors = 0;
			}
			break;
		case 3:
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::ERROR))
#endif
debugger->printf_P(PSTR("Serial FIFO overflow\n"));
			break;
		case 4:
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Serial frame error\n"));
			break;
		case 5:
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Serial parity error\n"));
		    unsigned long now = millis();
			if(now - meterAutodetectLastChange < 120000) {
				switch(autodetectParity) {
					case 2: // 7N1
						autodetectParity = 10;
						break;
					case 10: // 7E1
						autodetectParity = 6;
						break;
					case 6: // 7N2
						autodetectParity = 14;
						break;
					case 14: // 7E2
						autodetectParity = 2;
						break;

					case 3: // 8N1
						autodetectParity = 11;
						break;
					case 11: // 8E1
						autodetectParity = 7;
						break;
					case 7: // 8N2
						autodetectParity = 15;
						break;
					case 15: // 8E2
						autodetectParity = 3;
						break;

					default:
						autodetectParity = 3;
						break;
				}
				if(validDataReceived) {
					meterConfig.parity = autodetectParity;
					configChanged = true;
					setupHanPort(meterConfig.baud, meterConfig.parity, meterConfig.invert);
				}
			}
			break;
	}
	// Do not include serial break
	if(err > 1) lastError = 90+err;
}

void PassiveMeterCommunicator::handleAutodetect(unsigned long now) {
    if(!autodetect) return;

	if(!validDataReceived) {
		if(now - meterAutodetectLastChange > 20000 && (meterConfig.baud == 0 || meterConfig.parity == 0)) {
			autodetect = true;
			if(autodetectCount == 2)  {
				autodetectInvert = !autodetectInvert;
				autodetectCount = 0;
			}
			autodetectBaud = AUTO_BAUD_RATES[autodetectCount++];
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Meter serial autodetect, swapping to: %d, %d, %s\n"), autodetectBaud, autodetectParity, autodetectInvert ? "true" : "false");
			meterConfig.bufferSize = max((uint32_t) 1, autodetectBaud / 14400);
			setupHanPort(autodetectBaud, autodetectParity, autodetectInvert);
			meterAutodetectLastChange = now;
		}
	} else if(autodetect) {
		#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Meter serial autodetected, saving: %d, %d, %s\n"), autodetectBaud, autodetectParity, autodetectInvert ? "true" : "false");
		autodetect = false;
		meterConfig.baud = autodetectBaud;
		meterConfig.parity = autodetectParity;
		meterConfig.invert = autodetectInvert;
		configChanged = true;
		setupHanPort(meterConfig.baud, meterConfig.parity, meterConfig.invert);
	}
}
