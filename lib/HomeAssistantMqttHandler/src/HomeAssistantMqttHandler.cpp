/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "HomeAssistantMqttHandler.h"
#include "hexutils.h"
#include "Uptime.h"
#include "FirmwareVersion.h"
#include "json/ha1_json.h"
#include "json/ha2_json.h"
#include "json/ha3_json.h"
#include "json/ha4_json.h"
#include "json/hadiscover_json.h"
#include "FirmwareVersion.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

void HomeAssistantMqttHandler::setHomeAssistantConfig(HomeAssistantConfig config, char* hostname) {
    l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = rInit = fInit = false;

    if(strlen(config.discoveryNameTag) > 0) {
        snprintf_P(json, 128, PSTR("AMS reader (%s)"), config.discoveryNameTag);
        deviceName = String(json);
        snprintf_P(json, 128, PSTR("[%s] "), config.discoveryNameTag);
        sensorNamePrefix = String(json);
    } else {
        deviceName = F("AMS reader");
        sensorNamePrefix = "";
    }
    deviceModel = boardTypeToString(boardType);
    manufacturer = boardManufacturerToString(boardType);

    deviceUid = String(hostname); // Maybe configurable in the future?
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("  Hostname is [%s]\n"), hostname);

    if(strlen(config.discoveryHostname) > 0) {
        if(strncmp_P(config.discoveryHostname, PSTR("http"), 4) == 0) {
            deviceUrl = String(config.discoveryHostname);
        } else {
            snprintf_P(json, 128, PSTR("http://%s/"), config.discoveryHostname);
            deviceUrl = String(json);
        }
    } else {
        snprintf_P(json, 128, PSTR("http://%s.local/"), hostname);
        deviceUrl = String(json);
    }

    if(strlen(config.discoveryPrefix) > 0) {
        snprintf_P(json, 128, PSTR("%s/status"), config.discoveryPrefix);
        statusTopic = String(json);

        snprintf_P(json, 128, PSTR("%s/sensor"), config.discoveryPrefix);
        sensorTopic = String(json);

        snprintf_P(json, 128, PSTR("%s/update"), config.discoveryPrefix);
        updateTopic = String(json);
    } else {
        statusTopic = F("homeassistant/status");
        sensorTopic = F("homeassistant/sensor");
        updateTopic = F("homeassistant/update");
    }
    strcpy(this->mqttConfig.subscribeTopic, statusTopic.c_str());
}

bool HomeAssistantMqttHandler::postConnect() {
    bool ret = true;
    if(!statusTopic.isEmpty()) {
        if(mqtt.subscribe(statusTopic)) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("  Subscribed to [%s]\n"), statusTopic.c_str());
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("  Unable to subscribe to [%s]\n"), statusTopic.c_str());
            ret = false;
        }
    }
    return ret;
}

bool HomeAssistantMqttHandler::publish(AmsData* update, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) {
	if(pubTopic.isEmpty() || !mqtt.connected())
		return false;

    if(time(nullptr) < FirmwareVersion::BuildEpoch)
        return false;

    AmsData data;
    if(mqttConfig.stateUpdate) {
        uint64_t now = millis64();
        if(now-lastStateUpdate < mqttConfig.stateUpdateInterval * 1000) return false;
        data.apply(*previousState);
        data.apply(*update);
        lastStateUpdate = now;
    } else {
        data = *update;
    }

    if(data.getListType() >= 3 && !data.isCounterEstimated()) { // publish energy counts
        publishList3(&data, ea);
        mqtt.loop();
    }

    if(data.getListType() == 1) { // publish power counts
        publishList1(&data, ea);
        mqtt.loop();
    } else if(data.getListType() <= 3) { // publish power counts and volts/amps
        publishList2(&data, ea);
        mqtt.loop();
    } else if(data.getListType() == 4) { // publish power counts and volts/amps/phase power and PF
        publishList4(&data, ea);
        mqtt.loop();
    }

    if(ea->isInitialized()) {
        publishRealtime(&data, ea, ps);
        mqtt.loop();
    }
    loop();
    return true;
}

