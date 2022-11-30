#include "HomeAssistantMqttHandler.h"
#include "HomeAssistantStatic.h"
#include "hexutils.h"
#include "Uptime.h"
#include "version.h"
#include "web/root/ha1_json.h"
#include "web/root/ha2_json.h"
#include "web/root/ha3_json.h"
#include "web/root/jsonsys_json.h"
#include "web/root/jsonprices_json.h"
#include "web/root/hadiscover_json.h"
#include "web/root/realtime_json.h"

bool HomeAssistantMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

    listType = data->getListType(); // for discovery stuff in publishSystem()
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
        return mqtt->publish(topic + "/power", json);
    } else if(data->getListType() >= 2) { // publish power counts and volts/amps
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
            data->getL3Voltage(),
            data->getPowerFactor() == 0 ? 1 : data->getPowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL1PowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL2PowerFactor(),
            data->getPowerFactor() == 0 ? 1 : data->getL3PowerFactor()
        );
        return mqtt->publish(topic + "/power", json);
    }
    return false;}

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
	if(strlen(eapi->getToken()) == 0)
		return false;

	time_t now = time(nullptr);

	float min1hr, min3hr, min6hr;
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

	char ts1hr[21];
	if(min1hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min1hrIdx);
        //Serial.printf("1hr: %d %lu\n", min1hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[21];
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
        //Serial.printf("3hr: %d %lu\n", min3hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[21];
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
    return mqtt->publish(topic + "/prices", json);
}

bool HomeAssistantMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected()){
        sequence = 0;
		return false;
    }

    if(sequence % 3 == 0){
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
    }

    if(!autodiscoverInit) {
        #if defined(ESP8266)
			String haUID = WiFi.hostname();
        #elif defined(ESP32)
			String haUID = WiFi.getHostname();
        #endif
        String haUrl = "http://" + haUID + ".local/";
        // Could this be necessary? haUID.replace("-", "_");

        for(int i=0;i<17;i++){
            snprintf_P(json, BufferSize, HADISCOVER_JSON,
                FPSTR(HA_NAMES[i]),
                topic.c_str(), FPSTR(HA_TOPICS[i]),
                haUID.c_str(), FPSTR(HA_PARAMS[i]),
                haUID.c_str(), FPSTR(HA_PARAMS[i]),
                FPSTR(HA_UOM[i]),
                FPSTR(HA_PARAMS[i]),
                FPSTR(HA_DEVCL[i]),
                haUID.c_str(),
                haName.c_str(),
                haModel.c_str(),
                VERSION,
                haManuf.c_str(),
                haUrl.c_str(),
                strlen_P(HA_STACL[i]) > 0 ? ", \"stat_cla\" :" : "",
                strlen_P(HA_STACL[i]) > 0 ? (char *) FPSTR(HA_STACL[i]) : ""
            );
            mqtt->publish(haTopic + haUID + "_" + FPSTR(HA_PARAMS[i]) + "/config", json, true, 0);
        }
        autodiscoverInit = true;
    }
    if(listType>0) sequence++;
    return true;}
