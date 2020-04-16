#include "HanReader.h"

HanReader::HanReader()
{
}

void HanReader::setup(Stream *hanPort, RemoteDebug *debug)
{
	han = hanPort;
	bytesRead = 0;
	debugger = debug;

	// Central European Time (Frankfurt, Paris)
	TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
	TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
	localZone = new Timezone(CEST, CET);

	if (debug) debug->println("MBUS serial setup complete");
}

void HanReader::setup(Stream *hanPort)
{
	setup(hanPort, NULL);
}

bool HanReader::read(byte data) {
	if (reader.Read(data)) {
		bytesRead = reader.GetRawData(buffer, 0, 512);
		if (debugger->isActive(RemoteDebug::INFO)) {
			printI("Got valid DLMS data (%d bytes)", bytesRead);
			if (debugger->isActive(RemoteDebug::DEBUG)) {
				debugPrint(buffer, 0, bytesRead);
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
			buffer[0] != 0xE6 || 
			buffer[1] != 0xE7 ||
			buffer[2] != 0x00 ||
			buffer[3] != 0x0F
		)
		{
			printW("Invalid HAN data: Start should be E6 E7 00 0F");
			return false;
		}

		listSize = getInt(0, buffer, 0, bytesRead);
		printI("HAN data is valid, listSize: %d", listSize);
		return true;
	}

	return false;
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

time_t HanReader::getPackageTime() {
	int packageTimePosition = dataHeader 
		+ (compensateFor09HeaderBug ? 1 : 0);

	return getTime(buffer, packageTimePosition, bytesRead);
}

time_t HanReader::getTime(int objectId) {
	return getTime(objectId, buffer, 0, bytesRead);
}

int HanReader::getInt(int objectId) {
	return getInt(objectId, buffer, 0, bytesRead);
}

String HanReader::getString(int objectId) {
	return getString(objectId, buffer, 0, bytesRead);
}


int HanReader::findValuePosition(int dataPosition, byte *buffer, int start, int length) {
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


time_t HanReader::getTime(int dataPosition, byte *buffer, int start, int length) {
	// TODO: check if the time is represented always as a 12 byte string (0x09 0x0C)
	int timeStart = findValuePosition(dataPosition, buffer, start, length);
	timeStart += 1;
	return getTime(buffer, start + timeStart, length - timeStart);
}

time_t HanReader::getTime(byte *buffer, int start, int length) {
	int pos = start;
	int dataLength = buffer[pos++];

	if (dataLength == 0x0C) {
		int year = buffer[pos] << 8 |
			buffer[pos + 1];

		int month = buffer[pos + 2];
		int day = buffer[pos + 3];
		int hour = buffer[pos + 5];
		int minute = buffer[pos + 6];
		int second = buffer[pos + 7];

		tmElements_t tm;
		tm.Year = year - 1970;
		tm.Month = month;
		tm.Day = day;
		tm.Hour = hour;
		tm.Minute = minute;
		tm.Second = second;
		return localZone->toUTC(makeTime(tm));
	} else if(dataLength == 0) {
		return (time_t)0L;
	} else {
		printW("Unknown time length: %d", dataLength);
		// Date format not supported
		return (time_t)0L;
	}
}

int HanReader::getInt(int dataPosition, byte *buffer, int start, int length) {
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);

	if (valuePosition > 0) {
		int value = 0;
		int bytes = 0;
		bool signedInt = false;
		switch (buffer[valuePosition++]) {
			case 0x10: 
				bytes = 2;
				signedInt = true;
				break;
			case 0x12: 
				bytes = 2;
				break;
			case 0x06:
				bytes = 4;
				break;
			case 0x02:
				bytes = 1;
				break;
			case 0x01:
				bytes = 1;
				break;
			case 0x0F:
				bytes = 1;
				break;
			case 0x16:
				bytes = 1;
				break;
		}

		for (int i = valuePosition; i < valuePosition + bytes; i++) {
			value = value << 8 | buffer[i];
		}

		if (signedInt) {
			if (bytes == 2 && value > 0x7FFF) {
				value = value - 0x10000;
			}
		}
		
		return value;
	}
	return 0;
}

String HanReader::getString(int dataPosition, byte *buffer, int start, int length) {
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