bool HomeAssistantMqttHandler::publishList1(AmsData* data, EnergyAccounting* ea) {
    publishList1Sensors();

	char pt[24];
    memset(pt, 0, 24);
    if(data->getPackageTimestamp() > 0) {
        tmElements_t tm;
        breakTime(data->getPackageTimestamp(), tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

    snprintf_P(json, BufferSize, HA1_JSON, data->getActiveImportPower(), pt);
    return mqtt.publish(pubTopic + "/power", json);
}

bool HomeAssistantMqttHandler::publishList2(AmsData* data, EnergyAccounting* ea) {
    publishList2Sensors();
    if(data->getActiveExportPower() > 0) publishList2ExportSensors();

	char pt[24];
    memset(pt, 0, 24);
    if(data->getPackageTimestamp() > 0) {
        tmElements_t tm;
        breakTime(data->getPackageTimestamp(), tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

    snprintf_P(json, BufferSize, HA3_JSON,
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
        pt
    );
    return mqtt.publish(pubTopic + "/power", json);
}

bool HomeAssistantMqttHandler::publishList3(AmsData* data, EnergyAccounting* ea) {
    publishList3Sensors();
    if(data->getActiveExportCounter() > 0.0) publishList3ExportSensors();

	char mt[24];
    memset(mt, 0, 24);
    if(data->getMeterTimestamp() > 0) {
        tmElements_t tm;
        breakTime(data->getMeterTimestamp(), tm);
        sprintf_P(mt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

	char pt[24];
    memset(pt, 0, 24);
    if(data->getPackageTimestamp() > 0) {
        tmElements_t tm;
        breakTime(data->getPackageTimestamp(), tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

    snprintf_P(json, BufferSize, HA2_JSON,
        data->getActiveImportCounter(),
        data->getActiveExportCounter(),
        data->getReactiveImportCounter(),
        data->getReactiveExportCounter(),
        mt,
        pt
    );
    return mqtt.publish(pubTopic + "/energy", json);
}

bool HomeAssistantMqttHandler::publishList4(AmsData* data, EnergyAccounting* ea) {
    publishList4Sensors();
    if(data->getL1ActiveExportPower() > 0 || data->getL2ActiveExportPower() > 0 || data->getL3ActiveExportPower() > 0) publishList4ExportSensors();

	char pt[24];
    memset(pt, 0, 24);
    if(data->getPackageTimestamp() > 0) {
        tmElements_t tm;
        breakTime(data->getPackageTimestamp(), tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

    snprintf_P(json, BufferSize, HA4_JSON,
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
        data->getPowerFactor() == 0 ? 100 : (int) (data->getPowerFactor() * 100),
        data->getPowerFactor() == 0 ? 100 : (int) (data->getL1PowerFactor() * 100),
        data->getPowerFactor() == 0 ? 100 : (int) (data->getL2PowerFactor() * 100),
        data->getPowerFactor() == 0 ? 100 : (int) (data->getL3PowerFactor() * 100),
        data->getL1ActiveImportCounter(),
        data->getL2ActiveImportCounter(),
        data->getL3ActiveImportCounter(),
        data->getL1ActiveExportCounter(),
        data->getL2ActiveExportCounter(),
        data->getL3ActiveExportCounter(),
        pt
    );
    return mqtt.publish(pubTopic + "/power", json);
}

String HomeAssistantMqttHandler::getMeterModel(AmsData* data) {
    String meterModel = data->getMeterModel();
    meterModel.replace("\\", "\\\\");
    return meterModel;
}

bool HomeAssistantMqttHandler::publishRealtime(AmsData* data, EnergyAccounting* ea, PriceService* ps) {
    publishRealtimeSensors(ea, ps);
    if(ea->getProducedThisHour() > 0.0 || ea->getProducedToday() > 0.0 || ea->getProducedThisMonth() > 0.0) publishRealtimeExportSensors(ea, ps);
    if(lastThresholdPublish == 0) publishThresholdSensors();
    String peaks = "";
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 1; i <= peakCount; i++) {
        if(!peaks.isEmpty()) peaks += ",";
        peaks += String(ea->getPeak(i).value / 100.0, 2);
    }
    uint16_t pos = snprintf_P(json, BufferSize, PSTR("{\"max\":%.1f,\"peaks\":[%s],\"threshold\":%d,\"hour\":{\"use\":%.2f,\"cost\":%.2f,\"produced\":%.2f,\"income\":%.2f},\"day\":{\"use\":%.2f,\"cost\":%.2f,\"produced\":%.2f,\"income\":%.2f},\"month\":{\"use\":%.2f,\"cost\":%.2f,\"produced\":%.2f,\"income\":%.2f}"),
        ea->getMonthMax(),
        peaks.c_str(),
        ea->getCurrentThreshold(),
        ea->getUseThisHour(),
        ea->getCostThisHour(),
        ea->getProducedThisHour(),
        ea->getIncomeThisHour(),
        ea->getUseToday(),
        ea->getCostToday(),
        ea->getProducedToday(),
        ea->getIncomeToday(),
        ea->getUseThisMonth(),
        ea->getCostThisMonth(),
        ea->getProducedThisMonth(),
        ea->getIncomeThisMonth()
    );
    uint32_t ms = millis();
    if(lastThresholdPublish == 0 || ms-lastThresholdPublish > 3600000) {
        EnergyAccountingConfig* conf = ea->getConfig();
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR(",\"thresholds\": [%d,%d,%d,%d,%d,%d,%d,%d,%d]"),
            conf->thresholds[0],
            conf->thresholds[1],
            conf->thresholds[2],
            conf->thresholds[3],
            conf->thresholds[4],
            conf->thresholds[5],
            conf->thresholds[6],
            conf->thresholds[7],
            conf->thresholds[8]
        );
        lastThresholdPublish = ms;
    }

    time_t now = time(nullptr);
	char pt[24];
    memset(pt, 0, 24);
    if(now > 0) {
        tmElements_t tm;
        breakTime(now, tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR(",\"t\":\"%s\""), pt);
    
    json[pos++] = '}';
    json[pos] = '\0';

    return mqtt.publish(pubTopic + "/realtime", json);
}

bool HomeAssistantMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
	int count = hw->getTempSensorCount();
    if(count < 2) return false;

	uint16_t pos = snprintf_P(json, 24, PSTR("{\"temperatures\":{"));

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            String id = toHex(data->address, 8);
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%s\":%.2f,"), 
                id.c_str(),
                data->lastRead
            );
            data->changed = false;
            publishTemperatureSensor(i+1, id);
        }
	}
	pos += snprintf_P(json+pos, BufferSize-pos, PSTR("}"));

    time_t now = time(nullptr);
	char pt[24];
    memset(pt, 0, 24);
    if(now > 0) {
        tmElements_t tm;
        breakTime(now, tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR(",\"t\":\"%s\""), pt);
	pos += snprintf_P(json+pos, BufferSize-pos, PSTR("}"));

    bool ret = mqtt.publish(pubTopic + "/temperatures", json);
    loop();
    return ret;
}

bool HomeAssistantMqttHandler::publishPrices(PriceService* ps) {
	if(pubTopic.isEmpty() || !mqtt.connected())
		return false;
	if(!ps->hasPrice())
		return false;

    publishPriceSensors(ps);

	time_t now = time(nullptr);

	float min1hr = 0.0, min3hr = 0.0, min6hr = 0.0;
	int8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[38];
    for(int i = 0;i < 38; i++) values[i] = PRICE_NO_VALUE;
	for(uint8_t i = 0; i < 38; i++) {
		float val = ps->getPriceForRelativeHour(PRICE_DIRECTION_IMPORT, i);
		values[i] = val;

		if(val == PRICE_NO_VALUE) break;
		
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
			if(val1 == PRICE_NO_VALUE || val2 == PRICE_NO_VALUE || val3 == PRICE_NO_VALUE) continue;
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
			if(val1 == PRICE_NO_VALUE || val2 == PRICE_NO_VALUE || val3 == PRICE_NO_VALUE || val4 == PRICE_NO_VALUE || val5 == PRICE_NO_VALUE || val6 == PRICE_NO_VALUE) continue;
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

    uint16_t pos = snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\",\"prices\":{\"import\":["), WiFi.macAddress().c_str());

    uint8_t currentPricePointIndex = ps->getCurrentPricePointIndex();
	uint8_t numberOfPoints = ps->getNumberOfPointsAvailable();
	for(int i = currentPricePointIndex; i < numberOfPoints; i++) {
		float val = ps->getPricePoint(PRICE_DIRECTION_IMPORT, i);
        if(val == PRICE_NO_VALUE) {
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("null,"));
        } else {
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("%.4f,"), val);
        }
	}
    if(rteInit && ps->isExportPricesDifferentFromImport()) {
        pos--;
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("],\"export\":["));
        for(int i = currentPricePointIndex; i < numberOfPoints; i++) {
            float val = ps->getPricePoint(PRICE_DIRECTION_EXPORT, i);
            if(val == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("null,"));
            } else {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("%.4f,"), val);
            }
        }
    }

    pos--;
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("],\"min\":%.4f,\"max\":%.4f,\"cheapest1hr\":\"%s\",\"cheapest3hr\":\"%s\",\"cheapest6hr\":\"%s\"}"),
        min == INT16_MAX ? 0.0 : min,
        max == INT16_MIN ? 0.0 : max,
        ts1hr,
        ts3hr,
        ts6hr
    );
    
    char pt[24];
    memset(pt, 0, 24);
    if(now > 0) {
        tmElements_t tm;
        breakTime(now, tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR(",\"t\":\"%s\""), pt);

    json[pos++] = '}';
    json[pos] = '\0';

    bool ret = mqtt.publish(pubTopic + "/prices", json, true, 0);
    loop();
    return ret;
}

