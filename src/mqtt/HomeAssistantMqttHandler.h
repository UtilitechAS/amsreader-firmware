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
    bool publishSystem(HwTools*);

private:
    static const uint8_t sensors = 17;
    String topics[sensors] = {"/state", "/state", "/state", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/power", "/energy", "/energy", "/energy", "/energy"};
    String names[sensors] = {"Status", "Supply volt", "Temperature", "Active import", "Reactive import", "Active export", "Reactive export", "L1 current", "L2 current", "L3 current",
                                "L1 voltage", "L2 voltage", "L3 voltage", "Accumulated active import", "Accumulated active export", "Accumulated reactive import", "Accumulated reactive export"};
    String params[sensors] = {"rssi", "vcc", "temp", "P", "Q", "PO", "QO", "I1", "I2", "I3", "U1", "U2", "U3", "tPI", "tPO", "tQI", "tQO"};
    String uom[sensors] = {"dBm", "V", "C", "W", "W", "W", "W", "A", "A", "A", "V", "V", "V", "kWh", "kWh", "kWh", "kWh"};
    String devcl[sensors] = {"signal_strength", "voltage", "temperature", "power", "power", "power", "power", "current", "current", "current", "voltage", "voltage", "voltage", "energy", "energy", "energy", "energy"};
    String stacl[sensors] = {"", "", "", "measurement", "measurement", "measurement", "measurement", "", "", "", "", "", "", "total_increasing", "total_increasing", "total_increasing", "total_increasing"};

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
    uint8_t sequence = 0, listType = 0;
};
#endif
