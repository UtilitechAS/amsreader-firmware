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
    byte GetListID(byte *buffer, int start, int length);
    long GetPackageTime(byte *buffer, int start, int length);
    int GetInt(int dataPosition, byte *buffer, int start, int length);
    String GetString(int dataPosition, byte *buffer, int start, int length);
  protected:
    
  private:
  int findValuePosition(int dataPosition, byte *buffer, int start, int length);
  time_t toUnixTime(int year, int month, int day, int hour, int minute, int second);
};

enum class List : byte {
  ListUnknown = 0x00,
  List1 = 0x01,
  List2 = 0x0D,
  List3 = 0x12
};


enum class List1_ObisObjects {
  ActivePowerImported
};

enum class List2_ObisObjects {
  ObisListVersionIdentifier,
  MeterID,
  MeterType,
  ActivePowerImported,
  ActivePowerExported,
  ReactivePowerImported,
  ReactivePowerExported,
  CurrentPhaseL1,
  CurrentPhaseL2,
  CurrentPhaseL3,
  VoltagePhaseL1,
  VoltagePhaseL2,
  VoltagePhaseL3
};

enum class List3_ObisObjects {
  ObisListVersionIdentifier,
  MeterID,
  MeterType,
  ActivePowerImported,
  ActivePowerExported,
  ReactivePowerImported,
  ReactivePowerExported,
  CurrentPhaseL1,
  CurrentPhaseL2,
  CurrentPhaseL3,
  VoltagePhaseL1,
  VoltagePhaseL2,
  VoltagePhaseL3,
  ClockAndDate,
  TotalActiveEnergyImported,
  TotalActiveEnergyExported,
  TotalReactiveEnergyImported,
  TotalReactiveEnergyExported
};

#endif