bool HomeAssistantMqttHandler::publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea) {
	if(pubTopic.isEmpty() || !mqtt.connected())
		return false;

    publishSystemSensors();
    if(hw->getTemperature() > -50) publishTemperatureSensor(0, "");

	time_t now = time(nullptr);
	char pt[24];
    memset(pt, 0, 24);
    if(now > 0) {
        tmElements_t tm;
        breakTime(now, tm);
        sprintf_P(pt, PSTR("%04d-%02d-%02dT%02d:%02d:%02dZ"), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    }

    snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\",\"name\":\"%s\",\"up\":%d,\"vcc\":%.3f,\"rssi\":%d,\"temp\":%.2f,\"version\":\"%s\",\"t\":\"%s\"}"),
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        FirmwareVersion::VersionString,
        pt
    );
    bool ret = mqtt.publish(pubTopic + "/state", json);
    loop();
    return ret;
}

void HomeAssistantMqttHandler::publishSensor(const HomeAssistantSensor sensor) {
    String uid;
    if(strlen(sensor.uid) > 0) {
        uid = String(sensor.uid);
    } else {
        uid = String(sensor.path);
        uid.replace(".", "");
        uid.replace("[", "");
        uid.replace("]", "");
        uid.replace("'", "");
    }
    snprintf_P(json, BufferSize, HADISCOVER_JSON,
        sensorNamePrefix.c_str(),
        sensor.name,
        mqttConfig.publishTopic, sensor.topic,
        deviceUid.c_str(), uid.c_str(),
        deviceUid.c_str(), uid.c_str(),
        sensor.uom,
        sensor.path,
        sensor.ttl,
        deviceUid.c_str(),
        deviceName.c_str(),
        deviceModel.c_str(),
        FirmwareVersion::VersionString,
        manufacturer.c_str(),
        deviceUrl.c_str(),
        strlen_P(sensor.devcl) > 0 ? ",\"dev_cla\":\"" : "",
        strlen_P(sensor.devcl) > 0 ? (char *) FPSTR(sensor.devcl) : "",
        strlen_P(sensor.devcl) > 0 ? "\"" : "",
        strlen_P(sensor.stacl) > 0 ? ",\"stat_cla\":\"" : "",
        strlen_P(sensor.stacl) > 0 ? (char *) FPSTR(sensor.stacl) : "",
        strlen_P(sensor.stacl) > 0 ? "\"" : ""
    );
    mqtt.publish(sensorTopic + "/" + deviceUid + "_" + uid + "/config", json, true, 0);
    loop();
}

