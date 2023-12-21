#include "JsonMqttHandler.h"
#include "FirmwareVersion.h"
#include "hexutils.h"
#include "Uptime.h"
#include "json/json1_json.h"
#include "json/json2_json.h"
#include "json/json3_json.h"
#include "json/json4_json.h"
#include "json/jsonsys_json.h"
#include "json/jsonprices_json.h"

bool JsonMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi) {
    if(strlen(mqttConfig.publishTopic) == 0) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Unable to publish data, no publish topic\n"));
        return false;
    }
	if(!mqtt.connected()) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Unable to publish data, not connected\n"));
		return false;
    }

    bool ret = false;

    if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Publishing list ID %d!\n"), data->getListType());
    if(data->getListType() == 1) {
        ret = publishList1(data, ea);
    } else if(data->getListType() == 2) {
        ret = publishList2(data, ea);
    } else if(data->getListType() == 3) {
        ret = publishList3(data, ea);
    } else if(data->getListType() == 4) {
        ret = publishList4(data, ea);
    }
    loop();
    return ret;
}

bool JsonMqttHandler::publishList1(AmsData* data, EnergyAccounting* ea) {
    snprintf_P(json, BufferSize, JSON1_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        data->getPackageTimestamp(),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        data->getActiveImportPower(),
        ea->getUseThisHour(),
        ea->getUseToday(),
        ea->getCurrentThreshold(),
        ea->getMonthMax(),
        ea->getProducedThisHour(),
        ea->getProducedToday()
    );
    return mqtt.publish(mqttConfig.publishTopic, json);
}

bool JsonMqttHandler::publishList2(AmsData* data, EnergyAccounting* ea) {
    snprintf_P(json, BufferSize, JSON2_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        data->getPackageTimestamp(),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        data->getListId().c_str(),
        data->getMeterId().c_str(),
        getMeterModel(data).c_str(),
        data->getActiveImportPower(),
        data->getReactiveImportPower(),
        data->getActiveExportPower(),
        data->getReactiveExportPower(),
        data->getL1Current(),
        data->getL2Current(),
        data->getL3Current(),
        data->getL1Voltage(),
        data->getL2Voltage(),
        data->getL3Voltage(),
        ea->getUseThisHour(),
        ea->getUseToday(),
        ea->getCurrentThreshold(),
        ea->getMonthMax(),
        ea->getProducedThisHour(),
        ea->getProducedToday()
    );
    return mqtt.publish(mqttConfig.publishTopic, json);
}

bool JsonMqttHandler::publishList3(AmsData* data, EnergyAccounting* ea) {
    snprintf_P(json, BufferSize, JSON3_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        data->getPackageTimestamp(),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        data->getListId().c_str(),
        data->getMeterId().c_str(),
        getMeterModel(data).c_str(),
        data->getActiveImportPower(),
        data->getReactiveImportPower(),
        data->getActiveExportPower(),
        data->getReactiveExportPower(),
        data->getL1Current(),
        data->getL2Current(),
        data->getL3Current(),
        data->getL1Voltage(),
        data->getL2Voltage(),
        data->getL3Voltage(),
        data->getActiveImportCounter(),
        data->getActiveExportCounter(),
        data->getReactiveImportCounter(),
        data->getReactiveExportCounter(),
        data->getMeterTimestamp(),
        ea->getUseThisHour(),
        ea->getUseToday(),
        ea->getCurrentThreshold(),
        ea->getMonthMax(),
        ea->getProducedThisHour(),
        ea->getProducedToday()
    );
    return mqtt.publish(mqttConfig.publishTopic, json);
}

bool JsonMqttHandler::publishList4(AmsData* data, EnergyAccounting* ea) {
    snprintf_P(json, BufferSize, JSON4_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        data->getPackageTimestamp(),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        data->getListId().c_str(),
        data->getMeterId().c_str(),
        getMeterModel(data).c_str(),
        data->getActiveImportPower(),
        data->getL1ActiveImportPower(),
        data->getL2ActiveImportPower(),
        data->getL3ActiveImportPower(),
        data->getReactiveImportPower(),
        data->getActiveExportPower(),
        data->getL1ActiveExportPower(),
        data->getL2ActiveExportPower(),
        data->getL3ActiveExportPower(),
        data->getReactiveExportPower(),
        data->getL1Current(),
        data->getL2Current(),
        data->getL3Current(),
        data->getL1Voltage(),
        data->getL2Voltage(),
        data->getL3Voltage(),
        data->getPowerFactor(),
        data->getL1PowerFactor(),
        data->getL2PowerFactor(),
        data->getL3PowerFactor(),
        data->getActiveImportCounter(),
        data->getActiveExportCounter(),
        data->getReactiveImportCounter(),
        data->getReactiveExportCounter(),
        data->getMeterTimestamp(),
        ea->getUseThisHour(),
        ea->getUseToday(),
        ea->getCurrentThreshold(),
        ea->getMonthMax(),
        ea->getProducedThisHour(),
        ea->getProducedToday()
    );
    return mqtt.publish(mqttConfig.publishTopic, json);
}

