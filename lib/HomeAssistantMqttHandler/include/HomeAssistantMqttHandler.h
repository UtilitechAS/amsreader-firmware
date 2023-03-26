#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "HomeAssistantStatic.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    HomeAssistantMqttHandler(MQTTClient* mqtt, char* buf, const char* clientId, const char* topic, HwTools* hw) : AmsMqttHandler(mqtt, buf) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->hw = hw;
        l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = false;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);

    void publishSensor(const HomeAssistantSensor& sensor);
    void publishList1Sensors();
    void publishList1ExportSensors();
    void publishList2Sensors();
    void publishList2ExportSensors();
    void publishList3Sensors();
    void publishList3ExportSensors();
    void publishList4Sensors();
    void publishList4ExportSensors();
    void publishRealtimeSensors(EnergyAccounting* ea, EntsoeApi* eapi);
    void publishRealtimeExportSensors(EnergyAccounting* ea, EntsoeApi* eapi);
    void publishTemperatureSensor(uint8_t index, String id);
    void publishPriceSensors(EntsoeApi* eapi);
    void publishSystemSensors();

private:
    String haTopic = "homeassistant/sensor/";

    String haName = "AMS reader";
    #if defined(ESP32)
        String haModel = "ESP32";
    #elif defined(ESP8266)
        String haModel = "ESP8266";
    #endif
    String haManuf = "AmsToMqttBridge";

    bool l1Init, l2Init, l2eInit, l3Init, l3eInit, l4Init, l4eInit, rtInit, rteInit, pInit, sInit;
    bool tInit[32] = {false};
    bool prInit[38] = {false};

    String clientId;
    String topic;
    HwTools* hw;
};
#endif