void HomeAssistantMqttHandler::publishList1Sensors() {
    if(l1Init) return;
    for(uint8_t i = 0; i < List1SensorCount; i++) {
        publishSensor(List1Sensors[i]);
    }
    l1Init = true;
}

void HomeAssistantMqttHandler::publishList2Sensors() {
    publishList1Sensors();
    if(l2Init) return;
    for(uint8_t i = 0; i < List2SensorCount; i++) {
        publishSensor(List2Sensors[i]);
    }
    l2Init = true;
}

void HomeAssistantMqttHandler::publishList2ExportSensors() {
    if(l2eInit) return;
    for(uint8_t i = 0; i < List2ExportSensorCount; i++) {
        publishSensor(List2ExportSensors[i]);
    }
    l2eInit = true;
}

void HomeAssistantMqttHandler::publishList3Sensors() {
    publishList2Sensors();
    if(l3Init) return;
    for(uint8_t i = 0; i < List3SensorCount; i++) {
        publishSensor(List3Sensors[i]);
    }
    l3Init = true;
}

void HomeAssistantMqttHandler::publishList3ExportSensors() {
    publishList2ExportSensors();
    if(l3eInit) return;
    for(uint8_t i = 0; i < List3ExportSensorCount; i++) {
        publishSensor(List3ExportSensors[i]);
    }
    l3eInit = true;
}

