/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _DOMOTICZMQTTHANDLER_H
#define _DOMOTICZMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "AmsConfiguration.h"

class DomoticzMqttHandler : public AmsMqttHandler {
public:
    #if defined(AMS_REMOTE_DEBUG)
    DomoticzMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, DomoticzConfig config, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
        this->config = config;
    };
    #else
    DomoticzMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf, DomoticzConfig config) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->config = config;
    };
    #endif
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(uint8_t* raw, size_t length);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

    void setDomoticzConfig(DomoticzConfig config) {
        this->config = config;
    }

private:
    DomoticzConfig config;
    double energy = 0.0;
};

#endif
