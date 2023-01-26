#include "HomeAssistantMqttHandler.h"
#include "HomeAssistantStatic.h"
#include "hexutils.h"
#include "Uptime.h"
#include "version.h"
#include "json/ha1_json.h"
#include "json/ha2_json.h"
#include "json/ha3_json.h"
#include "json/ha4_json.h"
#include "json/jsonsys_json.h"
#include "json/jsonprices_json.h"
#include "json/hadiscover_json.h"
#include "json/realtime_json.h"

bool HomeAssistantMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

    if(data->getListType() >= 3) { // publish energy counts
        snprintf_P(json, BufferSize, HA2_JSON,
            data->getActiveImportCounter(),
            data->getActiveExportCounter(),
            data->getReactiveImportCounter(),
            data->getReactiveExportCounter(),
            data->getMeterTimestamp()
        );
        mqtt->publish(topic + "/energy", json);
        mqtt->loop();
    }
    String meterModel = data->getMeterModel();
    meterModel.replace("\\", "\\\\");
    if(data->getListType() == 1) { // publish power counts
        snprintf_P(json, BufferSize, HA1_JSON,
            data->getActiveImportPower()
        );
        mqtt->publish(topic + "/power", json);
    } else if(data->getListType() <= 3) { // publish power counts and volts/amps
        snprintf_P(json, BufferSize, HA3_JSON,
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
            data->getL3Voltage()
        );
        mqtt->publish(topic + "/power", json);
    } else if(data->getListType() == 4) { // publish power counts and volts/amps/phase power and PF
        snprintf_P(json, BufferSize, HA4_JSON,
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
            data->getPowerFactor() == 0 ? 1 : data->getPowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL1PowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL2PowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL3PowerFactor()
        );
        mqtt->publish(topic + "/power", json);
    }

	String peaks = "";
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
	for(uint8_t i = 1; i <= peakCount; i++) {
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(ea->getPeak(i).value / 100.0, 2);
	}
    snprintf_P(json, BufferSize, REALTIME_JSON,
		ea->getMonthMax(),
		peaks.c_str(),
		ea->getCurrentThreshold(),
		ea->getUseThisHour(),
		ea->getCostThisHour(),
		ea->getProducedThisHour(),
		ea->getUseToday(),
		ea->getCostToday(),
		ea->getProducedToday(),
		ea->getUseThisMonth(),
		ea->getCostThisMonth(),
		ea->getProducedThisMonth()
    );
    mqtt->publish(topic + "/realtime", json);

    return true;
}

bool HomeAssistantMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
	int count = hw->getTempSensorCount();
    if(count == 0) return false;

	int size = 32 + (count * 26);

	char buf[size];
	snprintf(buf, 24, "{\"temperatures\":{");

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            char* pos = buf+strlen(buf);
            snprintf(pos, 26, "\"%s\":%.2f,", 
                toHex(data->address, 8).c_str(),
                data->lastRead
            );
            data->changed = false;
            delay(1);
        }
	}
	char* pos = buf+strlen(buf);
	snprintf(count == 0 ? pos : pos-1, 8, "}}");
    return mqtt->publish(topic + "/temperatures", buf);
}

bool HomeAssistantMqttHandler::publishPrices(EntsoeApi* eapi) {
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
    return mqtt->publish(topic + "/prices", json, true, 0);
}

bool HomeAssistantMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
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
    mqtt->publish(topic + "/state", json);

    if(!autodiscoverInit) {
        #if defined(ESP8266)
			String haUID = WiFi.hostname();
        #elif defined(ESP32)
			String haUID = WiFi.getHostname();
        #endif
        String haUrl = "http://" + haUID + ".local/";
        // Could this be necessary? haUID.replace("-", "_");
        uint8_t peakCount = ea->getConfig()->hours;
        if(peakCount > 5) peakCount = 5;

        uint8_t peaks = 0;
        for(int i=0;i<HA_SENSOR_COUNT;i++) {
            HomeAssistantSensor sensor = HA_SENSORS[i];
            String uid = String(sensor.path);
            uid.replace(".", "");
            uid.replace("[", "");
            uid.replace("]", "");
            uid.replace("'", "");
            String uom = String(sensor.uom);
            if(strncmp(sensor.devcl, "monetary", 8) == 0) {
                if(eapi == NULL) continue;
                if(strncmp(sensor.path, "prices", 5) == 0) {
                    uom = String(eapi->getCurrency()) + "/kWh";
                } else {
                    uom = String(eapi->getCurrency());
                }
            }
            if(strncmp(sensor.path, "peaks[", 6) == 0) {
                if(peaks >= peakCount) continue;
                peaks++;
            }
            if(strncmp(sensor.path, "temp", 4) == 0) {
                if(hw->getTemperature() < 0) continue;
            }
            snprintf_P(json, BufferSize, HADISCOVER_JSON,
                sensor.name,
                topic.c_str(), sensor.topic,
                haUID.c_str(), uid.c_str(),
                haUID.c_str(), uid.c_str(),
                uom.c_str(),
                sensor.path,
                sensor.devcl,
                haUID.c_str(),
                haName.c_str(),
                haModel.c_str(),
                VERSION,
                haManuf.c_str(),
                haUrl.c_str(),
                strlen_P(sensor.stacl) > 0 ? ", \"stat_cla\" :" : "",
                strlen_P(sensor.stacl) > 0 ? (char *) FPSTR(sensor.stacl) : ""
            );
            mqtt->publish(haTopic + haUID + "_" + uid.c_str() + "/config", json, true, 0);
        }

        autodiscoverInit = true;
    }
    return true;
}
