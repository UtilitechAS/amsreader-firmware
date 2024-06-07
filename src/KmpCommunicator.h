/**
 * @copyright Utilitech AS 2024
 * License: Fair Source
 * 
 */

#pragma once

#include "PassiveMeterCommunicator.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "AmsConfiguration.h"
#include "Timezone.h"
#include "ImpulseAmsData.h"

#if defined(ESP8266)
#include "SoftwareSerial.h"
#endif

class KmpCommunicator : public PassiveMeterCommunicator  {
public:
    #if defined(AMS_REMOTE_DEBUG)
    KmpCommunicator(RemoteDebug* debugger) : PassiveMeterCommunicator(debugger) {};
    #else
    KmpCommunicator(Stream* debugger) : PassiveMeterCommunicator(debugger) {};
    #endif
    void configure(MeterConfig&, Timezone*);
    bool loop();
    AmsData* getData(AmsData& meterState);

private:
    uint64_t lastUpdate = 0;
    uint8_t batch = 0;
    AmsData state;

    bool readPacket();
    int16_t unwrapData(uint8_t *buf, DataParserContext &context);
    uint8_t stuff(uint8_t* buf, uint8_t len);
    uint8_t unstuff(uint8_t* buf, uint8_t len);
    void send(uint8_t* buf, uint8_t len);
    double convertvalue(uint32_t val, uint8_t unit, uint8_t siex);
};
