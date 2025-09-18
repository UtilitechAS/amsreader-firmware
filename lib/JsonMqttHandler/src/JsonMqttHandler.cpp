/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "JsonMqttHandler.h"
#include "FirmwareVersion.h"
#include "hexutils.h"
#include "Uptime.h"

bool JsonMqttHandler::publish(AmsData* update, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) {
    if(strlen(mqttConfig.publishTopic) == 0) {
        return false;
    }
	if(!mqtt.connected()) {
		return false;
    }

    bool ret = false;
    memset(json, 0, BufferSize);

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

    if(data.getListType() == 1) {
        ret = publishList1(&data, ea);
        mqtt.loop();
    } else if(data.getListType() == 2) {
        ret = publishList2(&data, ea);
        mqtt.loop();
    } else if(data.getListType() == 3) {
        ret = publishList3(&data, ea);
        mqtt.loop();
    } else if(data.getListType() == 4) {
        ret = publishList4(&data, ea);
        mqtt.loop();
    }

    if(data.getListType() >= 2 && data.getActiveExportPower() > 0.0) {
        hasExport = true;
    }

    if(data.getListType() >= 3 && data.getActiveExportCounter() > 0.0) {
        hasExport = true;
    }

    loop();
    return ret;
}

uint16_t JsonMqttHandler::appendJsonHeader(AmsData* data) {
    return snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\",\"name\":\"%s\",\"up\":%u,\"t\":%lu,\"vcc\":%.3f,\"rssi\":%d,\"temp\":%.2f,"),
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        data->getPackageTimestamp(),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature()
    );
}

uint16_t JsonMqttHandler::appendJsonFooter(EnergyAccounting* ea, uint16_t pos) {
    char pf[4];
    if(mqttConfig.payloadFormat == 6) {
        strcpy_P(pf, PSTR("rt_"));
    } else {
        memset(pf, 0, 4);
    }
    
    return snprintf_P(json+pos, BufferSize-pos, PSTR("%s\"%sh\":%.2f,\"%sd\":%.1f,\"%st\":%d,\"%sx\":%.2f,\"%she\":%.2f,\"%sde\":%.1f%s"),
        strlen(pf) == 0 ? "},\"realtime\":{" : ",",
        pf,
        ea->getUseThisHour(),
        pf,
        ea->getUseToday(),
        pf,
        ea->getCurrentThreshold(),
        pf,
        ea->getMonthMax(),
        pf,
        ea->getProducedThisHour(),
        pf,
        ea->getProducedToday(),
        strlen(pf) == 0 ? "}" : ""
    );
}

bool JsonMqttHandler::publishList1(AmsData* data, EnergyAccounting* ea) {
    uint16_t pos = appendJsonHeader(data);
    if(mqttConfig.payloadFormat != 6) {
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"P\":%d"), data->getActiveImportPower());
    pos += appendJsonFooter(ea, pos);
    json[pos++] = '}';
    json[pos] = '\0';
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/list1"), mqttConfig.publishTopic);
        return mqtt.publish(topic, json);
    } else {
        return mqtt.publish(mqttConfig.publishTopic, json);
    }
}

bool JsonMqttHandler::publishList2(AmsData* data, EnergyAccounting* ea) {
    uint16_t pos = appendJsonHeader(data);
    if(mqttConfig.payloadFormat != 6) {
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"Q\":%d,\"PO\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f"),
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
    pos += appendJsonFooter(ea, pos);
    json[pos++] = '}';
    json[pos] = '\0';
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/list2"), mqttConfig.publishTopic);
        return mqtt.publish(topic, json);
    } else {
        return mqtt.publish(mqttConfig.publishTopic, json);
    }
}

bool JsonMqttHandler::publishList3(AmsData* data, EnergyAccounting* ea) {
    uint16_t pos = appendJsonHeader(data);
    if(mqttConfig.payloadFormat != 6) {
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"Q\":%d,\"PO\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f,\"tPI\":%.3f,\"tPO\":%.3f,\"tQI\":%.3f,\"tQO\":%.3f,\"rtc\":%lu"),
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
        data->getMeterTimestamp()
    );
    pos += appendJsonFooter(ea, pos);
    json[pos++] = '}';
    json[pos] = '\0';
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/list3"), mqttConfig.publishTopic);
        return mqtt.publish(topic, json);
    } else {
        return mqtt.publish(mqttConfig.publishTopic, json);
    }
}

