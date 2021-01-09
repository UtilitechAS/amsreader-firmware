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
	uint dataHeader = 8;
	bool compensateFor09HeaderBug = false;

	HanReader();
	void setup(Stream *hanPort);
	void setup(Stream *hanPort, RemoteDebug *debug);
	bool read();
	bool read(byte data);
	int getListSize();
	time_t getPackageTime(bool respectTimezone, bool respectDsc);
	int32_t getInt(int objectId); // Use this for uint8, int8, uint16, int16
	uint32_t getUint(int objectId); // Only for uint32
	String getString(int objectId);
	time_t getTime(int objectId, bool respectTimezone, bool respectDsc);
	int getBuffer(byte* buf);

	void setEncryptionKey(uint8_t* encryption_key);
	void setAuthenticationKey(uint8_t* authentication_key);

private:
	RemoteDebug* debugger;
	Stream *han;
	byte buffer[512];
	int bytesRead;
	DlmsReader reader;
	int listSize;
	Timezone *localZone;
	uint8_t encryption_key[16];
	uint8_t authentication_key[16];

	int findValuePosition(int dataPosition, byte *buffer, int start, int length);

	time_t getTime(int dataPosition, bool respectTimezone, bool respectDsc, byte *buffer, int start, int length);
	time_t getTime(byte *buffer, int start, int length, bool respectTimezone, bool respectDsc);
	int getInt(int dataPosition, byte *buffer, int start, int length);
	int8_t getInt8(int dataPosition, byte *buffer, int start, int length);
	uint8_t getUint8(int dataPosition, byte *buffer, int start, int length);
	int16_t getInt16(int dataPosition, byte *buffer, int start, int length);
	uint16_t getUint16(int dataPosition, byte *buffer, int start, int length);
	uint32_t getUint32(int dataPosition, byte *buffer, int start, int length);
	String getString(int dataPosition, byte *buffer, int start, int length);

	time_t toUnixTime(int year, int month, int day, int hour, int minute, int second);

	bool decryptFrame();

	void debugPrint(byte *buffer, int start, int length);

	void printD(String fmt, int arg=0);
	void printI(String fmt, int arg=0);
	void printW(String fmt, int arg=0);
	void printW(String fmt, const char* arg);
	void printE(String fmt, int arg=0);
};


#endif
