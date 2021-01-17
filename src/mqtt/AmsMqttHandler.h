#ifndef _AMSMQTTHANDLER_H
#define _AMSMQTTHANDLER_H

#include "Arduino.h"
#include <MQTT.h>
#include "AmsData.h"
#include "AmsConfiguration.h"
#include "HwTools.h"
#include "entsoe/EntsoeApi.h"

class AmsMqttHandler {
public:
    AmsMqttHandler(MQTTClient* mqtt) {
        this->mqtt = mqtt;
    };

    virtual bool publish(AmsData* data, AmsData* previousState);
    virtual bool publishTemperatures(AmsConfiguration*, HwTools*);
    virtual bool publishPrices(EntsoeApi* eapi);
    virtual bool publishSystem(HwTools*);

protected:
    MQTTClient* mqtt;
};

#endif
