#ifndef _PASSTHROUGHMQTTHANDLER_H
#define _PASSTHROUGHMQTTHANDLER_H

#include "AmsMqttHandler.h"

class PassthroughMqttHandler : public AmsMqttHandler {
public:
    PassthroughMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf) : AmsMqttHandler(mqttConfig, debugger, buf) {};
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);
    bool publishRaw(String data);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();
};
#endif
