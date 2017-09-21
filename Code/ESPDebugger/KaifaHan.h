#ifndef _KAIFAHAN_h
#define _KAIFAHAN_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif

class KaifaHan
{
  public:
    const byte ListUnknown = 0x00;
    const byte List1 = 0x01;
    const byte List2 = 0x0D;
    const byte List3 = 0x12;
    byte GetListID(byte *buffer, int start, int length);
    long GetPackageTime(byte *buffer, int start, int length);
  protected:
    
    
  private:
  time_t toUnixTime(int year, int month, int day, int hour, int minute, int second);
};


#endif
