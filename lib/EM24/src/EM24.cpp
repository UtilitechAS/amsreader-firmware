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

        //Set32bitReg(AcPower, (int32)30000);
        //Set32bitReg(EnergyForward, (int32)50000);
        //Set32bitReg(EnergyReverse, (int32)-10000);

         //Set32bitReg(L1P, (int32)10000);         
         //Set32bitReg(L3P, (int32)10000);

         //Set32bitReg(L2I, (int32)10000);


}

void EM24::Set32bitReg(uint16_t reg, int32 value)
{
    mb->Hreg(reg, value & 0xffff);
    mb->Hreg(reg+1, (value >> 16) & 0xffff);

}

void EM24::update(AmsData* data) 
{
    debugger->printf_P(PSTR("LISTTYPE %i\r\n"), data->getListType());
    if(data->getListType() >= 3)
    {
        int32 energyForward = (int32)(data->getActiveImportCounter()*10);
        Set32bitReg(EnergyForward, energyForward);
        Set32bitReg(EnergyReverse, (int32)(data->getActiveExportCounter()*10));
        Set32bitReg(L1EnergyForward, energyForward/3);
        Set32bitReg(L2EnergyForward, energyForward/3);
        Set32bitReg(L3EnergyForward, energyForward/3);        
    }
    if(data->getListType() <= 3)
    {    
        int32 ActivePower =  (int32)(data->getActiveImportPower() - data->getActiveExportPower())*10;        
        Set32bitReg(AcPower, ActivePower);
        Set32bitReg(L1P, ActivePower/3);
        Set32bitReg(L2P, ActivePower/3);
        Set32bitReg(L3P, ActivePower/3);
        Set32bitReg(L1U, data->getL1Voltage()*10);
        Set32bitReg(L2U, data->getL2Voltage()*10);
        Set32bitReg(L3U, data->getL3Voltage()*10);
        Set32bitReg(L1I, data->getL1Current()*1000);
        Set32bitReg(L2I, data->getL2Current()*1000);
        Set32bitReg(L3I, data->getL3Current()*1000);
    }
    else if(data->getListType() == 4) 
    {
        int32 ActivePower =  (int32)(data->getActiveImportPower() - data->getActiveExportPower())*10;        
        Set32bitReg(AcPower, ActivePower);
        Set32bitReg(L1P, (((int32)data->getL1ActiveImportPower())-((int32)data->getL1ActiveExportPower()))*10);
        Set32bitReg(L2P, (((int32)data->getL2ActiveImportPower())-((int32)data->getL2ActiveExportPower()))*10);
        Set32bitReg(L3P, (((int32)data->getL3ActiveImportPower())-((int32)data->getL3ActiveExportPower()))*10);
        Set32bitReg(L1U, data->getL1Voltage()*10);
        Set32bitReg(L2U, data->getL2Voltage()*10);
        Set32bitReg(L3U, data->getL3Voltage()*10);
        Set32bitReg(L1I, data->getL1Current()*1000);
        Set32bitReg(L2I, data->getL2Current()*1000);
        Set32bitReg(L3I, data->getL3Current()*1000);
    }

    
}
