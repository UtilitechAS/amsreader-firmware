#include "HanReader.h"

#if defined(ESP32)
#include "mbedtls/gcm.h"
#endif

HanReader::HanReader() {
	// Central European Time (Frankfurt, Paris)
	TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
	TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
	localZone = new Timezone(CEST, CET);
}

void HanReader::setup(Stream *hanPort, RemoteDebug *debug)
{
	han = hanPort;
	bytesRead = 0;
	debugger = debug;

	if (debug) debug->println("MBUS serial setup complete");
}

void HanReader::setup(Stream *hanPort){
	setup(hanPort, NULL);
}

void HanReader::setEncryptionKey(uint8_t* encryption_key) {
	this->encryption_key = encryption_key;
}

void HanReader::setAuthenticationKey(uint8_t* authentication_key) {
	this->authentication_key = authentication_key;
}


bool HanReader::read(byte data) {
	if (reader.Read(data, debugger->isActive(RemoteDebug::DEBUG) ? debugger : NULL)) {
		bytesRead = reader.getBytesRead();
		buffer = reader.getBuffer();
		if (debugger->isActive(RemoteDebug::INFO)) {
			printI("Got valid DLMS data (%d bytes)", bytesRead);
			if (debugger->isActive(RemoteDebug::DEBUG)) {
				byte* full = reader.getFullBuffer();
				int size = reader.getFullBufferLength();
				printI("Full DLMS frame (%d bytes)", size);
				debugPrint(full, 0, size);
			}
		}

		/*
			Data should start with E6 E7 00 0F
			and continue with four bytes for the InvokeId
		*/
		if (bytesRead < 9) {
			printW("Invalid HAN data: Less than 9 bytes received");
			return false;
		}
		else if (
			buffer[0] != 0xE6 
			|| buffer[1] != 0xE7
			|| buffer[2] != 0x00
		)
		{
			printW("Invalid HAN data: Start should be E6 E7 00");
			return false;
		}

		// Have not found any documentation supporting this, but 0x0F for all norwegian meters.
		// Danish meters with encryption has 0xDB, so lets assume this has something to do with that.
		switch(buffer[3]) {
			case 0x0F:
				dataHeader = 8;
				break;
			case 0xDB:
				printI("Decrypting frame");
				if(!decryptFrame()) return false;
				if (debugger->isActive(RemoteDebug::DEBUG)) {
					printD("Data after decryption:");
					debugPrint(buffer, 0, bytesRead);
				}
				dataHeader = 26;
				break;
		}

		listSize = getInt(0, buffer, 0, bytesRead);
		printI("HAN data is valid, listSize: %d", listSize);
		return true;
	}

	return false;
}

const size_t headersize = 3;
const size_t footersize = 0;

bool HanReader::decryptFrame() {
	uint8_t system_title[8];
    memcpy(system_title, buffer + headersize + 2, 8);
	if (debugger->isActive(RemoteDebug::DEBUG)) {
		printD("System title:");
		debugPrint(system_title, 0, 8);
	}

	uint8_t initialization_vector[12];
    memcpy(initialization_vector, system_title, 8);
    memcpy(initialization_vector + 8, buffer + headersize + 14, 4);
	if (debugger->isActive(RemoteDebug::DEBUG)) {
		printD("Initialization vector:");
		debugPrint(initialization_vector, 0, 12);
	}

    uint8_t additional_authenticated_data[17];
    memcpy(additional_authenticated_data, buffer + headersize + 13, 1);
    memcpy(additional_authenticated_data + 1, authentication_key, 16);
	if (debugger->isActive(RemoteDebug::DEBUG)) {
		printD("Additional authenticated data:");
		debugPrint(additional_authenticated_data, 0, 17);
	}

    uint8_t authentication_tag[12];
    memcpy(authentication_tag, buffer + headersize + bytesRead - headersize - footersize - 12, 12);
	if (debugger->isActive(RemoteDebug::DEBUG)) {
		printD("Authentication tag:");
		debugPrint(authentication_tag, 0, 12);
	}

	if (debugger->isActive(RemoteDebug::DEBUG)) {
		printD("Encryption key:");
		debugPrint(encryption_key, 0, 16);
	}

#if defined(ESP8266)
	br_gcm_context gcmCtx;
	br_aes_ct_ctr_keys bc;
	br_aes_ct_ctr_init(&bc, encryption_key, 16);
	br_gcm_init(&gcmCtx, &bc.vtable, br_ghash_ctmul32);
	br_gcm_reset(&gcmCtx, initialization_vector, sizeof(initialization_vector));
	br_gcm_aad_inject(&gcmCtx, additional_authenticated_data, sizeof(additional_authenticated_data));
	br_gcm_flip(&gcmCtx);
	br_gcm_run(&gcmCtx, 0, buffer + headersize + 18, bytesRead - headersize - footersize - 18 - 12);
	if(br_gcm_check_tag_trunc(&gcmCtx, authentication_tag, 12) != 1) {
		printE("authdecrypt failed");
		return false;
	}
#elif defined(ESP32)
    uint8_t cipher_text[bytesRead - headersize - footersize - 18 - 12];
    memcpy(cipher_text, buffer + headersize + 18, bytesRead - headersize - footersize - 12 - 18);

	mbedtls_gcm_context m_ctx;
	mbedtls_gcm_init(&m_ctx);
	int success = mbedtls_gcm_setkey(&m_ctx, MBEDTLS_CIPHER_ID_AES, encryption_key, 128);
	if (0 != success ) {
		printE("Setkey failed: " + String(success));
		return false;
	}
	success = mbedtls_gcm_auth_decrypt(&m_ctx, sizeof(cipher_text), initialization_vector, sizeof(initialization_vector),
        additional_authenticated_data, sizeof(additional_authenticated_data), authentication_tag, sizeof(authentication_tag),
        cipher_text, buffer + headersize + 18);
	if (0 != success) {
		printE("authdecrypt failed: " + String(success));
		return false;
	}
	mbedtls_gcm_free(&m_ctx);
#endif
	return true;
}

