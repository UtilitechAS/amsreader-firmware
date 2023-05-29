#include "EM24.h"

EM24::EM24(RemoteDebug* debug, ModbusIP* mb) {
    debugger = debug;
    this->mb = mb;
    
    //debugger->flush();
    
}

EM24::EM24()
{}

void EM24::setup(ModbusIP* mbv){     
     mb->addHreg(EM24_SERIAL);
}

void EM24::update() {
    mb->Hreg(EM24_SERIAL, 234);
    
}