void HomeAssistantMqttHandler::publishList4Sensors() {
    publishList3Sensors();
    if(l4Init) return;
    for(uint8_t i = 0; i < List4SensorCount; i++) {
        publishSensor(List4Sensors[i]);
    }
    l4Init = true;
}

void HomeAssistantMqttHandler::publishList4ExportSensors() {
    publishList3ExportSensors();
    if(l4eInit) return;
    for(uint8_t i = 0; i < List4ExportSensorCount; i++) {
        publishSensor(List4ExportSensors[i]);
    }
    l4eInit = true;
}

void HomeAssistantMqttHandler::publishRealtimeSensors(EnergyAccounting* ea, PriceService* ps) {
    if(rtInit) return;
    for(uint8_t i = 0; i < RealtimeSensorCount; i++) {
        HomeAssistantSensor sensor = RealtimeSensors[i];
        if(strncmp_P(sensor.devcl, PSTR("monetary"), 8) == 0) {
            if(ps == NULL) continue;
            sensor.uom = ps->getCurrency();
        }
        publishSensor(sensor);
    }
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 0; i < peakCount; i++) {
        char name[strlen(RealtimePeakSensor.name)];
        snprintf(name, strlen(RealtimePeakSensor.name), RealtimePeakSensor.name, i+1);
        char path[strlen(RealtimePeakSensor.path)];
        snprintf(path, strlen(RealtimePeakSensor.path), RealtimePeakSensor.path, i);
        HomeAssistantSensor sensor = {
            name,
            RealtimePeakSensor.topic,
            path,
            RealtimePeakSensor.ttl,
            RealtimePeakSensor.uom,
            RealtimePeakSensor.devcl,
            RealtimePeakSensor.stacl,
            RealtimePeakSensor.uid
        };
        publishSensor(sensor);
    }
    rtInit = true;
}

void HomeAssistantMqttHandler::publishRealtimeExportSensors(EnergyAccounting* ea, PriceService* ps) {
    if(rteInit) return;
    for(uint8_t i = 0; i < RealtimeExportSensorCount; i++) {
        HomeAssistantSensor sensor = RealtimeExportSensors[i];
        if(strncmp_P(sensor.devcl, PSTR("monetary"), 8) == 0) {
            if(ps == NULL) continue;
            sensor.uom = ps->getCurrency();
        }
        publishSensor(sensor);
    }
    rteInit = true;
}

