#ifndef _HANREADER_h
#define _HANREADER_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif


#include "KaifaHan.h"
#include "DlmsReader.h"


class HanReader
{
  public:
    HanReader();
    void setup(HardwareSerial *hanPort);
    void setup(HardwareSerial *hanPort, Stream *debugPort);
	void setup(HardwareSerial *hanPort, unsigned long baudrate, SerialConfig config, Stream *debugPort);
    bool read();
	List getList();
	time_t getPackageTime();
	int getInt(List1_ObisObjects objectId);
	int getInt(List2_ObisObjects objectId);
	int getInt(List3_ObisObjects objectId);
	int getInt(int objectId);
	String getString(List1_ObisObjects objectId);
	String getString(List2_ObisObjects objectId);
	String getString(List3_ObisObjects objectId);
	String getString(int objectId);

  private:
    Stream *debug;
    HardwareSerial *han;
    byte buffer[512];
    int bytesRead;
    KaifaHan kaifa;
    DlmsReader reader;
	List list;
};


#endif
