#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "HomeAssistantStatic.h"
#include "AmsConfiguration.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    HomeAssistantMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, uint8_t boardType, HomeAssistantConfig config, HwTools* hw) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->hw = hw;

        l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = false;

        topic = String(mqttConfig.publishTopic);

        if(strlen(config.discoveryNameTag) > 0) {
            snprintf_P(buf, 128, PSTR("AMS reader (%s)"), config.discoveryNameTag);
            deviceName = String(buf);
            snprintf_P(buf, 128, PSTR("[%s] "), config.discoveryNameTag);
            sensorNamePrefix = String(buf);
        } else {
            deviceName = F("AMS reader");
            sensorNamePrefix = "";
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
            snprintf_P(json, 128, PSTR("%s/status"), config.discoveryPrefix);
            statusTopic = String(buf);

            snprintf_P(buf, 128, PSTR("%s/sensor/"), config.discoveryPrefix);
            discoveryTopic = String(buf);
        } else {
            statusTopic = F("homeassistant/status");
            discoveryTopic = F("homeassistant/sensor/");
        }
        strcpy(this->mqttConfig.subscribeTopic, statusTopic.c_str());
    };

    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);
    bool publishRaw(String data);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    String topic;

    String deviceName;
    String deviceModel;
    String deviceUid;
    String manufacturer;
    String deviceUrl;

    String statusTopic;
    String discoveryTopic;
    String sensorNamePrefix;

    bool l1Init, l2Init, l2eInit, l3Init, l3eInit, l4Init, l4eInit, rtInit, rteInit, pInit, sInit;
    bool tInit[32] = {false};
    bool prInit[38] = {false};

    HwTools* hw;

    bool publishList1(AmsData* data, EnergyAccounting* ea);
    bool publishList2(AmsData* data, EnergyAccounting* ea);
    bool publishList3(AmsData* data, EnergyAccounting* ea);
    bool publishList4(AmsData* data, EnergyAccounting* ea);
    String getMeterModel(AmsData* data);
    bool publishRealtime(AmsData* data, EnergyAccounting* ea, EntsoeApi* eapi);
    void publishSensor(const HomeAssistantSensor sensor);
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

    String boardTypeToString(uint8_t b) {
        switch(b) {
            case 5:
                #if defined(ESP8266)
                    return F("Pow-K");
                #elif defined(ESP32)
                    return F("Pow-K+");
                #endif
            case 7:
                #if defined(ESP8266)
                    return F("Pow-U");
                #elif defined(ESP32)
                    return F("Pow-U+");
                #endif
            case 6:
                return F("Pow-P1");
            case 51:
                return F("S2 mini");
            case 50:
                return F("ESP32-S2");
            case 201:
                return F("LOLIN D32");
            case 202:
                return F("HUZZAH32");
            case 203:
                return F("DevKitC");
            case 200:
                return F("ESP32");
            case 2:
                return F("HAN Reader 2.0 by Max Spencer");
            case 0:
                return F("Custom hardware by Roar Fredriksen");
            case 1:
                return F("Kamstrup module by Egil Opsahl");
            case 3:
                return F("Pow-K");
            case 4:
                return F("Pow-U");
            case 101:
                return F("D1 mini");
            case 100:
                return F("ESP8266");
            case 70:
                return F("ESP32-C3");
            case 71:
                return F("ESP32-C3-DevKitM-1");
        }
        #if defined(ESP8266)
            return F("ESP8266");
        #elif defined(ESP32)
            return F("ESP32");
        #endif
    };

    String boardManufacturerToString(uint8_t b) {
        if(b >= 3 && b <= 7)
            return F("amsleser.no");
        if(b < 50)
            return F("Custom");
        switch(b) {
            case 51:
            case 101:
            case 201:
                return F("Wemos");
            case 202:
                return F("Adafruit");
        }
        return F("Espressif");
    };
};
#endif
