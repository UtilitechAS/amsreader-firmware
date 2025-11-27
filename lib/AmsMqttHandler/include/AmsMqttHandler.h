/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _AMSMQTTHANDLER_H
#define _AMSMQTTHANDLER_H

#include "Arduino.h"
#include <MQTT.h>
#include "AmsData.h"
#include "AmsConfiguration.h"
#include "EnergyAccounting.h"
#include "HwTools.h"
#include "PriceService.h"
#include "AmsFirmwareUpdater.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

class AmsMqttHandler {
public:
    #if defined(AMS_REMOTE_DEBUG)
    AmsMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #else
    AmsMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf) {
    #endif
        this->mqttConfig = mqttConfig;
    	this->mqttConfigChanged = true;
        this->debugger = debugger;
        this->json = buf;
        this->updater = updater;
        mqtt.dropOverflow(true);

        pubTopic = String(mqttConfig.publishTopic);
        subTopic = String(mqttConfig.subscribeTopic);
        if(subTopic.isEmpty()) subTopic = pubTopic+"/command";
    };

    void setCaVerification(bool);
    void setConfig(MqttConfig& mqttConfig);

    bool connect();
    bool defaultSubscribe();
    void disconnect();
    lwmqtt_err_t lastError();
    bool connected();
    bool loop();
    bool isRebootSuggested();

    virtual uint8_t getFormat() { return 0; };

    virtual bool postConnect() { return false; };
    virtual bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) { return false; };
    virtual bool publishTemperatures(AmsConfiguration*, HwTools*) { return false; };
    virtual bool publishPrices(PriceService* ps) { return false; };
    virtual bool publishSystem(HwTools*, PriceService*, EnergyAccounting*) { return false; };
    virtual bool publishRaw(String data) { return false; };
    virtual bool publishFirmware() { return false; };
    virtual void onMessage(String &topic, String &payload) {};

    virtual ~AmsMqttHandler() {
        if(mqttClient != NULL) {
            mqttClient->stop();
            delete mqttClient;
        }
    };

protected:
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
    #else
    Stream* debugger;
    #endif
    MqttConfig mqttConfig;
    bool mqttConfigChanged = true;
    MQTTClient mqtt = MQTTClient(256);
    unsigned long lastMqttRetry = -10000;
    bool caVerification = true;
    WiFiClient *mqttClient = NULL;
    WiFiClientSecure *mqttSecureClient = NULL;
    char* json;
    uint16_t BufferSize = 2048;
    uint64_t lastStateUpdate = 0;
    uint64_t lastSuccessfulLoop = 0;

    String pubTopic;
    String subTopic;

    AmsFirmwareUpdater* updater = NULL;
    bool rebootSuggested = false;
};

#endif
