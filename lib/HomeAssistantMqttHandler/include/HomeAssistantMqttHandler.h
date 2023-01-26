#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    HomeAssistantMqttHandler(MQTTClient* mqtt, char* buf, const char* clientId, const char* topic, HwTools* hw) : AmsMqttHandler(mqtt, buf) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->hw = hw;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);

private:
    String haTopic = "homeassistant/sensor/";

    String haName = "AMS reader";
    #if defined(ESP32)
        String haModel = "ESP32";
    #elif defined(ESP8266)
        String haModel = "ESP8266";
    #endif
    String haManuf = "AmsToMqttBridge";

    bool autodiscoverInit = false;

    String clientId;
    String topic;
    HwTools* hw;
};
#endif
