#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif


#include "DlmsReader.h"


class HanReader
{
public:
	const uint dataHeader = 8;

	HanReader();
	void setup(HardwareSerial *hanPort);
	void setup(HardwareSerial *hanPort, Stream *debugPort);
	void setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort);
	bool read();
	bool read(byte data);
	int getListSize();
	time_t getPackageTime();
	int getInt(int objectId);
	String getString(int objectId);
	time_t getTime(int objectId);

private:
	Stream *debug;
	HardwareSerial *han;
	byte buffer[512];
	int bytesRead;
	DlmsReader reader;
	int listSize;

	int findValuePosition(int dataPosition, byte *buffer, int start, int length);

	long getTime(int dataPosition, byte *buffer, int start, int length);
	int getInt(int dataPosition, byte *buffer, int start, int length);
	String getString(int dataPosition, byte *buffer, int start, int length);

	time_t toUnixTime(int year, int month, int day, int hour, int minute, int second);
};


#endif
