#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "HomeAssistantStatic.h"
#include "AmsConfiguration.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    HomeAssistantMqttHandler(MQTTClient* mqtt, char* buf, const char* clientId, const char* topic, uint8_t boardType, HomeAssistantConfig config, HwTools* hw) : AmsMqttHandler(mqtt, buf) {
        this->clientId = clientId;
        this->topic = String(topic);
        this->config = config;
        this->hw = hw;
        l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = false;

        if(strlen(config.discoveryNameTag) > 0) {
            snprintf_P(buf, 128, PSTR("AMS reader (%s)"), config.discoveryNameTag);
            deviceName = String(buf);
            snprintf_P(buf, 128, PSTR(" (%s)"), config.discoveryNameTag);
            sensorNamePostFix = String(buf);
        } else {
            deviceName = "AMS reader";
            sensorNamePostFix = "";
        }
        deviceModel = boardTypeToString(boardType);
        manufacturer = boardManufacturerToString(boardType);

        #if defined(ESP8266)
            String hostname = WiFi.hostname();
        #elif defined(ESP32)
            String hostname = WiFi.getHostname();
        #endif

        deviceUid = hostname; // Maybe configurable in the future?

        if(strlen(config.discoveryHostname) > 0) {
            if(strncmp_P(config.discoveryHostname, PSTR("http"), 4) == 0) {
                deviceUrl = String(config.discoveryHostname);
            } else {
                snprintf_P(buf, 128, PSTR("http://%s/"), config.discoveryHostname);
                deviceUrl = String(buf);
            }
        } else {
            snprintf_P(buf, 128, PSTR("http://%s.local/"), hostname);
            deviceUrl = String(buf);
        }

        if(strlen(config.discoveryPrefix) > 0) {
            snprintf_P(buf, 128, PSTR("%s/sensor/"), config.discoveryPrefix);
            discoveryTopic = String(buf);
        } else {
            discoveryTopic = "homeassistant/sensor/";
        }
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
    HomeAssistantConfig config;
    String deviceName;
    String deviceModel;
    String deviceUid;
    String manufacturer;
    String deviceUrl;

    String discoveryTopic;
    String sensorNamePostFix;

    bool l1Init, l2Init, l2eInit, l3Init, l3eInit, l4Init, l4eInit, rtInit, rteInit, pInit, sInit;
    bool tInit[32] = {false};
    bool prInit[38] = {false};

    String clientId;
    String topic;
    HwTools* hw;

    String boardTypeToString(uint8_t b) {
        switch(b) {
            case 5:
                #if defined(ESP8266)
                    return "Pow-K";
                #elif defined(ESP32)
                    return "Pow-K+";
                #endif
            case 7:
                #if defined(ESP8266)
                    return "Pow-U";
                #elif defined(ESP32)
                    return "Pow-U+";
                #endif
            case 6:
                return "Pow-P1";
            case 51:
                return "S2 mini";
            case 50:
                return "ESP32-S2";
            case 201:
                return "LOLIN D32";
            case 202:
                return "HUZZAH32";
            case 203:
                return "DevKitC";
            case 200:
                return "ESP32";
            case 2:
                return "HAN Reader 2.0 by Max Spencer";
            case 0:
                return "Custom hardware by Roar Fredriksen";
            case 1:
                return "Kamstrup module by Egil Opsahl";
            case 3:
                return "Pow-K";
            case 4:
                return "Pow-U";
            case 101:
                return "D1 mini";
            case 100:
                return "ESP8266";
            case 70:
                return "ESP32-C3";
            case 71:
                return "ESP32-C3-DevKitM-1";
        }
        #if defined(ESP8266)
            return "ESP8266";
        #elif defined(ESP32)
            return "ESP32";
        #endif
    };

    String boardManufacturerToString(uint8_t b) {
        if(b >= 3 && b <= 7)
            return "amsleser.no";
        if(b < 50)
            return "Custom";
        switch(b) {
            case 51:
            case 101:
            case 201:
                return "Wemos";
            case 202:
                return "Adafruit";
        }
        return "Espressif";
    };
};
#endif
