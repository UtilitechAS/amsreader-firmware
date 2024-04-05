/**
 * @copyright Utilitech AS 2024
 * License: Fair Source
 * 
 */

#ifndef _PULSEMETERCOMMUNICATOR_H
#define _PULSEMETERCOMMUNICATOR_H

#include "MeterCommunicator.h"
#include "RemoteDebug.h"
#include "AmsConfiguration.h"
#include "Timezone.h"
#include "ImpulseAmsData.h"

class PulseMeterCommunicator : public MeterCommunicator  {
public:
    PulseMeterCommunicator(RemoteDebug* debugger);
    void configure(MeterConfig& config, Timezone* tz);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);

    void onPulse(uint8_t pulses);

protected:
    RemoteDebug* debugger = NULL;
    MeterConfig meterConfig;
    bool configChanged = false;
    Timezone* tz;
    bool updated = false;
    bool initialized = false;
    AmsData state;
    uint64_t lastUpdate = 0;

    void setupGpio();
};

#endif