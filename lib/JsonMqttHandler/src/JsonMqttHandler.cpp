#include "JsonMqttHandler.h"
#include "version.h"
#include "hexutils.h"
#include "Uptime.h"
#include "json/json1_json.h"
#include "json/json2_json.h"
#include "json/json3_json.h"
#include "json/json4_json.h"
#include "json/jsonsys_json.h"
#include "json/jsonprices_json.h"

bool JsonMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

    String meterModel = data->getMeterModel();
    meterModel.replace("\\", "\\\\");
    if(data->getListType() == 1) {
        snprintf_P(json, BufferSize, JSON1_JSON,
            WiFi.macAddress().c_str(),
            clientId.c_str(),
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
        return mqtt->publish(topic, json);
    } else if(data->getListType() == 2) {
        snprintf_P(json, BufferSize, JSON2_JSON,
            WiFi.macAddress().c_str(),
            clientId.c_str(),
            (uint32_t) (millis64()/1000),
            data->getPackageTimestamp(),
            hw->getVcc(),
            hw->getWifiRssi(),
            hw->getTemperature(),
            data->getListId().c_str(),
            data->getMeterId().c_str(),
            meterModel.c_str(),
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
        return mqtt->publish(topic, json);
    } else if(data->getListType() == 3) {
        snprintf_P(json, BufferSize, JSON3_JSON,
            WiFi.macAddress().c_str(),
            clientId.c_str(),
            (uint32_t) (millis64()/1000),
            data->getPackageTimestamp(),
            hw->getVcc(),
            hw->getWifiRssi(),
            hw->getTemperature(),
            data->getListId().c_str(),
            data->getMeterId().c_str(),
            meterModel.c_str(),
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
        return mqtt->publish(topic, json);
    } else if(data->getListType() == 4) {
        snprintf_P(json, BufferSize, JSON4_JSON,
            WiFi.macAddress().c_str(),
            clientId.c_str(),
            (uint32_t) (millis64()/1000),
            data->getPackageTimestamp(),
            hw->getVcc(),
            hw->getWifiRssi(),
            hw->getTemperature(),
            data->getListId().c_str(),
            data->getMeterId().c_str(),
            meterModel.c_str(),
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
        return mqtt->publish(topic, json);
    }
    return false;
}

bool JsonMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
	int count = hw->getTempSensorCount();
    if(count < 2) {
        return false;
    }

	snprintf(json, 24, "{\"temperatures\":{");

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            char* pos = json+strlen(json);
            snprintf(pos, 26, "\"%s\":%.2f,", 
                toHex(data->address, 8).c_str(),
                data->lastRead
            );
            data->changed = false;
            delay(1);
        }
	}
	char* pos = json+strlen(json);
	snprintf(count == 0 ? pos : pos-1, 8, "}}");
    return mqtt->publish(topic, json);
}

bool JsonMqttHandler::publishPrices(EntsoeApi* eapi) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;
	if(eapi->getValueForHour(0) == ENTSOE_NO_VALUE)
		return false;

	time_t now = time(nullptr);

	float min1hr = 0.0, min3hr = 0.0, min6hr = 0.0;
	int8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[24];
    for(int i = 0;i < 24; i++) values[i] = ENTSOE_NO_VALUE;
	for(uint8_t i = 0; i < 24; i++) {
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
        //Serial.printf("1hr: %d %lu\n", min1hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[24];
    memset(ts3hr, 0, 24);
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
        //Serial.printf("3hr: %d %lu\n", min3hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[24];
    memset(ts6hr, 0, 24);
	if(min6hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min6hrIdx);
        //Serial.printf("6hr: %d %lu\n", min6hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts6hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

    snprintf_P(json, BufferSize, JSONPRICES_JSON,
        WiFi.macAddress().c_str(),
        values[0],
        values[1],
        values[2],
        values[3],
        values[4],
        values[5],
        values[6],
        values[7],
        values[8],
        values[9],
        values[10],
        values[11],
        min == INT16_MAX ? 0.0 : min,
        max == INT16_MIN ? 0.0 : max,
        ts1hr,
        ts3hr,
        ts6hr
    );
    return mqtt->publish(topic, json);
}

bool JsonMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

    snprintf_P(json, BufferSize, JSONSYS_JSON,
        WiFi.macAddress().c_str(),
        clientId.c_str(),
        (uint32_t) (millis64()/1000),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        VERSION
    );
    return mqtt->publish(topic, json);
}
