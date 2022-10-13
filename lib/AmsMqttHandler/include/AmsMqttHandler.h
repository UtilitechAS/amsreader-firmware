#ifndef _AMSMQTTHANDLER_H
#define _AMSMQTTHANDLER_H

#include "Arduino.h"
#include <MQTT.h>
#include "AmsData.h"
#include "AmsConfiguration.h"
#include "EnergyAccounting.h"
#include "HwTools.h"
#include "EntsoeApi.h"

class AmsMqttHandler {
public:
    AmsMqttHandler(MQTTClient* mqtt, char* buf) {
        this->mqtt = mqtt;
        this->json = buf;
    };
    virtual ~AmsMqttHandler() {};

    virtual bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea);
    virtual bool publishTemperatures(AmsConfiguration*, HwTools*);
    virtual bool publishPrices(EntsoeApi* eapi);
    virtual bool publishSystem(HwTools*, EntsoeApi*, EnergyAccounting*);

protected:
    MQTTClient* mqtt;
    char* json;
    uint16_t BufferSize = 1024;
};

#endif
