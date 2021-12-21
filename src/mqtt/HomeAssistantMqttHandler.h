#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    HomeAssistantMqttHandler(MQTTClient* mqtt, const char* clientId, const char* topic, HwTools* hw) : AmsMqttHandler(mqtt) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->hw = hw;
    };
    bool publish(AmsData* data, AmsData* previousState);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools*);

private:
    String clientId;
    String topic;
    HwTools* hw;
    bool init = false;
};
#endif
