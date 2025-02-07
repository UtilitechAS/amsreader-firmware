/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "KamstrupPullCommunicator.h"
#include "Uptime.h"
#include "Cosem.h"
#include "hexutils.h"

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3)
#include <driver/uart.h>
#endif

void KamstrupPullCommunicator::configure(MeterConfig& meterConfig, Timezone* tz) {
    this->meterConfig = meterConfig;
    this->configChanged = false;
    this->tz = tz;
    setupHanPort(meterConfig.baud, meterConfig.parity, meterConfig.invert);
}

bool KamstrupPullCommunicator::loop() {
	uint64_t now = millis64();
	if(PassiveMeterCommunicator::loop() || now-lastLoop > 5000) {
		if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("State: %d\n"), state);
		lastLoop = now;
		switch(state) {
			case STATE_DISCONNECTED:
				sendConnectMessage();
				lastMessageTime = now;
				break;

			case STATE_CONNECTING:
				if(!checkForConnectConfirmed() && now-lastMessageTime > 10000) {
					state = STATE_DISCONNECTED;
					lastLoop = 0;
				}
				break;

			case STATE_CONNECTED_NOT_ASSOCIATED:
				sendAssociateMessage();
				lastMessageTime = now;
				break;

			case STATE_CONNECTED_ASSOCIATING:
				if(!checkForAssociationConfirmed() && now-lastMessageTime > 10000) {
					state = STATE_CONNECTION_BROKEN; // TODO: Use state: Broken
					lastLoop = 0;
				}
				break;

			case STATE_CONNECTED_ASSOCIATED:
				if(dataAvailable) {
					if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Data is available: %lu\n"), ctx.length);
					return true;
				} else {
					lastMessageTime = now;
					requestData();
				}
				break;

			case STATE_DISCONNECT:
			case STATE_CONNECTION_BROKEN:
				sendDisconnectMessage();
				lastMessageTime = now;
				break;

			case STATE_DISCONNECTING:
				if(!checkForDisconnectMessage() && now-lastMessageTime > 10000) {
					state = STATE_DISCONNECTED;
					lastLoop = 0;
				}
				break;

			default:
				state = STATE_DISCONNECTED;
		}
	}
	return false;
}

