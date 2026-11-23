/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#ifndef _METERCOMMUNICATOR_H
#define _METERCOMMUNICATOR_H

#include <Arduino.h>
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "AmsData.h"
#include "AmsConfiguration.h"
#include "AmsMqttHandler.h"

class MeterCommunicator {
public:
    virtual ~MeterCommunicator() {};
    virtual void configure(MeterConfig&, Timezone*);
    virtual bool loop();
    virtual AmsData* getData(AmsData& meterState);
    virtual int getLastError();
    // Uptime millis of the last verified frame from the meter, 0 if never or unsupported
    virtual uint64_t getLastFrameMillis() { return 0; };
    virtual bool isConfigChanged();
    virtual void ackConfigChanged();
    virtual void getCurrentConfig(MeterConfig& meterConfig);
    virtual void setMqttHandlerForDebugging(AmsMqttHandler* mqttHandler) {
        this->mqttDebug = mqttHandler;
    };
    // Publishes the raw frames collected for debugging since the last call.
    // Called once a frame has been handled, so that a slow MQTT publish does not
    // stall reading the HAN port in the middle of a frame.
    virtual void publishDebugFrames() {};

protected:
    AmsMqttHandler* mqttDebug = NULL;

};

#endif
