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

        mb->addHreg(L1U, 0, 2);
        mb->addHreg(L2U, 0, 2);
        mb->addHreg(L3U, 0, 2);
        mb->addHreg(L1P, 0, 2);
        mb->addHreg(L2P, 0, 2);
        mb->addHreg(L3P, 0, 2);
        mb->addHreg(L1I, 0, 2);
        mb->addHreg(L2I, 0, 2);
        mb->addHreg(L3I, 0, 2);
        mb->addHreg(L1EnergyForward, 0, 2);
        mb->addHreg(L2EnergyForward, 0, 2);
        mb->addHreg(L3EnergyForward, 0, 2);
        
        mb->addHreg(AcPower, 0, 2);
        mb->addHreg(AcFrequency, 50);
        mb->addHreg(EnergyForward, 0, 2);
        mb->addHreg(EnergyReverse, 0, 2);
        mb->addHreg(SwitchPos, 3);
        mb->addHreg(PhaseSequence, 0);

        Set32bitReg(AcPower, (int32)30000);
        Set32bitReg(EnergyForward, (int32)50000);
        Set32bitReg(EnergyReverse, (int32)-10000);

         Set32bitReg(L1P, (int32)10000);         
         Set32bitReg(L3P, (int32)10000);

         Set32bitReg(L2I, (int32)10000);


}

void EM24::Set32bitReg(uint16_t reg, int32 value)
{
    mb->Hreg(reg, value & 0xffff);
    mb->Hreg(reg+1, (value >> 16) & 0xffff);

}

void EM24::update(AmsData* data) 
{
    


    debugger->printf_P(PSTR("LISTTYPE %i\r\n"), data->getListType());
    mb->Hreg(EnergyForward, data->getActiveImportCounter()/1000);
    mb->Hreg(EnergyReverse, data->getActiveExportCounter()/1000);
    
}
