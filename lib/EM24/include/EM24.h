
#include "RemoteDebug.h"
#include "ModbusIP_ESP8266.h"


const int EM24_SERIAL = 50;



class EM24 {
public:
    EM24(RemoteDebug*, ModbusIP* mb);
    EM24();
    void setup(ModbusIP* mbv);
    void update();

private:
    RemoteDebug* debugger;
    ModbusIP* mb;

};

