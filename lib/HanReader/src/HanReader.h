#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif


#include "DlmsReader.h"
#include <Timezone.h>
#include "RemoteDebug.h"

class HanReader
{
public:
	const uint dataHeader = 8;
	bool compensateFor09HeaderBug = false;

	HanReader();
	void setup(Stream *hanPort);
	void setup(Stream *hanPort, RemoteDebug *debug);
	bool read();
	bool read(byte data);
	int getListSize();
	time_t getPackageTime();
	int getInt(int objectId);
	String getString(int objectId);
	time_t getTime(int objectId);

private:
	RemoteDebug* debugger;
	Stream *han;
	byte buffer[512];
	int bytesRead;
	DlmsReader reader;
	int listSize;
	Timezone *localZone;

	int findValuePosition(int dataPosition, byte *buffer, int start, int length);

	time_t getTime(int dataPosition, byte *buffer, int start, int length);
	time_t getTime(byte *buffer, int start, int length);
	int getInt(int dataPosition, byte *buffer, int start, int length);
	String getString(int dataPosition, byte *buffer, int start, int length);

	time_t toUnixTime(int year, int month, int day, int hour, int minute, int second);

	void debugPrint(byte *buffer, int start, int length);

	void printD(String fmt, int arg=0);
	void printI(String fmt, int arg=0);
	void printW(String fmt, int arg=0);
	void printW(String fmt, const char* arg);
	void printE(String fmt, int arg=0);
};


#endif
