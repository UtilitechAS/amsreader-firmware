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
	int32_t getInt(uint8_t objectId); // Use this for uint8, int8, uint16, int16
	uint32_t getUint(uint8_t objectId); // Only for uint32
	String getString(uint8_t objectId);
	time_t getTime(uint8_t objectId, bool respectTimezone, bool respectDsc);
	int getBuffer(byte* buf);

	void setEncryptionKey(uint8_t* encryption_key);
	void setAuthenticationKey(uint8_t* authentication_key);

private:
	RemoteDebug* debugger;
	Stream *han;
	byte* buffer;
	int bytesRead;
	DlmsReader reader;
	int listSize;
	Timezone *localZone;
	uint8_t* encryption_key;
	uint8_t* authentication_key;

	int findValuePosition(uint8_t dataPosition, byte *buffer, int start, int length);

	time_t getTime(uint8_t dataPosition, bool respectTimezone, bool respectDsc, byte *buffer, int start, int length);
	time_t getTime(byte *buffer, int start, int length, bool respectTimezone, bool respectDsc);
	int getInt(uint8_t dataPosition, byte *buffer, int start, int length);
	int8_t getInt8(uint8_t dataPosition, byte *buffer, int start, int length);
	uint8_t getUint8(uint8_t dataPosition, byte *buffer, int start, int length);
	int16_t getInt16(uint8_t dataPosition, byte *buffer, int start, int length);
	uint16_t getUint16(uint8_t dataPosition, byte *buffer, int start, int length);
	uint32_t getUint32(uint8_t dataPosition, byte *buffer, int start, int length);
	String getString(uint8_t dataPosition, byte *buffer, int start, int length);

	time_t toUnixTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

	bool decryptFrame();

	void debugPrint(byte *buffer, int start, int length);

	void printD(String fmt, int arg=0);
	void printI(String fmt, int arg=0);
	void printW(String fmt, int arg=0);
	void printW(String fmt, const char* arg);
	void printE(String fmt, int arg=0);
};


#endif
