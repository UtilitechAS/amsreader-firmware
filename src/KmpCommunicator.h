/**
 * @copyright Utilitech AS 2024
 * License: Fair Source
 * 
 */

#pragma once

#include "MeterCommunicator.h"
#include "RemoteDebug.h"
#include "AmsConfiguration.h"
#include "Timezone.h"
#include "ImpulseAmsData.h"

class KmpCommunicator : public MeterCommunicator  {
public:
    KmpCommunicator(RemoteDebug* debugger);
    void configure(MeterConfig& config, Timezone* tz);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);

    HardwareSerial* getHwSerial();
    void rxerr(int err);
};
