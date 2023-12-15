#ifndef _JSONMQTTHANDLER_H
#define _JSONMQTTHANDLER_H

#include "AmsMqttHandler.h"

class JsonMqttHandler : public AmsMqttHandler {
public:
    JsonMqttHandler(MqttConfig& mqttConfig, RemoteDebug* debugger, char* buf, HwTools* hw) : AmsMqttHandler(mqttConfig, debugger, buf) {
        this->hw = hw;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);
    bool publishRaw(String data);

    void onMessage(String &topic, String &payload);

    uint8_t getFormat();

private:
    HwTools* hw;

    bool publishList1(AmsData* data, EnergyAccounting* ea);
    bool publishList2(AmsData* data, EnergyAccounting* ea);
    bool publishList3(AmsData* data, EnergyAccounting* ea);
    bool publishList4(AmsData* data, EnergyAccounting* ea);
    String getMeterModel(AmsData* data);
};
#endif