void KamstrupPullCommunicator::setupHanPort(uint32_t baud, uint8_t parityOrdinal, bool invert) {
	uint8_t rxPin = meterConfig.rxPin;
	uint8_t txPin = meterConfig.txPin;

	if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(setupHanPort) Setting up HAN on pin %d/%d with baud %d and parity %d\n"), rxPin, txPin, baud, parityOrdinal);

	if(rxPin == 3 || rxPin == 113) {
		#if ARDUINO_USB_CDC_ON_BOOT
			hwSerial = &Serial0;
		#else
			hwSerial = &Serial;
		#endif
	}

	#if defined(ESP32)
		if(rxPin == 9) {
			hwSerial = &Serial1;
		}
		#if defined(CONFIG_IDF_TARGET_ESP32)
			if(rxPin == 16) {
				hwSerial = &Serial2;
			}
		#elif defined(CONFIG_IDF_TARGET_ESP32S2) ||  defined(CONFIG_IDF_TARGET_ESP32C3)
			hwSerial = &Serial1;
		#endif
	#endif

	if(rxPin == 0) {
		if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Invalid GPIO configured for HAN RX\n"));
		return;
	}
	if(txPin == 0) {
		if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Invalid GPIO configured for HAN TX\n"));
		return;
	}

	if(meterConfig.bufferSize < 1) meterConfig.bufferSize = 1;

	if(hwSerial != NULL) {
		if (debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Hardware serial\n"));
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
			case 10:
				serialConfig = SERIAL_7E1;
				break;
			default:
				serialConfig = SERIAL_8E1;
				break;
		}
		if(meterConfig.bufferSize < 4) meterConfig.bufferSize = 4; // 64 bytes (1) is default for software serial, 256 bytes (4) for hardware

		hwSerial->setRxBufferSize(64 * meterConfig.bufferSize);
		#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3)
			hwSerial->begin(baud, serialConfig, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, invert);
			uart_set_pin(UART_NUM_1, txPin == 0xFF ? UART_PIN_NO_CHANGE : txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		#elif defined(ESP32)
			hwSerial->begin(baud, serialConfig, -1, -1, invert);
		#else
			hwSerial->begin(baud, serialConfig, SERIAL_FULL, 1, invert);
		#endif
		
		#if defined(ESP8266)
			if(rxPin == 3) {
				if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Switching UART0 to pin 1 & 3\n"));
				Serial.pins(1,3);
			} else if(rxPin == 113) {
				if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Switching UART0 to pin 15 & 13\n"));
				Serial.pins(15,13);
			}
		#endif

		// Prevent pullup on TX pin if not uart0
		#if defined(CONFIG_IDF_TARGET_ESP32S2)
			if(txPin != 17) pinMode(17, INPUT);
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
			if(txPin != 7) pinMode(7, INPUT);
		#elif defined(ESP32)
			if(rxPin == 9) {
				if(txPin != 10) pinMode(10, INPUT);
			} else if(rxPin == 16) {
				if(txPin != 17) pinMode(17, INPUT);
			}
		#elif defined(ESP8266)
			if(rxPin == 113) {
				if(txPin != 15) pinMode(15, INPUT);
			}
		#endif

		hanSerial = hwSerial;
		if(swSerial != NULL) {
			swSerial->end();
			delete swSerial;
			swSerial = NULL;
		}
	} else {
		if (debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Software serial\n"));
		Serial.flush();
		
		if(swSerial == NULL) {
			swSerial = new SoftwareSerial(rxPin, txPin == 0xFF ? -1 : txPin, invert);
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
			case 10:
				serialConfig = SWSERIAL_7E1;
				break;
			default:
				serialConfig = SWSERIAL_8E1;
				break;
		}

		swSerial->begin(baud, serialConfig, rxPin, txPin == 0xFF ? -1 : txPin, invert, meterConfig.bufferSize * 64);
		hanSerial = swSerial;

		Serial.end();
		Serial.begin(115200);
		hwSerial = NULL;
	}

	// The library automatically sets the pullup in Serial.begin()
	if(!meterConfig.rxPinPullup) {
		if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("HAN pin pullup disabled\n"));
		pinMode(meterConfig.rxPin, INPUT);
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

// 7E A0 15 21 03 52 5D 8A E6 E7 00 C4 01 81 00 06 00 BC 61 4F E4 36 7E
AmsData* KamstrupPullCommunicator::getData(AmsData& meterState) {
	if(!dataAvailable) return NULL;
	if(ctx.length > BUF_SIZE_HAN) {
        debugger->printf_P(PSTR("Invalid context length %lu\n"), ctx.length);
		dataAvailable = false;
		return NULL;
	}

	if(hanBuffer[5] == 0x51) {
		if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("Request was denied\n"));
		len = 0;
		dataAvailable = false;
		state = STATE_CONNECTION_BROKEN;
		return NULL;
	}

	byte* payload = ((byte *) (hanBuffer)) + pos;
    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Received data from Kamstrup meter:\n"));
        debugPrint(payload, 0, min(ctx.length, (uint16_t) BUF_SIZE_HAN), debugger);
    }

	if(hanBuffer[11] == DATA_TAG_RES) {
		if(obisPosition == 1) { // Version string
			debugger->printf_P(PSTR("RECEIVED Firmware version\n"));
		} else if(obisPosition == 1) { // Meter model string
			debugger->printf_P(PSTR("RECEIVED Meter model\n"));
		} else { // All other uint32
			uint32_t value = ntohl(*((uint32_t*) (hanBuffer + 16)));
			debugger->printf_P(PSTR("RECEIVED DATA FOR position %d, value: %lu\n"), obisPosition, value);
			meterState.apply(currentObis, value / 1.0);
		}
		len = 0;
		dataAvailable = false;
		return NULL;
	} else {
	    return PassiveMeterCommunicator::getData(meterState);
	}
}

void KamstrupPullCommunicator::sendConnectMessage() {
    uint8_t i = 3; // Leave 3 bytes for header
    hanBuffer[i++] = serverSap; // Destination address
    hanBuffer[i++] = clientSap; // Source address
    hanBuffer[i++] = 0x93; // Control
    i += 2; // Leave 2 bytes for header checksum
    hanBuffer[i++] = 0x81; // Format identifier
    hanBuffer[i++] = 0x80; // Format group
    uint8_t glPos = i++; // Position where we should write group length
    uint8_t glLen = 0; // Actual group length

    ConnectParameter2b txMax = { 0x5, 0x2, htons(0x200) };
    memcpy(hanBuffer+i, &txMax, txMax.length+2);
    i += txMax.length+2;
    glLen += txMax.length+2;

    ConnectParameter2b rxMax = { 0x6, 0x2, htons(0x200) };
    memcpy(hanBuffer+i, &rxMax, rxMax.length+2);
    i += rxMax.length+2;
    glLen += rxMax.length+2;

    ConnectParameter4b txWin = { 0x7, 0x4, htonl(0x1) };
    memcpy(hanBuffer+i, &txWin, txWin.length+2);
    i += txWin.length+2;
    glLen += txWin.length+2;

    ConnectParameter4b rxWin = { 0x8, 0x4, htonl(0x1) };
    memcpy(hanBuffer+i, &rxWin, rxWin.length+2);
    i += rxWin.length+2;
    glLen += rxWin.length+2;

    hanBuffer[glPos] = glLen;

    HDLCHeader head = { HDLC_FLAG, htons(0xA000 | (i+1)) };
    memcpy(hanBuffer, &head, 3);

    HDLC3CtrlHcs ch = { 0x93, htons(crc16_x25(hanBuffer+1, 5)) };
    memcpy(hanBuffer+5, &ch, 3);

    HDLCFooter foot = { htons(crc16_x25(hanBuffer+1, i-1)), HDLC_FLAG };
    memcpy(hanBuffer+i, &foot, 3);
	i += 3;

	for(int x = i; x<BUF_SIZE_HAN; x++) {
		hanBuffer[x] = 0x00;
	}

    hanSerial->write(hanBuffer, i);
    hanSerial->flush();

    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Sending data to Kamstrup meter:\n"));
        debugPrint(hanBuffer, 0, i, debugger);
    }
	state = STATE_CONNECTING;
	len = 0;
	dataAvailable = false;
}

