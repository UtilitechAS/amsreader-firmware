/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PASSTHROUGHMQTTHANDLER_H
#define _PASSTHROUGHMQTTHANDLER_H

#include "AmsMqttHandler.h"

class PassthroughMqttHandler : public AmsMqttHandler {
public:
    PassthroughMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->topic = String(mqttConfig.publishTopic);
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);
    bool publishBytes(uint8_t* buf, uint16_t len);
    bool publishString(char* str);

private:
    String topic;
    uint8_t getFormat();
};
#endif
