#include "HomeAssistantMqttHandler.h"
#include "hexutils.h"
#include "Uptime.h"
#include "FirmwareVersion.h"
#include "json/ha1_json.h"
#include "json/ha2_json.h"
#include "json/ha3_json.h"
#include "json/ha4_json.h"
#include "json/jsonsys_json.h"
#include "json/jsonprices_json.h"
#include "json/hadiscover_json.h"
#include "json/realtime_json.h"
#include "FirmwareVersion.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

bool HomeAssistantMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi) {
	if(topic.isEmpty() || !mqtt.connected())
		return false;

    if(time(nullptr) < FirmwareVersion::BuildEpoch)
        return false;

    if(data->getListType() >= 3) { // publish energy counts
        publishList3(data, ea);
        loop();
    }

    if(data->getListType() == 1) { // publish power counts
        publishList1(data, ea);
    } else if(data->getListType() <= 3) { // publish power counts and volts/amps
        publishList2(data, ea);
    } else if(data->getListType() == 4) { // publish power counts and volts/amps/phase power and PF
        publishList4(data, ea);
    }
    loop();

    if(ea->isInitialized()) {
        publishRealtime(data, ea, eapi);
        loop();
    }
    return true;
}

bool HomeAssistantMqttHandler::publishList1(AmsData* data, EnergyAccounting* ea) {
    publishList1Sensors();
    snprintf_P(json, BufferSize, HA1_JSON, data->getActiveImportPower());
    return mqtt.publish(topic + "/power", json);
}

bool HomeAssistantMqttHandler::publishList2(AmsData* data, EnergyAccounting* ea) {
    publishList2Sensors();
    if(data->getActiveExportPower() > 0) publishList2ExportSensors();
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
        data->getL3Voltage()
    );
    return mqtt.publish(topic + "/power", json);
}

bool HomeAssistantMqttHandler::publishList3(AmsData* data, EnergyAccounting* ea) {
    publishList3Sensors();
    if(data->getActiveExportCounter() > 0.0) publishList3ExportSensors();
    snprintf_P(json, BufferSize, HA2_JSON,
        data->getActiveImportCounter(),
        data->getActiveExportCounter(),
        data->getReactiveImportCounter(),
        data->getReactiveExportCounter(),
        data->getMeterTimestamp()
    );
    return mqtt.publish(topic + "/energy", json);
}

bool HomeAssistantMqttHandler::publishList4(AmsData* data, EnergyAccounting* ea) {
    publishList4Sensors();
    if(data->getL1ActiveExportPower() > 0 || data->getL2ActiveExportPower() > 0 || data->getL3ActiveExportPower() > 0) publishList4ExportSensors();
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
        data->getPowerFactor() == 0 ? 1 : data->getPowerFactor(),
        data->getPowerFactor() == 0 ? 1 : data->getL1PowerFactor(),
        data->getPowerFactor() == 0 ? 1 : data->getL2PowerFactor(),
        data->getPowerFactor() == 0 ? 1 : data->getL3PowerFactor()
    );
    return mqtt.publish(topic + "/power", json);
}

String HomeAssistantMqttHandler::getMeterModel(AmsData* data) {
    String meterModel = data->getMeterModel();
    meterModel.replace("\\", "\\\\");
    return meterModel;
}

bool HomeAssistantMqttHandler::publishRealtime(AmsData* data, EnergyAccounting* ea, EntsoeApi* eapi) {
    publishRealtimeSensors(ea, eapi);
    if(ea->getProducedThisHour() > 0.0 || ea->getProducedToday() > 0.0 || ea->getProducedThisMonth() > 0.0) publishRealtimeExportSensors(ea, eapi);
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
    return mqtt.publish(topic + "/realtime", json);
}


bool HomeAssistantMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
	int count = hw->getTempSensorCount();
    if(count < 2) return false;

	int size = 32 + (count * 26);

	char buf[size];
	snprintf_P(buf, 24, PSTR("{\"temperatures\":{"));

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            char* pos = buf+strlen(buf);
            String id = toHex(data->address, 8);
            snprintf_P(pos, 26, PSTR("\"%s\":%.2f,"), 
                id.c_str(),
                data->lastRead
            );
            data->changed = false;
            publishTemperatureSensor(i+1, id);
        }
	}
	char* pos = buf+strlen(buf);
	snprintf_P(count == 0 ? pos : pos-1, 8, PSTR("}}"));
    bool ret = mqtt.publish(topic + "/temperatures", buf);
    loop();
    return ret;
}