bool KamstrupPullCommunicator::checkForConnectConfirmed() {
	if(!dataAvailable) return false;
	if(ctx.length > BUF_SIZE_HAN) {
        debugger->printf_P(PSTR("Invalid context length\n"));
		dataAvailable = false;
		return NULL;
	}

	byte* payload = ((byte *) (hanBuffer)) + pos;
    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Received data from Kamstrup meter:\n"));
        debugPrint(payload, 0, min(ctx.length, (uint16_t) BUF_SIZE_HAN), debugger);
    }

	len = 0;
	dataAvailable = false;
	lastMessageTime = 0;

    // 7E A0 20 21 03 73 73 98 81 80 14 05 02 02 00 06 02 02 00 07 04 00 00 00 01 08 04 00 00 00 01 6F EF 7E
	// 7E A0 20 21 03 73 73 98 81 80 14 05 02 02 00 06 02 00 80 07 04 00 00 00 01 08 04 00 00 00 01 19 D4 7E
	if(payload[0] == 0x81 && payload[1] == 0x80) {
		state = STATE_CONNECTED_NOT_ASSOCIATED;
		return true;
	} else {
		state = STATE_CONNECTION_BROKEN;
		return false;
	}
}


// TA: Tag
// LE: Legth
// RA: Response allowed
// PQ: Proposed QoS
// PV: Proposed DLMS version
// CO: Conformance
// AT: Application tag
// LC: Length of content field
// LU: Number of unused bits in the final octet
// MP: Max PDU size
// DK: Dedicated key, use (0x01), length (0xXX) and data
// AC: encoding of the tag of the xDLMS APDU CHOICE (InitiateRequest)
// FF: Fixed
// BE: Ber Object Identifier, 0x06=Object, 0x80=Context, 0x20=Constructed, 0x12=Calling auth, 0x40=Application, 0x04=String
// AN: Application context name tag
// NR: Name referencing, 1d=LN unciphered, 2d=unciphered, 3d=LN ciphered, 4d=cihered
// CA: Calling-AP-title
// CU: Calling Authentication
// SR: Sender requirements