void HanReader::debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			debugger->print("0");
		debugger->print(buffer[i], HEX);
		debugger->print(" ");
		if ((i - start + 1) % 16 == 0)
			debugger->println("");
		else if ((i - start + 1) % 4 == 0)
			debugger->print(" ");

		yield(); // Let other get some resources too
	}
	debugger->println("");
}

bool HanReader::read() {
	while(han->available()) {
		if(read(han->read())) {
			return true;
		}
	}
	return false;
}

int HanReader::getListSize() {
	return listSize;
}

time_t HanReader::getPackageTime(bool respectTimezone, bool respectDsc) {
	int packageTimePosition = dataHeader 
		+ (compensateFor09HeaderBug ? 1 : 0);

	return getTime(buffer, packageTimePosition, bytesRead, respectTimezone, respectDsc);
}

time_t HanReader::getTime(uint8_t objectId, bool respectTimezone, bool respectDsc) {
	return getTime(objectId, respectTimezone, respectDsc, buffer, 0, bytesRead);
}

int32_t HanReader::getInt(uint8_t objectId) {
	return getInt(objectId, buffer, 0, bytesRead);
}

uint32_t HanReader::getUint(uint8_t objectId) {
	return getUint32(objectId, buffer, 0, bytesRead);
}

String HanReader::getString(uint8_t objectId) {
	return getString(objectId, buffer, 0, bytesRead);
}

int HanReader::getBuffer(byte* buf) {
	for (int i = 0; i < bytesRead; i++) {
		buf[i] = buffer[i];
	}
	return bytesRead;
}

int HanReader::findValuePosition(uint8_t dataPosition, byte *buffer, int start, int length) {
	// The first byte after the header gives the length 
	// of the extended header information (variable)
	int headerSize = dataHeader + (compensateFor09HeaderBug ? 1 : 0);
	int firstData = headerSize + buffer[headerSize] + 1;

	for (int i = start + firstData; i<length; i++) {
		if (dataPosition-- == 0)
			return i;
		else if (buffer[i] == 0x00) // null
			i += 0;
		else if (buffer[i] == 0x0A) // String
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x09) // byte array
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x01) // array (1 byte for reading size)
			i += 1;
		else if (buffer[i] == 0x02) // struct (1 byte for reading size)
			i += 1;
		else if (buffer[i] == 0x10) // int16 value (2 bytes)
			i += 2;
		else if (buffer[i] == 0x12) // uint16 value (2 bytes)
			i += 2;
		else if (buffer[i] == 0x06) // uint32 value (4 bytes)
			i += 4;
		else if (buffer[i] == 0x0F) // int8 value (1 bytes)
			i += 1;
		else if (buffer[i] == 0x16) // enum (1 bytes)
			i += 1;
		else {
			printW("Unknown data type found: 0x%s", String(buffer[i], HEX).c_str());
			return 0; // unknown data type found
		}
	}

	printD("Passed the end of the data. Length was: %d", length);

	return 0;
}


