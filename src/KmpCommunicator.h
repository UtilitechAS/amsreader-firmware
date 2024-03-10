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

private:
    uint64_t lastUpdate = 0;
    uint8_t reqNo = 0;

    uint8_t *hanBuffer = NULL;
    uint16_t hanBufferSize = 0;
    Stream *hanSerial;
    #if defined(ESP8266)
    SoftwareSerial *swSerial = NULL;
    #endif
    HardwareSerial *hwSerial = NULL;
    uint8_t rxBufferErrors = 0;

};
