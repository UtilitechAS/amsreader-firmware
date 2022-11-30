#ifndef _RAWMQTTHANDLER_H
#define _RAWMQTTHANDLER_H

#include "AmsMqttHandler.h"

class RawMqttHandler : public AmsMqttHandler {
public:
    RawMqttHandler(MQTTClient* mqtt, char* buf, const char* topic, bool full) : AmsMqttHandler(mqtt, buf) {
        this->topic = String(topic);
        this->full = full;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);

private:
    String topic;
    bool full;
};
#endif
