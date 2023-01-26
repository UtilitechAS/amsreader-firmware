#ifndef _JSONMQTTHANDLER_H
#define _JSONMQTTHANDLER_H

#include "AmsMqttHandler.h"

class JsonMqttHandler : public AmsMqttHandler {
public:
    JsonMqttHandler(MQTTClient* mqtt, char* buf, const char* clientId, const char* topic, HwTools* hw) : AmsMqttHandler(mqtt, buf) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->hw = hw;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);

private:
    String clientId;
    String topic;
    HwTools* hw;
};
#endif