bool HomeAssistantMqttHandler::publishPrices(EntsoeApi* eapi) {
	if(topic.isEmpty() || !mqtt.connected())
		return false;
	if(eapi->getValueForHour(0) == ENTSOE_NO_VALUE)
		return false;

    publishPriceSensors(eapi);

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
    bool ret = mqtt.publish(topic + "/prices", json, true, 0);
    loop();
    return ret;
}

bool HomeAssistantMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt.connected())
		return false;

    publishSystemSensors();
    if(hw->getTemperature() > -50) publishTemperatureSensor(0, "");

    snprintf_P(json, BufferSize, JSONSYS_JSON,
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        FirmwareVersion::VersionString
    );
    bool ret = mqtt.publish(topic + "/state", json);
    loop();
    return ret;
}

void HomeAssistantMqttHandler::publishSensor(const HomeAssistantSensor sensor) {
    String uid = String(sensor.path);
    uid.replace(".", "");
    uid.replace("[", "");
    uid.replace("]", "");
    uid.replace("'", "");
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
    mqtt.publish(discoveryTopic + deviceUid + "_" + uid.c_str() + "/config", json, true, 0);
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

void HomeAssistantMqttHandler::publishRealtimeSensors(EnergyAccounting* ea, EntsoeApi* eapi) {
    if(rtInit) return;
    for(uint8_t i = 0; i < RealtimeSensorCount; i++) {
        HomeAssistantSensor sensor = RealtimeSensors[i];
        if(strncmp_P(sensor.devcl, PSTR("monetary"), 8) == 0) {
            if(eapi == NULL) continue;
            sensor.uom = eapi->getCurrency();
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
            RealtimePeakSensor.stacl
        };
        publishSensor(sensor);
    }
    rtInit = true;
}

void HomeAssistantMqttHandler::publishRealtimeExportSensors(EnergyAccounting* ea, EntsoeApi* eapi) {
    if(rteInit) return;
    for(uint8_t i = 0; i < RealtimeExportSensorCount; i++) {
        HomeAssistantSensor sensor = RealtimeExportSensors[i];
        if(strncmp_P(sensor.devcl, PSTR("monetary"), 8) == 0) {
            if(eapi == NULL) continue;
            sensor.uom = eapi->getCurrency();
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
        TemperatureSensor.stacl
    };
    publishSensor(sensor);
    tInit[index] = true;
}

void HomeAssistantMqttHandler::publishPriceSensors(EntsoeApi* eapi) {
    if(eapi == NULL) return;
    String uom = String(eapi->getCurrency()) + "/kWh";

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
    for(uint8_t i = 0; i < 38; i++) {
        if(prInit[i]) continue;
        float val = eapi->getValueForHour(i);
        if(val == ENTSOE_NO_VALUE) continue;
        
        char name[strlen(PriceSensor.name)+2];
        snprintf(name, strlen(PriceSensor.name)+2, PriceSensor.name, i, i == 1 ? "hour" : "hours");
        char path[strlen(PriceSensor.path)+1];
        snprintf(path, strlen(PriceSensor.path)+1, PriceSensor.path, i);
        HomeAssistantSensor sensor = {
            i == 0 ? "Price current hour" : name,
            PriceSensor.topic,
            path,
            PriceSensor.ttl,
            uom.c_str(),
            PriceSensor.devcl,
            i == 0 ? "total" : PriceSensor.stacl
        };
        publishSensor(sensor);
        prInit[i] = true;
    }

}

void HomeAssistantMqttHandler::publishSystemSensors() {
    if(sInit) return;
    for(uint8_t i = 0; i < SystemSensorCount; i++) {
        publishSensor(SystemSensors[i]);
    }
    sInit = true;
}

uint8_t HomeAssistantMqttHandler::getFormat() {
    return 4;
}

bool HomeAssistantMqttHandler::publishRaw(String data) {
    return false;
}

void HomeAssistantMqttHandler::onMessage(String &topic, String &payload) {
    if(topic.equals(statusTopic)) {
        if(payload.equals("online")) {
 			if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Received online status from HA, resetting sensor status\n"));
            l1Init = l2Init = l2eInit = l3Init = l3eInit = l4Init = l4eInit = rtInit = rteInit = pInit = sInit = false;
            for(uint8_t i = 0; i < 32; i++) tInit[i] = false;
            for(uint8_t i = 0; i < 38; i++) prInit[i] = false;
        }
    }
}
