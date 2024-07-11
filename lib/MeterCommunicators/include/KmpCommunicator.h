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

#include "KmpTalker.h"

class KmpCommunicator : public PassiveMeterCommunicator  {
public:
    #if defined(AMS_REMOTE_DEBUG)
    KmpCommunicator(RemoteDebug* debugger) : PassiveMeterCommunicator(debugger) {};
    #else
    KmpCommunicator(Stream* debugger) : PassiveMeterCommunicator(debugger) {};
    #endif
    void configure(MeterConfig&);
    bool loop();
    AmsData* getData(AmsData& meterState);
private:
    KmpTalker* talker = NULL;
};
