#include "HanReader.h"

HanReader::HanReader()
{
  
}

void HanReader::setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort)
{
	// Initialize H/W serial port for MBus communication
	if (hanPort != NULL)
	{
		hanPort->begin(baudrate, config);
		while (!hanPort) {}
	}
	
	han = hanPort;
	bytesRead = 0;
	debug = debugPort;
	if (debug) debug->println("MBUS serial setup complete");
}

void HanReader::setup(HardwareSerial *hanPort)
{
	setup(hanPort, 2400, SERIAL_8E1, NULL);
}

void HanReader::setup(HardwareSerial *hanPort, Stream *debugPort)
{
	setup(hanPort, 2400, SERIAL_8E1, debugPort);
}

bool HanReader::read(byte data)
{
	if (reader.Read(data))
	{
		bytesRead = reader.GetRawData(buffer, 0, 512);
		listSize = getInt(1, buffer, 0, bytesRead);
		return true;
	}
}

bool HanReader::read()
{
	if (han->available())
	{
		byte newByte = han->read();
		return read(newByte);
	}
	return false;
}

int HanReader::getListSize()
{
	return listSize;
}

time_t HanReader::getPackageTime()
{
	return getTime(0);
}

time_t HanReader::getTime(int objectId)
{
	return getTime(objectId, buffer, 0, bytesRead);
}

int HanReader::getInt(int objectId)
{
	return getInt(objectId, buffer, 0, bytesRead);
}

String HanReader::getString(int objectId)
{
	return getString(objectId, buffer, 0, bytesRead);
}


int HanReader::findValuePosition(int dataPosition, byte *buffer, int start, int length)
{
	for (int i = start + dataHeader; i<length; i++)
	{
		if (dataPosition-- == 0)
			return i;
		else if (buffer[i] == 0x0A) // OBIS code value
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x09) // string value
			i += buffer[i + 1] + 1;
		else if (buffer[i] == 0x02) // byte value (1 byte)
			i += 1;
		else if (buffer[i] == 0x12) // integer value (2 bytes)
			i += 2;
		else if (buffer[i] == 0x06) // integer value (4 bytes)
			i += 4;
		else
		{
			if (debug)
			{
				debug->print("Unknown data type found: 0x");
				debug->println(buffer[i], HEX);
			}
			return 0; // unknown data type found
		}
	}

	if (debug)
	{
		debug->print("Passed the end of the data. Length was: ");
		debug->println(length);
	}

	return 0;
}


time_t HanReader::getTime(int dataPosition, byte *buffer, int start, int length)
{
	// TODO: check if the time is represented always as a 12 byte string (0x09 0x0C)
	int timeStart = findValuePosition(dataPosition, buffer, start, length);
	timeStart += 2;

	int year = buffer[start + timeStart] << 8 |
		buffer[start + timeStart + 1];

	int month = buffer[start + timeStart + 2];
	int day = buffer[start + timeStart + 3];
	int hour = buffer[start + timeStart + 5];
	int minute = buffer[start + timeStart + 6];
	int second = buffer[start + timeStart + 7];

	return toUnixTime(year, month, day, hour, minute, second);
}

int HanReader::getInt(int dataPosition, byte *buffer, int start, int length)
{
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);

	if (valuePosition > 0)
	{
		int value = 0;
		int bytes = 0;
		switch (buffer[valuePosition++])
		{
			case 0x12: 
				bytes = 2;
				break;
			case 0x06:
				bytes = 4;
				break;
			case 0x02:
				bytes = 1;
				break;
		}

		for (int i = valuePosition; i < valuePosition + bytes; i++)
		{
			value = value << 8 | buffer[i];
		}

		return value;
	}
	return 0;
}

String HanReader::getString(int dataPosition, byte *buffer, int start, int length)
{
	int valuePosition = findValuePosition(dataPosition, buffer, start, length);
	if (valuePosition > 0)
	{
		String value = String("");
		for (int i = valuePosition + 2; i < valuePosition + buffer[valuePosition + 1] + 2; i++)
		{
			value += String((char)buffer[i]);
		}
		return value;
	}
	return String("");
}

time_t HanReader::toUnixTime(int year, int month, int day, int hour, int minute, int second)
{
	byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	long secondsPerMinute = 60;
	long secondsPerHour = secondsPerMinute * 60;
	long secondsPerDay = secondsPerHour * 24;

	long time = (year - 1970) * secondsPerDay * 365L;

	for (int yearCounter = 1970; yearCounter<year; yearCounter++)
		if ((yearCounter % 4 == 0) && ((yearCounter % 100 != 0) || (yearCounter % 400 == 0)))
			time += secondsPerDay;

	if (month > 2 && (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))
		time += secondsPerDay;

	for (int monthCounter = 1; monthCounter<month; monthCounter++)
		time += daysInMonth[monthCounter - 1] * secondsPerDay;

	time += (day - 1) * secondsPerDay;
	time += hour * secondsPerHour;
	time += minute * secondsPerMinute;
	time += second;

	return (time_t)time;
}