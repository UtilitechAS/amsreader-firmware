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
    #if defined(AMS_REMOTE_DEBUG)
    RawMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
        full = mqttConfig.payloadFormat == 2;
        topic = String(mqttConfig.publishTopic);
    };
    #else
    RawMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
        full = mqttConfig.payloadFormat == 2;
        topic = String(mqttConfig.publishTopic);
    };
    #endif
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(uint8_t* raw, size_t length);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    bool full;
    String topic;
    uint32_t lastThresholdPublish = 0;
    bool hasExport = false;

    bool publishList1(AmsData* data, AmsData* meterState);
    bool publishList2(AmsData* data, AmsData* meterState);
    bool publishList3(AmsData* data, AmsData* meterState);
    bool publishList4(AmsData* data, AmsData* meterState);
    bool publishRealtime(EnergyAccounting* ea);
};
#endif