bool JsonMqttHandler::publishList4(AmsData* data, EnergyAccounting* ea) {
    uint16_t pos = appendJsonHeader(data);
    if(mqttConfig.payloadFormat != 6) {
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"P1\":%d,\"P2\":%d,\"P3\":%d,\"Q\":%d,\"PO\":%d,\"PO1\":%d,\"PO2\":%d,\"PO3\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f,\"PF\":%.2f,\"PF1\":%.2f,\"PF2\":%.2f,\"PF3\":%.2f,\"tPI\":%.3f,\"tPO\":%.3f,\"tQI\":%.3f,\"tQO\":%.3f,\"tPI1\":%.3f,\"tPI2\":%.3f,\"tPI3\":%.3f,\"tPO1\":%.3f,\"tPO2\":%.3f,\"tPO3\":%.3f,\"rtc\":%lu"),
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
        data->getL1ActiveImportCounter(),
        data->getL2ActiveImportCounter(),
        data->getL3ActiveImportCounter(),
        data->getL1ActiveExportCounter(),
        data->getL2ActiveExportCounter(),
        data->getL3ActiveExportCounter(),
        data->getMeterTimestamp()
    );
    pos += appendJsonFooter(ea, pos);
    json[pos++] = '}';
    json[pos] = '\0';
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/list4"), mqttConfig.publishTopic);
        return mqtt.publish(topic, json);
    } else {
        return mqtt.publish(mqttConfig.publishTopic, json);
    }
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

	uint16_t pos = 0;
    if(mqttConfig.payloadFormat == 6) {
        json[pos++] = '{';
    } else {
        pos = snprintf_P(json, 24, PSTR("{\"temperatures\":{"));
    }
	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL) {
            pos += snprintf_P(json+pos, 26, PSTR("\"%s\":%.2f,"), 
                toHex(data->address, 8).c_str(),
                data->lastRead
            );
            data->changed = false;
        }
	}
    bool ret = false;
    json[pos-1] = '}';
    if(mqttConfig.payloadFormat != 6) {
        json[pos++] = '}';
        json[pos] = '\0';
    }
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/temperatures"), mqttConfig.publishTopic);
        ret = mqtt.publish(topic, json);
    } else {
        ret = mqtt.publish(mqttConfig.publishTopic, json);
    }
    loop();
    return ret;
}

