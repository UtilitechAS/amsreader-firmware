#ifndef _DOMOTICZMQTTHANDLER_H
#define _DOMOTICZMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "AmsConfiguration.h"

class DomoticzMqttHandler : public AmsMqttHandler {
public:
    DomoticzMqttHandler(MQTTClient* mqtt, DomoticzConfig config) : AmsMqttHandler(mqtt) {
        this->config = config;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools*);

private:
    DomoticzConfig config;
    int energy = 0.0;
};
#endif
