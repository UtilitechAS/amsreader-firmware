#ifndef _JSONMQTTHANDLER_H
#define _JSONMQTTHANDLER_H

#include "AmsMqttHandler.h"

class JsonMqttHandler : public AmsMqttHandler {
public:
    JsonMqttHandler(MQTTClient* mqtt, const char* clientId, const char* topic, HwTools* hw) : AmsMqttHandler(mqtt) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->hw = hw;
        this->json = (char*) malloc(BufferSize);
    };
    bool publish(AmsData* data, AmsData* previousState);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools*);

    static const uint16_t BufferSize = 768;

private:
    String clientId;
    String topic;
    HwTools* hw;
    bool init = false;
    char* json;
};
#endif