time_t HanReader::getTime(uint8_t dataPosition, bool respectTimezone, bool respectDsc, byte *buffer, int start, int length) {
	// TODO: check if the time is represented always as a 12 byte string (0x09 0x0C)
	int timeStart = findValuePosition(dataPosition, buffer, start, length);
	timeStart += 1;
	return getTime(buffer, start + timeStart, length - timeStart, respectTimezone, respectDsc);
}

time_t HanReader::getTime(byte *buffer, int start, int length, bool respectTimezone, bool respectDsc) {
	int pos = start;
	int dataLength = buffer[pos++];

	if (dataLength == 0x0C) {
		int year = buffer[pos] << 8 |
			buffer[pos + 1];

		int month = buffer[pos + 2];
		int day = buffer[pos + 3];
		// 4: Day of week
		int hour = buffer[pos + 5];
		int minute = buffer[pos + 6];
		int second = buffer[pos + 7];
		// 8: Hundredths
		int16_t tzMinutes = buffer[pos + 9] << 8 | buffer[pos + 10];
		bool dsc = (buffer[pos + 11] & 0x80) == 0x80;

		tmElements_t tm;
		tm.Year = year - 1970;
		tm.Month = month;
		tm.Day = day;
		tm.Hour = hour;
		tm.Minute = minute;
		tm.Second = second;

		time_t time = makeTime(tm);
		if(respectTimezone && tzMinutes != 0x8000) {
			time -= tzMinutes * 60;
			if(respectDsc && dsc)
				time -= 3600;
		} else {
			if(respectDsc && dsc)
				time += 3600;
			time = localZone->toUTC(time);
		}
		return time;
	} else if(dataLength == 0) {
		return (time_t)0L;
	} else {
		printW("Unknown time length: %d", dataLength);
		// Date format not supported
		return (time_t)0L;
	}
}

int HanReader::getInt(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);

	if (valuePosition > 0) {
		switch (buffer[valuePosition++]) {
			case 0x01:
			case 0x02:
			case 0x16:
				return getUint8(dataPosition, buffer, start, length);
			case 0x0F:
				return getInt8(dataPosition, buffer, start, length);
			case 0x12:
				return getUint16(dataPosition, buffer, start, length);
			case 0x10: 
				return getInt16(dataPosition, buffer, start, length);
			case 0x06:
				return getUint32(dataPosition, buffer, start, length);
		}
	}
	return 0;
}

int8_t HanReader::getInt8(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0 && buffer[valuePosition++] == 0x0F) {
		return buffer[valuePosition];
	}
	return 0;
}

int16_t HanReader::getInt16(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0 && buffer[valuePosition++] == 0x10) {
		return buffer[valuePosition] << 8 | buffer[valuePosition+1];
	}
	return 0;
}

uint8_t HanReader::getUint8(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0) {
		switch(buffer[valuePosition++]) {
			case 0x01:
			case 0x02:
			case 0x16:
				return buffer[valuePosition];
		}
	}
	return 0;
}

uint16_t HanReader::getUint16(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0 && buffer[valuePosition++] == 0x12) {
		return buffer[valuePosition] << 8 | buffer[valuePosition+1];
	}
	return 0;
}

uint32_t HanReader::getUint32(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0) {
		if(buffer[valuePosition++] != 0x06)
			return 0;
		uint32_t value = 0;
		for (int i = valuePosition; i < valuePosition + 4; i++) {
			value = value << 8 | buffer[i];
		}
		return value;
	}
	return 0;
}

String HanReader::getString(uint8_t dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0) {
		String value = String("");
		for (int i = valuePosition + 2; i < valuePosition + buffer[valuePosition + 1] + 2; i++) {
			value += String((char)buffer[i]);
		}
		return value;
	}
	return String("");
}

void HanReader::printD(String fmt, int arg) {
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(String("(HanReader)" + fmt + "\n").c_str(), arg);
}

void HanReader::printI(String fmt, int arg) {
	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(String("(HanReader)" + fmt + "\n").c_str(), arg);
}

void HanReader::printW(String fmt, int arg) {
	if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf(String("(HanReader)" + fmt + "\n").c_str(), arg);
}

void HanReader::printW(String fmt, const char* arg) {
	if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf(String("(HanReader)" + fmt + "\n").c_str(), arg);
}

void HanReader::printE(String fmt, int arg) {
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(String("(HanReader)" + fmt + "\n").c_str(), arg);
}