String JsonMqttHandler::getMeterModel(AmsData* data) {
    String meterModel = data->getMeterModel();
    meterModel.replace("\\", "\\\\");
    return meterModel;
}

bool JsonMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
	int count = hw->getTempSensorCount();
    if(count < 2) {
        return false;
    }

	snprintf_P(json, 24, PSTR("{\"temperatures\":{"));

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            char* pos = json+strlen(json);
            snprintf_P(pos, 26, PSTR("\"%s\":%.2f,"), 
                toHex(data->address, 8).c_str(),
                data->lastRead
            );
            data->changed = false;
        }
	}
	char* pos = json+strlen(json);
	snprintf_P(count == 0 ? pos : pos-1, 8, PSTR("}}"));
    bool ret = mqtt.publish(mqttConfig.publishTopic, json);
    loop();
    return ret;
}

bool JsonMqttHandler::publishPrices(EntsoeApi* eapi) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;
	if(eapi->getValueForHour(0) == ENTSOE_NO_VALUE)
		return false;

	time_t now = time(nullptr);

	float min1hr = 0.0, min3hr = 0.0, min6hr = 0.0;
	int8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[38];
    for(int i = 0;i < 38; i++) values[i] = ENTSOE_NO_VALUE;
	for(uint8_t i = 0; i < 38; i++) {
		float val = eapi->getValueForHour(now, i);
		values[i] = val;

		if(val == ENTSOE_NO_VALUE) break;
		
		if(val < min) min = val;
		if(val > max) max = val;

		if(min1hrIdx == -1 || min1hr > val) {
			min1hr = val;
			min1hrIdx = i;
		}

		if(i >= 2) {
			i -= 2;
			float val1 = values[i++];
			float val2 = values[i++];
			float val3 = val;
			if(val1 == ENTSOE_NO_VALUE || val2 == ENTSOE_NO_VALUE || val3 == ENTSOE_NO_VALUE) continue;
			float val3hr = val1+val2+val3;
			if(min3hrIdx == -1 || min3hr > val3hr) {
				min3hr = val3hr;
				min3hrIdx = i-2;
			}
		}

		if(i >= 5) {
			i -= 5;
			float val1 = values[i++];
			float val2 = values[i++];
			float val3 = values[i++];
			float val4 = values[i++];
			float val5 = values[i++];
			float val6 = val;
			if(val1 == ENTSOE_NO_VALUE || val2 == ENTSOE_NO_VALUE || val3 == ENTSOE_NO_VALUE || val4 == ENTSOE_NO_VALUE || val5 == ENTSOE_NO_VALUE || val6 == ENTSOE_NO_VALUE) continue;
			float val6hr = val1+val2+val3+val4+val5+val6;
			if(min6hrIdx == -1 || min6hr > val6hr) {
				min6hr = val6hr;
				min6hrIdx = i-5;
			}
		}

	}

	char ts1hr[24];
    memset(ts1hr, 0, 24);
	if(min1hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min1hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf_P(ts1hr, PSTR("%04d-%02d-%02dT%02d:00:00Z"), tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[24];
    memset(ts3hr, 0, 24);
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf_P(ts3hr, PSTR("%04d-%02d-%02dT%02d:00:00Z"), tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[24];
    memset(ts6hr, 0, 24);
	if(min6hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min6hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf_P(ts6hr, PSTR("%04d-%02d-%02dT%02d:00:00Z"), tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

    snprintf_P(json, BufferSize, JSONPRICES_JSON,
        WiFi.macAddress().c_str(),
        values[0] == ENTSOE_NO_VALUE ? "null" : String(values[0], 4).c_str(),
        values[1] == ENTSOE_NO_VALUE ? "null" : String(values[1], 4).c_str(),
        values[2] == ENTSOE_NO_VALUE ? "null" : String(values[2], 4).c_str(),
        values[3] == ENTSOE_NO_VALUE ? "null" : String(values[3], 4).c_str(),
        values[4] == ENTSOE_NO_VALUE ? "null" : String(values[4], 4).c_str(),
        values[5] == ENTSOE_NO_VALUE ? "null" : String(values[5], 4).c_str(),
        values[6] == ENTSOE_NO_VALUE ? "null" : String(values[6], 4).c_str(),
        values[7] == ENTSOE_NO_VALUE ? "null" : String(values[7], 4).c_str(),
        values[8] == ENTSOE_NO_VALUE ? "null" : String(values[8], 4).c_str(),
        values[9] == ENTSOE_NO_VALUE ? "null" : String(values[9], 4).c_str(),
        values[10] == ENTSOE_NO_VALUE ? "null" : String(values[10], 4).c_str(),
        values[11] == ENTSOE_NO_VALUE ? "null" : String(values[11], 4).c_str(),
        values[12] == ENTSOE_NO_VALUE ? "null" : String(values[12], 4).c_str(),
        values[13] == ENTSOE_NO_VALUE ? "null" : String(values[13], 4).c_str(),
        values[14] == ENTSOE_NO_VALUE ? "null" : String(values[14], 4).c_str(),
        values[15] == ENTSOE_NO_VALUE ? "null" : String(values[15], 4).c_str(),
        values[16] == ENTSOE_NO_VALUE ? "null" : String(values[16], 4).c_str(),
        values[17] == ENTSOE_NO_VALUE ? "null" : String(values[17], 4).c_str(),
        values[18] == ENTSOE_NO_VALUE ? "null" : String(values[18], 4).c_str(),
        values[19] == ENTSOE_NO_VALUE ? "null" : String(values[19], 4).c_str(),
        values[20] == ENTSOE_NO_VALUE ? "null" : String(values[20], 4).c_str(),
        values[21] == ENTSOE_NO_VALUE ? "null" : String(values[21], 4).c_str(),
        values[22] == ENTSOE_NO_VALUE ? "null" : String(values[22], 4).c_str(),
        values[23] == ENTSOE_NO_VALUE ? "null" : String(values[23], 4).c_str(),
        values[24] == ENTSOE_NO_VALUE ? "null" : String(values[24], 4).c_str(),
        values[25] == ENTSOE_NO_VALUE ? "null" : String(values[25], 4).c_str(),
        values[26] == ENTSOE_NO_VALUE ? "null" : String(values[26], 4).c_str(),
        values[27] == ENTSOE_NO_VALUE ? "null" : String(values[27], 4).c_str(),
        values[28] == ENTSOE_NO_VALUE ? "null" : String(values[28], 4).c_str(),
        values[29] == ENTSOE_NO_VALUE ? "null" : String(values[29], 4).c_str(),
        values[30] == ENTSOE_NO_VALUE ? "null" : String(values[30], 4).c_str(),
        values[31] == ENTSOE_NO_VALUE ? "null" : String(values[31], 4).c_str(),
        values[32] == ENTSOE_NO_VALUE ? "null" : String(values[32], 4).c_str(),
        values[33] == ENTSOE_NO_VALUE ? "null" : String(values[33], 4).c_str(),
        values[34] == ENTSOE_NO_VALUE ? "null" : String(values[34], 4).c_str(),
        values[35] == ENTSOE_NO_VALUE ? "null" : String(values[35], 4).c_str(),
        values[36] == ENTSOE_NO_VALUE ? "null" : String(values[36], 4).c_str(),
        values[37] == ENTSOE_NO_VALUE ? "null" : String(values[37], 4).c_str(),
        min == INT16_MAX ? 0.0 : min,
        max == INT16_MIN ? 0.0 : max,
        ts1hr,
        ts3hr,
        ts6hr
    );
    bool ret = mqtt.publish(mqttConfig.publishTopic, json);
    loop();
    return ret;
}

bool JsonMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;

    snprintf_P(json, BufferSize, JSONSYS_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        FirmwareVersion::VersionString
    );
    bool ret = mqtt.publish(mqttConfig.publishTopic, json);
    loop();
    return ret;
}

uint8_t JsonMqttHandler::getFormat() {
    return 0;
}

bool JsonMqttHandler::publishRaw(String data) {
    return false;
}

void JsonMqttHandler::onMessage(String &topic, String &payload) {
}