// No authentication

// 7E A0 2B 03 21 10 FB AF 
// E6 E6 00 

// TA LE AN LE BO LE FF FF FF FF FF FF NR
// 60 1D A1 09 06 07 60 85 74 05 08 01 01                                                                  

// CA LE OS LE AC DK RA PQ PV AT AT LC LU CO CO CO MP MP
// BE 10 04 0E 01 00 00 00 06 5F 1F 04 00 00 18 1D FF FF 

// 5F AF 7E


// With authentication

// 7E A0 41 21 25 10 52 3B
// E6 E6 00                                              

// BE LE AN LE
// 60 33 A1 09

// 06=CALLING_AP_TITLE
// BE LE FF FF FF FF FF FF NR
// 06 07 60 85 74 05 08 01 01

// 8A=SENDER_ACSE_REQUIREMENTS (BE 0x80 + 0x0A)
// BE LE FF FF
// 8A 02 07 80

// 8B=MECHANISM_NAME (BE 0x80 + 0x0B)
// BE LE FF FF FF FF FF FF NR
// 8B 07 60 85 74 05 08 02 01

// AC=CALLING_AUTHENTICATION_VALUE (BE 0x80 + BE 0x20 + 0x0C)
// BE LE BE LE -- Password --
// AC 07 80 05 31 32 33 34 35

// BE=USER_INFORMATION (BE 0x80 + BE 0x20 + 0x1E)
// CA LE BE LE AC DK RA PQ PV AT AT LC LU CO CO CO MP MP
// BE 10 04 0E 01 00 00 00 06 5F 1F 04 00 00 FE 1F FF FF

// 0C FF 7E

