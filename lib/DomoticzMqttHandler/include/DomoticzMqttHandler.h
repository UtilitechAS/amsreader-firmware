#ifndef _DOMOTICZMQTTHANDLER_H
#define _DOMOTICZMQTTHANDLER_H

#include "AmsMqttHandler.h"
#include "AmsConfiguration.h"

class DomoticzMqttHandler : public AmsMqttHandler {
public:
    DomoticzMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, DomoticzConfig config) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->config = config;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);
    bool publishRaw(String data);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    DomoticzConfig config;
    double energy = 0.0;
};

#endif
