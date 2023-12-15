#include "DomoticzMqttHandler.h"
#include "json/domoticz_json.h"

bool DomoticzMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi) {
    bool ret = false;
    if (config.elidx > 0) {
        if(data->getActiveImportCounter() > 1.0) {
            energy = data->getActiveImportCounter();
        }
        if(energy > 0.0) {
            char val[16];
            snprintf_P(val, 16, PSTR("%.1f;%.1f"), (data->getActiveImportPower()/1.0), energy*1000.0);
            snprintf_P(json, BufferSize, DOMOTICZ_JSON,
                config.elidx,
                val
            );
            ret = mqtt.publish(F("domoticz/in"), json);
        }
    }

    if(data->getListType() == 1)
        return ret;

    if (config.vl1idx > 0){				
        char val[16];
        snprintf_P(val, 16, PSTR("%.2f"), data->getL1Voltage());
        snprintf_P(json, BufferSize, DOMOTICZ_JSON,
            config.vl1idx,
            val
        );
        ret |= mqtt.publish(F("domoticz/in"), json);
    }

    if (config.vl2idx > 0){				
        char val[16];
        snprintf_P(val, 16, PSTR("%.2f"), data->getL2Voltage());
        snprintf_P(json, BufferSize, DOMOTICZ_JSON,
            config.vl2idx,
            val
        );
        ret |= mqtt.publish(F("domoticz/in"), json);
    }

    if (config.vl3idx > 0){				
        char val[16];
        snprintf(val, 16, "%.2f", data->getL3Voltage());
        snprintf_P(json, BufferSize, DOMOTICZ_JSON,
            config.vl3idx,
            val
        );
        ret |= mqtt.publish(F("domoticz/in"), json);
    }

    if (config.cl1idx > 0){				
        char val[16];
        snprintf(val, 16, "%.1f;%.1f;%.1f", data->getL1Current(), data->getL2Current(), data->getL3Current());
        snprintf_P(json, BufferSize, DOMOTICZ_JSON,
            config.cl1idx,
            val
        );
        ret |= mqtt.publish(F("domoticz/in"), json);
    }			
    return ret;
}

bool DomoticzMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
    return false;
}

bool DomoticzMqttHandler::publishPrices(EntsoeApi* eapi) {
    return false;
}

bool DomoticzMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
    return false;
}

uint8_t DomoticzMqttHandler::getFormat() {
    return 3;
}

bool DomoticzMqttHandler::publishRaw(String data) {
    return false;
}

void DomoticzMqttHandler::onMessage(String &topic, String &payload) {
}
