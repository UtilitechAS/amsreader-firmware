/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _RAWMQTTHANDLER_H
#define _RAWMQTTHANDLER_H

#include "AmsMqttHandler.h"

class RawMqttHandler : public AmsMqttHandler {
public:
    RawMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf) : AmsMqttHandler(mqttConfig, debugger, buf) {
        full = mqttConfig.payloadFormat == 2;
        topic = String(mqttConfig.publishTopic);
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(String data);

    uint8_t getFormat();

private:
    bool full;
    String topic;

    bool publishList1(AmsData* data, AmsData* meterState);
    bool publishList2(AmsData* data, AmsData* meterState);
    bool publishList3(AmsData* data, AmsData* meterState);
    bool publishList4(AmsData* data, AmsData* meterState);
    bool publishRealtime(EnergyAccounting* ea);
};
#endif
