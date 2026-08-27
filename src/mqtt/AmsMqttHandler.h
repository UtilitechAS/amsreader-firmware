/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#ifndef _AMSMQTTHANDLER_H
#define _AMSMQTTHANDLER_H

#include "Arduino.h"
#include <MQTT.h>
#include "AmsData.h"
#include "AmsDataStorage.h"
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
    AmsMqttHandler(AmsConfiguration* config, RemoteDebug* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #else
    AmsMqttHandler(AmsConfiguration* config, Stream* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #endif
        this->config = config;
        config->getMqttConfig(mqttConfig);
        mqttConfigChanged = true;
        init(debugger, buf, updater);
    };
    #if defined(AMS_REMOTE_DEBUG)
    AmsMqttHandler(MqttConfig& MqttConfig, RemoteDebug* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #else
    AmsMqttHandler(MqttConfig& MqttConfig, Stream* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #endif
        this->mqttConfig = MqttConfig;
        init(debugger, buf, updater);
    }

    void setCaVerification(bool);
    void setConfig(MqttConfig& mqttConfig);
    void setResetDataContainer(ResetDataContainer* rdc) { this->rdc = rdc; }
    void setDataStorage(AmsDataStorage* ds) { this->ds = ds; }

    bool connect();
    bool defaultSubscribe();
    void disconnect();
    lwmqtt_err_t lastError();
    bool connected();
    bool loop();
    bool isRebootSuggested();

    // Service state and event announcements (#1128). Shared by every payload
    // format: both go to sub-topics of the configured publish topic, so they sit
    // alongside the data payload rather than replacing it. <topic>/status is
    // already the retained online/offline availability payload backed by the
    // last will, so the state snapshot uses <topic>/services.
    //
    // publishServices() is retained: a subscriber that connects late still sees
    // the current state. publishEvent() is not - an event describes a moment,
    // and a retained one would replay as current on every reconnect.
    bool publishServices(const char* payload);
    bool publishEvent(const char* event, const char* fields);

    virtual uint8_t getFormat() { return 0; };

    virtual bool postConnect() { return false; };
    virtual bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) { return false; };
    virtual bool publishTemperatures(AmsConfiguration*, HwTools*) { return false; };
    virtual bool publishPrices(PriceService* ps) { return false; };
    virtual bool publishSystem(HwTools*, PriceService*, EnergyAccounting*) { return false; };
    virtual bool publishRaw(uint8_t* raw, size_t length) { return false; };
    virtual bool publishFirmware() { return false; };
    virtual void onMessage(String &topic, String &payload) {};

    virtual ~AmsMqttHandler() {
        if(mqttSecureClient != NULL) {
            mqttSecureClient->stop();
            delete mqttSecureClient;
        }

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
    AmsConfiguration* config;
    MqttConfig mqttConfig;
    bool mqttConfigChanged = true;
    #if defined(ESP32)
    MQTTClient mqtt = MQTTClient(2048);
    #else
    MQTTClient mqtt = MQTTClient(256);
    #endif
    unsigned long lastMqttRetry = -10000;
    bool caVerification = true;
    WiFiClient *mqttClient = NULL;
    WiFiClientSecure *mqttSecureClient = NULL;
    boolean _connected = false;
    char* json;
    uint64_t lastStateUpdate = 0;
    uint64_t lastSuccessfulLoop = 0;

    char pubTopic[64];
    char subTopic[64];

    AmsFirmwareUpdater* updater = NULL;
    bool rebootSuggested = false;
    ResetDataContainer* rdc = NULL;
    AmsDataStorage* ds = NULL;

    // Generic device commands shared by all payload handlers (fwupgrade / reboot /
    // factoryreset). Returns true if the message was a recognised command.
    bool handleCommand(String &topic, String &payload);
    void rebootDevice(uint8_t cause);

private:
    #if defined(AMS_REMOTE_DEBUG)
    void init(RemoteDebug* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #else
    void init(Stream* debugger, char* buf, AmsFirmwareUpdater* updater) {
    #endif
        this->debugger = debugger;
        this->json = buf;
        this->updater = updater;
        mqtt.dropOverflow(true);

        strncpy(pubTopic, mqttConfig.publishTopic, sizeof(pubTopic) - 1);
        pubTopic[sizeof(pubTopic) - 1] = '\0';
        if(strlen(mqttConfig.subscribeTopic) > 0) {
            strncpy(subTopic, mqttConfig.subscribeTopic, sizeof(subTopic) - 1);
            subTopic[sizeof(subTopic) - 1] = '\0';
        } else {
            snprintf(subTopic, sizeof(subTopic), "%s/command", mqttConfig.publishTopic);
        }
    }
};

#endif
