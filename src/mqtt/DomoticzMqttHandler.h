#ifndef _DOMOTICZMQTTHANDLER_H
#define _DOMOTICZMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "AmsConfiguration.h"

class DomoticzMqttHandler : public AmsMqttHandler {
public:
    DomoticzMqttHandler(MQTTClient* mqtt, DomoticzConfig config) : AmsMqttHandler(mqtt) {
        this->config = config;
        this->json = (char*) malloc(BufferSize);
    };
    bool publish(AmsData* data, AmsData* previousState);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools*);

    static const uint16_t BufferSize = 192;

private:
    DomoticzConfig config;
    int energy = 0.0;
    char* json;
};

#endif
