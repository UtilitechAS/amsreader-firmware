#ifndef _CRC16_h
#define _CRC16_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif

class Crc16Class
{
  public:
    Crc16Class();
    unsigned short ComputeChecksum(byte *data, int start, int length);
  protected:
  private:
    const unsigned short polynomial = 0x8408;
    unsigned short table[256];
};

#endif


