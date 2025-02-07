/**
 * @copyright Utilitech AS 2024
 * License: Fair Source
 * 
 */

#ifndef _PULSEMETERCOMMUNICATOR_H
#define _PULSEMETERCOMMUNICATOR_H

#include "MeterCommunicator.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "AmsConfiguration.h"
#include "Timezone.h"
#include "ImpulseAmsData.h"

class PulseMeterCommunicator : public MeterCommunicator  {
public:
    #if defined(AMS_REMOTE_DEBUG)
    PulseMeterCommunicator(RemoteDebug* debugger);
    #else
    PulseMeterCommunicator(Stream* debugger);
    #endif
    void configure(MeterConfig& config, Timezone* tz);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void ackConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);

    void onPulse(uint8_t pulses);

protected:
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger = NULL;
    #else
    Stream* debugger = NULL;
    #endif
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