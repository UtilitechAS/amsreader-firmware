/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _HOMEASSISTANTMQTTHANDLER_H
#define _HOMEASSISTANTMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "HomeAssistantStatic.h"
#include "AmsConfiguration.h"
#include "hexutils.h"

class HomeAssistantMqttHandler : public AmsMqttHandler {
public:
    #if defined(AMS_REMOTE_DEBUG)
    HomeAssistantMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, uint8_t boardType, HomeAssistantConfig config, HwTools* hw, AmsFirmwareUpdater* updater, char* hostname) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
    #else
    HomeAssistantMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf, uint8_t boardType, HomeAssistantConfig config, HwTools* hw) : AmsMqttHandler(mqttConfig, debugger, buf) {
    #endif
        this->boardType = boardType;
        this->hw = hw;
        setHomeAssistantConfig(config, hostname);
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(uint8_t* raw, size_t length);
    bool publishFirmware();

    bool postConnect();

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

    void setHomeAssistantConfig(HomeAssistantConfig config, char* hostname);
private:
    uint8_t boardType;

    String deviceName;
    String deviceModel;
    String deviceUid;
    String manufacturer;
    String deviceUrl;

    String statusTopic;
    String sensorTopic;
    String updateTopic;
    String sensorNamePrefix;

    bool l1Init, l2Init, l2eInit, l3Init, l3eInit, l4Init, l4eInit, rtInit, rteInit, pInit, sInit, rInit, fInit, dInit;
    bool tInit[32] = {false};
    uint8_t priceImportInit = 0, priceExportInit = 0;
    uint32_t lastThresholdPublish = 0;

    HwTools* hw;

    bool publishList1(AmsData* data, EnergyAccounting* ea);
    bool publishList2(AmsData* data, EnergyAccounting* ea);
    bool publishList3(AmsData* data, EnergyAccounting* ea);
    bool publishList4(AmsData* data, EnergyAccounting* ea);
    String getMeterModel(AmsData* data);
    bool publishRealtime(AmsData* data, EnergyAccounting* ea, PriceService* ps);
    void publishSensor(const HomeAssistantSensor sensor);
    void publishList1Sensors();
    void publishList1ExportSensors();
    void publishList2Sensors();
    void publishList2ExportSensors();
    void publishList3Sensors();
    void publishList3ExportSensors();
    void publishList4Sensors();
    void publishList4ExportSensors();
    void publishRealtimeSensors(EnergyAccounting* ea, PriceService* ps);
    void publishRealtimeExportSensors(EnergyAccounting* ea, PriceService* ps);
    void publishTemperatureSensor(uint8_t index, String id);
    void publishPriceSensors(PriceService* ps);
    void publishSystemSensors();
    void publishThresholdSensors();

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
            case 80:
                return F("ESP32-S3");
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
