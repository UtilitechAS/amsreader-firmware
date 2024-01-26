#include "ImpulseAmsData.h"
#include "Uptime.h"

ImpulseAmsData::ImpulseAmsData(AmsData& state, uint16_t pulsePerKwh, uint8_t pulses) {
    listType = 1;
    if(pulses > 0) {
        lastUpdateMillis = millis64();
        uint64_t lastStateMillis = state.getLastUpdateMillis();
        if(lastStateMillis > 0) {
            uint64_t ms = (lastUpdateMillis - lastStateMillis) / pulses;
            activeImportPower = (1000.0 / pulsePerKwh) / (((float) ms) / 3600000.0);
        }
    } else {
        lastUpdateMillis = state.getLastUpdateMillis();
    }
}

ImpulseAmsData::ImpulseAmsData(double activeImportCounter) {
    this->activeImportCounter = activeImportCounter;
    this->listType = 3;
}