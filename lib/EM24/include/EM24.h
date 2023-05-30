
#include "RemoteDebug.h"
#include "ModbusIP_ESP8266.h"
#include "AmsData.h"


const int HardwareVersion = 0x302;
const int FirmwareVersion = 0x304;
const int PhaseConfig = 0x1002;
const int SerialNr = 0x5000;
const int AppReg = 0xa000;
const int PhaseSequence = 0x0032;
const int IDCode = 0x000b;

const int L1U = 0x0000;
const int L2U = 0x0002;
const int L3U = 0x0004;

const int L1I = 0x000c;
const int L2I = 0x000e;
const int L3I = 0x0010;

const int L1P = 0x0012;
const int L2P = 0x0014;
const int L3P = 0x0016;
const int L1EnergyForward = 0x0040;
const int L2EnergyForward = 0x0042;
const int L3EnergyForward = 0x0044;

const int AcPower = 0x0028;
const int AcFrequency = 0x0033;
const int EnergyForward = 0x0034;
const int EnergyReverse = 0x004e;
const int SwitchPos = 0xa100;


class EM24 {
public:
    EM24(RemoteDebug*, ModbusIP* mb);
    EM24();
    void setup(ModbusIP* mbv);
    void update(AmsData*);

private:
    RemoteDebug* debugger;
    ModbusIP* mb;

};

