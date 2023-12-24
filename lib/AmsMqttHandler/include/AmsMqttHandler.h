#ifndef _AMSMQTTHANDLER_H
#define _AMSMQTTHANDLER_H

#include "Arduino.h"
#include <MQTT.h>
#include "AmsData.h"
#include "AmsConfiguration.h"
#include "EnergyAccounting.h"
#include "HwTools.h"
#include "EntsoeApi.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

class AmsMqttHandler {
public:
    AmsMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf) {
        this->mqttConfig = mqttConfig;
    	this->mqttConfigChanged = true;
        this->debugger = debugger;
        this->json = buf;
        mqtt.dropOverflow(true);
    };

    void setCaVerification(bool);
    void setConfig(MqttConfig& mqttConfig);

    bool connect();
    void disconnect();
    lwmqtt_err_t lastError();
    bool connected();
    bool loop();

    virtual uint8_t getFormat() { return 0; };

    virtual bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi) { return false; };
    virtual bool publishTemperatures(AmsConfiguration*, HwTools*) { return false; };
    virtual bool publishPrices(EntsoeApi* eapi) { return false; };
    virtual bool publishSystem(HwTools*, EntsoeApi*, EnergyAccounting*) { return false; };
    virtual bool publishRaw(String data) { return false; };
    virtual void onMessage(String &topic, String &payload) {};

    virtual ~AmsMqttHandler() {
        if(mqttClient != NULL) {
            mqttClient->stop();
            delete mqttClient;
        }
    };

protected:
    RemoteDebug* debugger;
    MqttConfig mqttConfig;
    bool mqttConfigChanged = true;
    MQTTClient mqtt = MQTTClient(256);
    unsigned long lastMqttRetry = -10000;
    bool caVerification = true;
    WiFiClient *mqttClient = NULL;
    WiFiClientSecure *mqttSecureClient = NULL;
    char* json;
    uint16_t BufferSize = 2048;
};

#endif