void KamstrupPullCommunicator::sendAssociateMessage() {
	bool usePsk = !passkey.isEmpty();

    uint8_t i = 3; // Leave 3 bytes for header
    hanBuffer[i++] = serverSap; // Destination address
    hanBuffer[i++] = clientSap; // Source address
    hanBuffer[i++] = 0x10; // Control
    i += 2; // Leave 2 bytes for header checksum
    hanBuffer[i++] = 0xE6; // LLC dst
    hanBuffer[i++] = 0xE6; // LLC src
    hanBuffer[i++] = 0x00; // LLC control

	hanBuffer[i++] = DATA_TAG_AARQ;
	uint8_t aarqLenIdx = i++; // length placeholder
	hanBuffer[i++] = 0xA1;
	hanBuffer[i++] = 0x09; // Length
	hanBuffer[i++] = 0x06; // CALLING_AP_TITLE
	hanBuffer[i++] = 0x07; // Length
	hanBuffer[i++] = 0x60; // Fixed data
	hanBuffer[i++] = 0x85; // Fixed data
	hanBuffer[i++] = 0x74; // Fixed data
	hanBuffer[i++] = 0x05; // Fixed data
	hanBuffer[i++] = 0x08; // Fixed data
	hanBuffer[i++] = 0x01; // Fixed data
	hanBuffer[i++] = 0x01; // Name referencing, 1d=LN unciphered, 2d=unciphered, 3d=LN ciphered, 4d=cihered

	if(usePsk) {
		hanBuffer[i++] = 0x8A; // SENDER_ACSE_REQUIREMENTS (BE 0x80 + 0x0A)
		hanBuffer[i++] = 0x02; // Length
		hanBuffer[i++] = 0x07; // Data
		hanBuffer[i++] = 0x80; // Data

		hanBuffer[i++] = 0x8B; // MECHANISM_NAME (BE 0x80 + 0x0B)
		hanBuffer[i++] = 0x07; // Length
		hanBuffer[i++] = 0x60; // Fixed data
		hanBuffer[i++] = 0x85; // Fixed data
		hanBuffer[i++] = 0x74; // Fixed data
		hanBuffer[i++] = 0x05; // Fixed data
		hanBuffer[i++] = 0x08; // Fixed data
		hanBuffer[i++] = 0x02; // Fixed data
		hanBuffer[i++] = 0x01; // Name referencing, 1d=LN unciphered, 2d=unciphered, 3d=LN ciphered, 4d=cihered

		hanBuffer[i++] = 0xAC; // CALLING_AUTHENTICATION_VALUE (BE 0x80 + BE 0x20 + 0x0C)
		hanBuffer[i++] = passkey.length() + 2; // Length
		hanBuffer[i++] = 0x80; // Ber Context
		hanBuffer[i++] = passkey.length(); // Length

		const char* key = passkey.c_str();
		for(uint8_t x = 0; x < passkey.length(); x++) {
			hanBuffer[i++] = key[x];
		}
	}

	hanBuffer[i++] = 0xBE; // USER_INFORMATION (BE 0x80 + BE 0x20 + 0x1E)
	hanBuffer[i++] = 0x10; // Length
	hanBuffer[i++] = 0x04; // CALLED_AP_INVOCATION_ID
	hanBuffer[i++] = 0x0E; // Length
	hanBuffer[i++] = 0x01; // encoding of the tag of the xDLMS APDU CHOICE (InitiateRequest)
	hanBuffer[i++] = 0x00; // Dedicated key, use (0x01), length (0xXX) and data
	hanBuffer[i++] = 0x00; // Response allowed
	hanBuffer[i++] = 0x00; // Proposed QoS
	hanBuffer[i++] = 0x06; // Proposed DLMS version
	hanBuffer[i++] = 0x5F; // 
	hanBuffer[i++] = 0x1F; // 
	hanBuffer[i++] = 0x04; // Length
	hanBuffer[i++] = 0x00; // Number of unused bits
	hanBuffer[i++] = 0x00; // Conformance
	if(usePsk) {
		hanBuffer[i++] = 0xFE; // Conformance
		hanBuffer[i++] = 0x1F; // Conformance
	} else {
		hanBuffer[i++] = 0x18; // Conformance
		hanBuffer[i++] = 0x1D; // Conformance
	}
	hanBuffer[i++] = 0xFF; // Max PDU size
	hanBuffer[i++] = 0xFF; // Max PDU size
	hanBuffer[aarqLenIdx] = i-aarqLenIdx-1;

    HDLCHeader head = { HDLC_FLAG, htons(0xA000 | (i+1)) };
    memcpy(hanBuffer, &head, 3);

    HDLC3CtrlHcs ch = { 0x10, htons(crc16_x25(hanBuffer+1, 5)) };
    memcpy(hanBuffer+5, &ch, 3);

    HDLCFooter foot = { htons(crc16_x25(hanBuffer+1, i-1)), HDLC_FLAG };
    memcpy(hanBuffer+i, &foot, 3);
	i += 3;

	for(int x = i; x<BUF_SIZE_HAN; x++) {
		hanBuffer[x] = 0x00;
	}

    hanSerial->write(hanBuffer, i);
    hanSerial->flush();

    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Sending data to Kamstrup meter:\n"));
        debugPrint(hanBuffer, 0, i, debugger);
    }
	state = STATE_CONNECTED_ASSOCIATING;
	len = 0;
	dataAvailable = false;
}