void HomeAssistantMqttHandler::publishTemperatureSensor(uint8_t index, String id) {
    if(index > 32) return;
    if(tInit[index]) return;
    char name[strlen(TemperatureSensor.name)+id.length()];
    snprintf(name, strlen(TemperatureSensor.name)+id.length(), TemperatureSensor.name, id.c_str());

    char path[strlen(TemperatureSensor.path)+id.length()];
    if(index == 0) {
        memcpy_P(path, PSTR("temp\0"), 5);
    } else {
        snprintf(path, strlen(TemperatureSensor.path)+id.length(), TemperatureSensor.path, id.c_str());
    }
    HomeAssistantSensor sensor = {
        name,
        index == 0 ? SystemSensors[0].topic : TemperatureSensor.topic,
        path,
        TemperatureSensor.ttl,
        TemperatureSensor.uom,
        TemperatureSensor.devcl,
        TemperatureSensor.stacl,
        TemperatureSensor.uid
    };
    publishSensor(sensor);
    tInit[index] = true;
}

void HomeAssistantMqttHandler::publishPriceSensors(PriceService* ps) {
    if(ps == NULL) return;
    String uom = String(ps->getCurrency()) + "/kWh";

    if(!pInit) {
        for(uint8_t i = 0; i < PriceSensorCount; i++) {
            HomeAssistantSensor sensor = PriceSensors[i];
            if(strncmp_P(sensor.devcl, PSTR("monetary"), 8) == 0) {
                sensor.uom = uom.c_str();
            }
            publishSensor(sensor);
        }
        pInit = true;
    }

    uint8_t currentPricePointIndex = ps->getCurrentPricePointIndex();
	uint8_t numberOfPoints = ps->getNumberOfPointsAvailable();

    if(priceImportInit < numberOfPoints-currentPricePointIndex) {
        uint8_t importPriceSensorNo = 0;
        for(int pricePointIndex = currentPricePointIndex; pricePointIndex < numberOfPoints; pricePointIndex++) {
            float val = ps->getPricePoint(PRICE_DIRECTION_IMPORT, pricePointIndex);
            if(val == PRICE_NO_VALUE) break;
            if(importPriceSensorNo < priceImportInit) {
                importPriceSensorNo++;
                continue;
            }

            uint8_t resolution = ps->getResolutionInMinutes();

            char path[64];
            memset(path, 0, 64);
            snprintf_P(path, 64, PSTR("prices.import[%d]"), importPriceSensorNo);

            char uid[32];
            memset(uid, 0, 32);
            snprintf_P(uid, 32, PSTR("prices%d"), importPriceSensorNo);

            char name[64];
            if(resolution == 60) 
                snprintf_P(name, 64, PSTR("Import price in %02d hour%s"), importPriceSensorNo, importPriceSensorNo == 1 ? "" : "s");
            else    
                snprintf_P(name, 64, PSTR("Import price in %03d minutes"), importPriceSensorNo * resolution);

            HomeAssistantSensor sensor = {
                importPriceSensorNo == 0 ? "Current import price" : name,
                "/prices",
                path,
                resolution * 60 + 300,
                uom.c_str(),
                "monetary",
                importPriceSensorNo == 0 ? "total" : "",
                uid
            };
            publishSensor(sensor);

            priceImportInit = importPriceSensorNo++;
        }
    }

    if(priceExportInit < numberOfPoints-currentPricePointIndex) {
        uint8_t exportPriceSensorNo = 0;
        for(int pricePointIndex = currentPricePointIndex; pricePointIndex < numberOfPoints; pricePointIndex++) {
            float val = ps->getPricePoint(PRICE_DIRECTION_EXPORT, pricePointIndex);
            if(val == PRICE_NO_VALUE) break;
            if(exportPriceSensorNo < priceExportInit) {
                exportPriceSensorNo++;
                continue;
            }

            uint8_t resolution = ps->getResolutionInMinutes();

            char path[64];
            memset(path, 0, 64);
            snprintf_P(path, 64, PSTR("prices.export[%d]"), exportPriceSensorNo);

            char uid[32];
            memset(uid, 0, 32);
            snprintf_P(uid, 32, PSTR("exportprices%d"), exportPriceSensorNo);

                        char name[64];
            if(resolution == 60) 
                snprintf_P(name, 64, PSTR("Export price in %02d hour%s"), exportPriceSensorNo, exportPriceSensorNo == 1 ? "" : "s");
            else    
                snprintf_P(name, 64, PSTR("Export price in %03d minutes"), exportPriceSensorNo * resolution);

            HomeAssistantSensor sensor = {
                exportPriceSensorNo == 0 ? "Current export price" : name,
                "/prices",
                path,
                resolution * 60 + 300,
                uom.c_str(),
                "monetary",
                exportPriceSensorNo == 0 ? "total" : "",
                uid
            };
            publishSensor(sensor);

            priceExportInit = exportPriceSensorNo++;
        }
    }
}


