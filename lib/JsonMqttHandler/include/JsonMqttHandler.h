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
    JsonMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, HwTools* hw, AmsFirmwareUpdater* updater) : AmsMqttHandler(mqttConfig, debugger, buf, updater) {
        this->hw = hw;
    };
    #else
    JsonMqttHandler(MqttConfig& mqttConfig, Stream* debugger, char* buf, HwTools* hw) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->hw = hw;
    };
    #endif
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, PriceService* ps);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(PriceService*);
    bool publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea);
    bool publishRaw(String data);
    bool publishFirmware();

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    HwTools* hw;
    bool hasExport = false;
    uint16_t appendJsonHeader(AmsData* data);
    uint16_t appendJsonFooter(EnergyAccounting* ea, uint16_t pos);
    bool publishList1(AmsData* data, EnergyAccounting* ea);
    bool publishList2(AmsData* data, EnergyAccounting* ea);
    bool publishList3(AmsData* data, EnergyAccounting* ea);
    bool publishList4(AmsData* data, EnergyAccounting* ea);
    String getMeterModel(AmsData* data);
};
#endif