bool KamstrupPullCommunicator::checkForAssociationConfirmed() {
	if(!dataAvailable) return false;
	if(ctx.length > BUF_SIZE_HAN) {
        debugger->printf_P(PSTR("Invalid context length\n"));
		dataAvailable = false;
		return NULL;
	}

	byte* payload = ((byte *) (hanBuffer)) + pos;
    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Received data from Kamstrup meter:\n"));
        debugPrint(payload, 0, ctx.length, debugger);
    }
	len = 0;
	dataAvailable = false;
	lastMessageTime = 0;

	if(payload[0] == DATA_TAG_AARE) {
		state = STATE_CONNECTED_ASSOCIATED;
		return true;
	} else {
		state = STATE_CONNECTION_BROKEN;
		return false;
	}

	return false;
}

// 7E A0 19 03 21 32 6F D8 E6 E6 00 C0 01 81 00 01 01 01 00 00 01 FF 02 00 A8 E3 7E
// 7E A0 19 21 25 32 8C 09 E6 E6 00 C0 01 81 00 01 01 01 60 01 01 FF 02 00 5D 6F 7E
// 7E A0 19 21 25 32 8C 09 E6 E6 00 C0 01 C1 00 03 01 01 20 07 00 FF 02 00 38 36 7E

// 7E A0 4C 21 25 32 CD B2 E6 E6 00 C0 01 81 00 
// 07 - Class
// 01 01 63 01 00 FF - OBIS
// 02 - Attribute number 2
// 01 01 - Array 1
// 02 04 - Struct 4
// 02 04 - Struct 4
// 12 00 08 - uint16
// 09 06 00 01 01 00 00 FF - OBIS
// 0F 02 - int8
// 12 00 00 - uint16
// 09 0C 07 DD 0A 19 FF 00 00 00 00 80 00 80 - from date
// 09 0C 07 DD 0A 1A FF 00 00 00 00 80 00 80 - to date
// 01 00 - Empty array
// 2F 84 7E
bool KamstrupPullCommunicator::requestData() {
	bool usePsk = !passkey.isEmpty();

    uint8_t i = 3; // Leave 3 bytes for header
    hanBuffer[i++] = serverSap; // Destination address
    hanBuffer[i++] = clientSap; // Source address
    hanBuffer[i++] = 0x32; // Control
    i += 2; // Leave 2 bytes for header checksum
    hanBuffer[i++] = 0xE6; // LLC dst
    hanBuffer[i++] = 0xE6; // LLC src
    hanBuffer[i++] = 0x00; // LLC control

	hanBuffer[i++] = 0xC0; // Get Request
	hanBuffer[i++] = 0x01; // Type, 01 = Normal
	hanBuffer[i++] = 0x81; // Invoke ID and priority
	hanBuffer[i++] = 0x00;
	hanBuffer[i++] = ++obisPosition < 3 ? 0x01 : 0x03; // Class ID

	OBIS_t obis = {1,1,OBIS_NULL,OBIS_RANGE_NA};
	switch(obisPosition) {
		case 1: obis.code = OBIS_FIRMWARE_VERSION; break;
		case 2: obis.code = OBIS_METER_MODEL; break;
		case 3: obis.code = OBIS_METER_ID; break;

		case 4: obis.code = OBIS_ACTIVE_IMPORT; break;
		case 5: obis.code = OBIS_REACTIVE_IMPORT; break;
		case 6: obis.code = OBIS_ACTIVE_EXPORT; break;
		case 7: obis.code = OBIS_REACTIVE_EXPORT; break;
		default:
		obisPosition = 0; return false;
	}
    memcpy(hanBuffer+i, &obis, sizeof(obis));
	i += sizeof(obis);
	currentObis = obis.code;

	hanBuffer[i++] = 0x02; // Attribute number
	hanBuffer[i++] = 0x00;

    HDLCHeader head = { HDLC_FLAG, htons(0xA000 | (i+1)) };
    memcpy(hanBuffer, &head, 3);

    HDLC3CtrlHcs ch = { 0x32, htons(crc16_x25(hanBuffer+1, 5)) };
    memcpy(hanBuffer+5, &ch, 3);

    HDLCFooter foot = { htons(crc16_x25(hanBuffer+1, i-1)), HDLC_FLAG };
    memcpy(hanBuffer+i, &foot, 3);
	i += 3;

	for(int x = i; x<BUF_SIZE_HAN; x++) {
		hanBuffer[x] = 0x00;
	}

    hanSerial->write(hanBuffer, i);
    hanSerial->flush();

    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Sending data to Kamstrup meter:\n"));
        debugPrint(hanBuffer, 0, i, debugger);
    }

	len = 0;
	dataAvailable = false;

    return true;
}