void HomeAssistantMqttHandler::publishSystemSensors() {
    if(sInit) return;
    for(uint8_t i = 0; i < SystemSensorCount; i++) {
        publishSensor(SystemSensors[i]);
    }
    sInit = true;
}

void HomeAssistantMqttHandler::publishThresholdSensors() {
    if(rInit) return;
    for(uint8_t i = 0; i < 9; i++) {
        char name[strlen(RealtimeThresholdSensor.name)+1];
        snprintf(name, strlen(RealtimeThresholdSensor.name)+2, RealtimeThresholdSensor.name, i+1);
        char path[strlen(RealtimeThresholdSensor.path)+1];
        snprintf(path, strlen(RealtimeThresholdSensor.path)+1, RealtimeThresholdSensor.path, i);
        HomeAssistantSensor sensor = {
            name,
            RealtimeThresholdSensor.topic,
            path,
            RealtimeThresholdSensor.ttl,
            RealtimeThresholdSensor.uom,
            RealtimeThresholdSensor.devcl,
            RealtimeThresholdSensor.stacl,
            RealtimeThresholdSensor.uid
        };
        publishSensor(sensor);
    }
    rInit = true;
}

uint8_t HomeAssistantMqttHandler::getFormat() {
    return 4;
}

bool HomeAssistantMqttHandler::publishRaw(String data) {
    return false;
}

bool HomeAssistantMqttHandler::publishFirmware() {
    if(!fInit) {
        snprintf_P(json, BufferSize, PSTR("{\"name\":\"%sFirmware\",\"stat_t\":\"%s/firmware\",\"uniq_id\":\"%s_fwupgrade\",\"dev_cla\":\"firmware\",\"cmd_t\":\"%s\",\"pl_inst\":\"fwupgrade\"}"),
            sensorNamePrefix.c_str(),
            pubTopic.c_str(),
            deviceUid.c_str(),
            subTopic.c_str()
        );
        fInit = mqtt.publish(updateTopic + "/" + deviceUid + "/config", json, true, 0);
        loop();
        return fInit;
    }
    snprintf_P(json, BufferSize, PSTR("{\"installed_version\":\"%s\",\"latest_version\":\"%s\",\"title\":\"amsreader firmware\",\"release_url\":\"https://github.com/UtilitechAS/amsreader-firmware/releases\",\"release_summary\":\"New version %s is available\",\"update_percentage\":%s}"),
        FirmwareVersion::VersionString,
        strlen(updater->getNextVersion()) == 0 ? FirmwareVersion::VersionString : updater->getNextVersion(),
        strlen(updater->getNextVersion()) == 0 ? FirmwareVersion::VersionString : updater->getNextVersion(),
        updater->getProgress() < 0 ? "null" : String(updater->getProgress(), 0)
    );
    bool ret = mqtt.publish(pubTopic + "/firmware", json);
    loop();
    return ret;
}

void HomeAssistantMqttHandler::onMessage(String &topic, String &payload) {
    if(topic.equals(statusTopic)) {
        if(payload.equals("online")) {
 			#if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            debugger->printf_P(PSTR("Received online status from HA, resetting sensor status\n"));
            l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = rInit = false;
            for(uint8_t i = 0; i < 32; i++) tInit[i] = false;
            priceImportInit = 0;
            priceExportInit = 0;
        }
    } else if(topic.equals(subTopic)) {
        if(payload.equals("fwupgrade")) {
            if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                updater->setTargetVersion(updater->getNextVersion());
            }
        }
    }
}
