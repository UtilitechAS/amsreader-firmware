#ifndef _RAWMQTTHANDLER_H
#define _RAWMQTTHANDLER_H

#include "AmsMqttHandler.h"

class RawMqttHandler : public AmsMqttHandler {
public:
    RawMqttHandler(MQTTClient* mqtt, char* buf, const char* topic, bool full) : AmsMqttHandler(mqtt, buf) {
        this->topic = String(topic);
        this->full = full;
    };
    bool publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi);
    bool publishTemperatures(AmsConfiguration*, HwTools*);
    bool publishPrices(EntsoeApi*);
    bool publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea);

protected:
    bool loop();

private:
    String topic;
    bool full;

    bool publishList1(AmsData* data, AmsData* meterState);
    bool publishList2(AmsData* data, AmsData* meterState);
    bool publishList3(AmsData* data, AmsData* meterState);
    bool publishList4(AmsData* data, AmsData* meterState);
    bool publishRealtime(EnergyAccounting* ea);
};
#endif