//7E A0 07 03 21 53 03 C7 7E
void KamstrupPullCommunicator::sendDisconnectMessage() {
    uint8_t i = 3; // Leave 3 bytes for header
    hanBuffer[i++] = serverSap; // Destination address
    hanBuffer[i++] = clientSap; // Source address
    hanBuffer[i++] = 0x53; // Control

    HDLCHeader head = { HDLC_FLAG, htons(0xA000 | (i+1)) };
    memcpy(hanBuffer, &head, 3);

    HDLCFooter foot = { htons(crc16_x25(hanBuffer+1, i-1)), HDLC_FLAG };
    memcpy(hanBuffer+i, &foot, 3);
	i += 3;

	for(int x = i; x<BUF_SIZE_HAN; x++) {
		hanBuffer[x] = 0x00;
	}

    hanSerial->write(hanBuffer, i);
    hanSerial->flush();

    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Sending data to Kamstrup meter:\n"));
        debugPrint(hanBuffer, 0, i, debugger);
    }
	state = STATE_DISCONNECTING;
	len = 0;
	dataAvailable = false;
}

// 7E A0 07 21 03 73 01 40 7E
bool KamstrupPullCommunicator::checkForDisconnectMessage() {
	if(!dataAvailable) return false;
	if(ctx.length > BUF_SIZE_HAN) {
        debugger->printf_P(PSTR("Invalid context length\n"));
		dataAvailable = false;
		return NULL;
	}

    if(debugger->isActive(RemoteDebug::INFO)) {
        debugger->printf_P(PSTR("Received data from Kamstrup meter:\n"));
        debugPrint(hanBuffer, 0, ctx.length, debugger);
    }
	for(int i = 0; i<BUF_SIZE_HAN; i++) {
		hanBuffer[i] = 0x00;
	}

	len = 0;
	dataAvailable = false;

	if(hanBuffer[3] == serverSap && hanBuffer[4] == clientSap && hanBuffer[5] == 0x73) {
		state = STATE_DISCONNECTED;
		return true;
	} else {
		state = STATE_DISCONNECTED;
		return false;
	}

	return false;
}

HardwareSerial* KamstrupPullCommunicator::getHwSerial() {
    return hwSerial;
}

int KamstrupPullCommunicator::getLastError() {
	if(hwSerial != NULL) {
		#if defined ESP8266
		if(hwSerial->hasRxError()) {
			if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Serial RX error\n"));
			lastError = 96;
		}
		if(hwSerial->hasOverrun()) {
			rxerr(2);
		}
		#endif
	} else if(swSerial != NULL) {
		if(swSerial->overflow()) {
			rxerr(2);
		}
	}
    return lastError;
}

bool KamstrupPullCommunicator::isConfigChanged() {
    return configChanged;
}

void KamstrupPullCommunicator::getCurrentConfig(MeterConfig& meterConfig) {
    meterConfig = this->meterConfig;
}

void KamstrupPullCommunicator::rxerr(int err) {
	if(err == 0) return;
	switch(err) {
		case 2:
			if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Serial buffer overflow\n"));
			rxBufferErrors++;
			if(rxBufferErrors > 3 && meterConfig.bufferSize < 64) {
				meterConfig.bufferSize += 2;
				if (debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Increasing RX buffer to %d bytes\n"), meterConfig.bufferSize * 64);
                configChanged = true;
				rxBufferErrors = 0;
			}
			break;
		case 3:
			if (debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("Serial FIFO overflow\n"));
			break;
		case 4:
			if (debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("Serial frame error\n"));
			break;
		case 5:
			if (debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("Serial parity error\n"));
			break;
	}
	// Do not include serial break
	if(err > 1) lastError = 90+err;
}