bool JsonMqttHandler::publishPrices(PriceService* ps) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;
	if(!ps->hasPrice())
		return false;

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

    char pf[4];
    uint16_t pos = snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\","), WiFi.macAddress().c_str());
    uint8_t numberOfPoints = ps->getNumberOfPointsAvailable();
    if(mqttConfig.payloadFormat != 6) {
        memset(pf, 0, 4);
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"prices\":{\"import\":{\"current\":%.4f,\"all\":["), ps->getCurrentPrice(PRICE_DIRECTION_IMPORT));
        for(int i = 0; i < numberOfPoints; i++) {
            float val = ps->getPricePoint(PRICE_DIRECTION_IMPORT, i);
            if(val == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("null,"));
            } else {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("%.4f,"), val);
            }
        }
        if(hasExport && ps->isExportPricesDifferentFromImport()) {
            pos--;
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("]},\"export\":{\"current\":%.4f,\"all\":["), ps->getCurrentPrice(PRICE_DIRECTION_EXPORT));
            for(int i = 0; i < numberOfPoints; i++) {
                float val = ps->getPricePoint(PRICE_DIRECTION_EXPORT, i);
                if(val == PRICE_NO_VALUE) {
                    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("null,"));
                } else {
                    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("%.4f,"), val);
                }
            }
        }
        pos--;
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("]},"));
    } else {
        strcpy_P(pf, PSTR("pr_"));
        float val = ps->getCurrentPrice(PRICE_DIRECTION_IMPORT);
        if(val == PRICE_NO_VALUE) {
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%sc\":null,"), pf);
        } else {
            pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%sc\":%.4f,"), pf, val);
        }
        for(uint8_t i = 0;i < numberOfPoints; i++) {
            val = ps->getPricePoint(PRICE_DIRECTION_IMPORT, i);
            if(val == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%s%02d\":null,"), pf, i);
            } else {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%s%02d\":%.4f,"), pf, i, val);
            }
        }
        if(hasExport && ps->isExportPricesDifferentFromImport()) {
            float val = ps->getCurrentPrice(PRICE_DIRECTION_EXPORT);
            if(val == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%sec\":null,"), pf);
            } else {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%sec\":%.4f,"), pf, val);
            }
            for(uint8_t i = 0;i < numberOfPoints; i++) {
                val = ps->getPricePoint(PRICE_DIRECTION_EXPORT, i);
                if(val == PRICE_NO_VALUE) {
                    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%se%02d\":null,"), pf, i);
                } else {
                    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%se%02d\":%.4f,"), pf, i, val);
                }
            }
        }
    }


    pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"%smin\":%.4f,\"%smax\":%.4f,\"%scheapest1hr\":\"%s\",\"%scheapest3hr\":\"%s\",\"%scheapest6hr\":\"%s\"}"),
        pf,
        min == INT16_MAX ? 0.0 : min,
        pf,
        max == INT16_MIN ? 0.0 : max,
        pf,
        ts1hr,
        pf,
        ts3hr,
        pf,
        ts6hr
    );
    if(mqttConfig.payloadFormat != 6) {
        json[pos++] = '}';
        json[pos] = '\0';
    }
    bool ret = false;
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/prices"), mqttConfig.publishTopic);
        ret = mqtt.publish(topic, json);
    } else {
        ret = mqtt.publish(mqttConfig.publishTopic, json);
    }
    loop();
    return ret;
}

bool JsonMqttHandler::publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;

    snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\",\"name\":\"%s\",\"up\":%d,\"vcc\":%.3f,\"rssi\":%d,\"temp\":%.2f,\"version\":\"%s\"}"),
        WiFi.macAddress().c_str(),
        mqttConfig.clientId,
        (uint32_t) (millis64()/1000),
        hw->getVcc(),
        hw->getWifiRssi(),
        hw->getTemperature(),
        FirmwareVersion::VersionString
    );
    bool ret = false;
    if(mqttConfig.payloadFormat == 5) {
        char topic[192];
        snprintf_P(topic, 192, PSTR("%s/system"), mqttConfig.publishTopic);
        ret = mqtt.publish(topic, json);
    } else {
        ret = mqtt.publish(mqttConfig.publishTopic, json);
    }
    loop();
    return ret;
}

uint8_t JsonMqttHandler::getFormat() {
    return 0;
}

bool JsonMqttHandler::publishRaw(String data) {
    return false;
}

bool JsonMqttHandler::publishFirmware() {
    snprintf_P(json, BufferSize, PSTR("{\"installed_version\":\"%s\",\"latest_version\":\"%s\",\"title\":\"amsreader firmware\",\"release_url\":\"https://github.com/UtilitechAS/amsreader-firmware/releases\",\"release_summary\":\"New version %s is available\",\"update_percentage\":%s}"),
        FirmwareVersion::VersionString,
        strlen(updater->getNextVersion()) == 0 ? FirmwareVersion::VersionString : updater->getNextVersion(),
        strlen(updater->getNextVersion()) == 0 ? FirmwareVersion::VersionString : updater->getNextVersion(),
        updater->getProgress() < 0 ? "null" : String(updater->getProgress(), 0)
    );
    char topic[192];
    snprintf_P(topic, 192, PSTR("%s/firmware"), mqttConfig.publishTopic);
    bool ret = mqtt.publish(topic, json);
    loop();
    return ret;
}

void JsonMqttHandler::onMessage(String &topic, String &payload) {
    if(strncmp(topic.c_str(), mqttConfig.subscribeTopic, 12) == 0) {
        if(payload.equals("fwupgrade")) {
            if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                updater->setTargetVersion(updater->getNextVersion());
            }
        }
    }
}
