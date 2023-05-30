#include "EM24.h"

EM24::EM24(RemoteDebug* debug, ModbusIP* mb) {
    debugger = debug;
    this->mb = mb;
    
    //debugger->flush();
    
}

void EM24::setup(ModbusIP* mb){  
    
        mb->addHreg(HardwareVersion, 0x101E);
        mb->addHreg(FirmwareVersion, 0x101E);
        mb->addHreg(PhaseConfig, 0);
        mb->addHreg(SerialNr, 0, 7);
        mb->addHreg(AppReg, 7);       
        mb->addHreg(IDCode, 1648);

        mb->addHreg(L1U, 200);
        mb->addHreg(L2U, 201);
        mb->addHreg(L3U, 202);
        mb->addHreg(L1P, 200);
        mb->addHreg(L2P, 201);
        mb->addHreg(L3P, 202);
        mb->addHreg(L1I, 0);
        mb->addHreg(L2I, 1);
        mb->addHreg(L3I, 2);
        mb->addHreg(L1EnergyForward, 0);
        mb->addHreg(L2EnergyForward, 1);
        mb->addHreg(L3EnergyForward, 2);

        mb->addHreg(AcPower, 200);
        mb->addHreg(AcFrequency, 50);
        mb->addHreg(EnergyForward, 200);
        mb->addHreg(EnergyReverse, 200);
        mb->addHreg(SwitchPos, 3);
        mb->addHreg(PhaseSequence, 0);
        
}

void EM24::update(AmsData* data) 
{
    


    debugger->printf_P(PSTR("LISTTYPE %i\r\n"), data->getListType());
    mb->Hreg(EnergyForward, data->getActiveImportCounter()/1000);
    mb->Hreg(EnergyReverse, data->getActiveExportCounter()/1000);
    
}
