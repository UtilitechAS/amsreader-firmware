/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _JSONMQTTHANDLER_H
#define _JSONMQTTHANDLER_H

#include "AmsMqttHandler.h"

class JsonMqttHandler : public AmsMqttHandler {
public:
    #if defined(AMS_REMOTE_DEBUG)
    JsonMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, HwTools* hw, AmsDataStorage* ds, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
    #else
    JsonMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf, HwTools* hw, AmsDataStorage* ds, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
    #endif
        this->hw = hw;
        this->ds = ds;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(String data);
    bool publishFirmware();

    bool postConnect();

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    HwTools* hw;
    AmsDataStorage* ds;

    uint16_t appendJsonHeader(AmsData* data);
    uint16_t appendJsonFooter(EnergyAccounting* ea, uint16_t pos);
    bool publishList1(AmsData* data, EnergyAccounting* ea);
    bool publishList2(AmsData* data, EnergyAccounting* ea);
    bool publishList3(AmsData* data, EnergyAccounting* ea);
    bool publishList4(AmsData* data, EnergyAccounting* ea);
    String getMeterModel(AmsData* data);
};
#endif
