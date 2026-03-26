/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "JsonMqttHandler.h"
#include "FirmwareVersion.h"
#include "hexutils.h"
#include "Uptime.h"
#include "AmsJsonGenerator.h"

bool JsonMqttHandler::publish(AmsData* update, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) {
    if(strlen(mqttConfig.publishTopic) == 0) {
        return false;
    }
	if(!connected()) {
		return false;
    }

    bool ret = false;
    memset(json, 0, BUF_SIZE_COMMON);

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
    return snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"id\":\"%s\",\"name\":\"%s\",\"up\":%u,\"t\":%lu,\"vcc\":%.3f,\"rssi\":%d,\"temp\":%.2f,"),
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

    String peaks = "";
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 1; i <= peakCount; i++) {
        if(!peaks.isEmpty()) peaks += ",";
        peaks += String(ea->getPeak(i).value / 100.0, 2);
    }
    
    return snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("%s\"%sh\":%.3f,\"%sd\":%.2f,\"%sm\":%.1f,\"%st\":%d,\"%sx\":%.2f,\"%she\":%.3f,\"%sde\":%.2f,\"%sme\":%.1f,\"peaks\":[%s]%s"),
        strlen(pf) == 0 ? "},\"realtime\":{" : ",",
        pf,
        ea->getUseThisHour(),
        pf,
        ea->getUseToday(),
        pf,
        ea->getUseThisMonth(),
        pf,
        ea->getCurrentThreshold(),
        pf,
        ea->getMonthMax(),
        pf,
        ea->getProducedThisHour(),
        pf,
        ea->getProducedToday(),
        pf,
        ea->getProducedThisMonth(),
        peaks.c_str(),
        strlen(pf) == 0 ? "}" : ""
    );
}

bool JsonMqttHandler::publishList1(AmsData* data, EnergyAccounting* ea) {
    uint16_t pos = appendJsonHeader(data);
    if(mqttConfig.payloadFormat != 6) {
        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"P\":%d"), data->getActiveImportPower());
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
        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"Q\":%d,\"PO\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f"),
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
        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"Q\":%d,\"PO\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f,\"tPI\":%.3f,\"tPO\":%.3f,\"tQI\":%.3f,\"tQO\":%.3f,\"rtc\":%lu"),
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
        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"data\":{"));
    }
    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"lv\":\"%s\",\"meterId\":\"%s\",\"type\":\"%s\",\"P\":%d,\"P1\":%d,\"P2\":%d,\"P3\":%d,\"Q\":%d,\"PO\":%d,\"PO1\":%d,\"PO2\":%d,\"PO3\":%d,\"QO\":%d,\"I1\":%.2f,\"I2\":%.2f,\"I3\":%.2f,\"U1\":%.2f,\"U2\":%.2f,\"U3\":%.2f,\"PF\":%.2f,\"PF1\":%.2f,\"PF2\":%.2f,\"PF3\":%.2f,\"tPI\":%.3f,\"tPO\":%.3f,\"tQI\":%.3f,\"tQO\":%.3f,\"tPI1\":%.3f,\"tPI2\":%.3f,\"tPI3\":%.3f,\"tPO1\":%.3f,\"tPO2\":%.3f,\"tPO3\":%.3f,\"rtc\":%lu"),
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
	if(strlen(mqttConfig.publishTopic) == 0 || !connected())
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
        tm.Minute = 0;
        tm.Second = 0;
        ts = makeTime(tm);
        toJsonIsoTimestamp(ts, ts1hr, sizeof(ts1hr));
	}
	char ts3hr[24];
    memset(ts3hr, 0, 24);
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
        tmElements_t tm;
        breakTime(ts, tm);
        tm.Minute = 0;
        tm.Second = 0;
        ts = makeTime(tm);
        toJsonIsoTimestamp(ts, ts3hr, sizeof(ts3hr));
	}
	char ts6hr[24];
    memset(ts6hr, 0, 24);
	if(min6hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min6hrIdx);
        tmElements_t tm;
        breakTime(ts, tm);
        tm.Minute = 0;
        tm.Second = 0;
        ts = makeTime(tm);
        toJsonIsoTimestamp(ts, ts6hr, sizeof(ts6hr));
	}

    if(mqttConfig.payloadFormat == 6) {
        uint16_t pos = snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"id\":\"%s\","), WiFi.macAddress().c_str());

        for(uint8_t i = 0;i < 38; i++) {
            if(values[i] == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"pr_%d\":null,"), i);
            } else {
                pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"pr_%d\":%.4f,"), i, values[i]);
            }
        }

        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("\"pr_min\":%.4f,\"pr_max\":%.4f,\"pr_cheapest1hr\":%s,\"pr_cheapest3hr\":%s,\"pr_cheapest6hr\":%s}"),
            min == INT16_MAX ? 0.0 : min,
            max == INT16_MIN ? 0.0 : max,
            ts1hr,
            ts3hr,
            ts6hr
        );
    } else {
        uint16_t pos = snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"id\":\"%s\",\"prices\":{\"import\":["), WiFi.macAddress().c_str());

        uint8_t currentPricePointIndex = ps->getCurrentPricePointIndex();
        uint8_t numberOfPoints = ps->getNumberOfPointsAvailable();
        for(int i = currentPricePointIndex; i < numberOfPoints; i++) {
            float val = ps->getPricePoint(PRICE_DIRECTION_IMPORT, i);
            if(val == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("null,"));
            } else {
                pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("%.4f,"), val);
            }
        }
        if(hasExport && ps->isExportPricesDifferentFromImport()) {
            pos--;
            pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("],\"export\":["));
            for(int i = currentPricePointIndex; i < numberOfPoints; i++) {
                float val = ps->getPricePoint(PRICE_DIRECTION_EXPORT, i);
                if(val == PRICE_NO_VALUE) {
                    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("null,"));
                } else {
                    pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("%.4f,"), val);
                }
            }
        }

        pos--;
        pos += snprintf_P(json+pos, BUF_SIZE_COMMON-pos, PSTR("],\"min\":%.4f,\"max\":%.4f,\"cheapest1hr\":%s,\"cheapest3hr\":%s,\"cheapest6hr\":%s}}"),
            min == INT16_MAX ? 0.0 : min,
            max == INT16_MIN ? 0.0 : max,
            ts1hr,
            ts3hr,
            ts6hr
        );
        
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
	if(strlen(mqttConfig.publishTopic) == 0 || !connected())
		return false;

    snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"id\":\"%s\",\"name\":\"%s\",\"up\":%d,\"vcc\":%.3f,\"rssi\":%d,\"temp\":%.2f,\"version\":\"%s\"}"),
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

bool JsonMqttHandler::publishRaw(uint8_t* raw, size_t length) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;
    
    if(length <= 0 || length > BUF_SIZE_COMMON) return false;

    String str = toHex(raw, length);

    snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"data\":\"%s\"}"), str.c_str());
    char topic[192];
    snprintf_P(topic, 192, PSTR("%s/data"), mqttConfig.publishTopic);
    bool ret = mqtt.publish(topic, json);
    loop();
    return ret;
}

bool JsonMqttHandler::publishFirmware() {
    snprintf_P(json, BUF_SIZE_COMMON, PSTR("{\"installed_version\":\"%s\",\"latest_version\":\"%s\",\"title\":\"amsreader firmware\",\"release_url\":\"https://github.com/UtilitechAS/amsreader-firmware/releases\",\"release_summary\":\"New version %s is available\",\"update_percentage\":%s}"),
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
    if(strlen(mqttConfig.publishTopic) == 0 || !connected())
		return;

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Received command [%s] to [%s]\n"), payload.c_str(), topic.c_str());

    if(topic == subTopic) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
        debugger->printf_P(PSTR(" - this is our subscribed topic\n"));

        if(payload.startsWith("{")) {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, payload);
            if(error) {
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::ERROR))
                #endif
                debugger->printf_P(PSTR(" - failed to parse JSON: %s\n"), error.c_str());
                return;
            } else {
                JsonObject obj = doc.as<JsonObject>();
                if(obj.containsKey(F("action"))) {
                    const char* action = obj[F("action")];
                    if(strcmp_P(action, PSTR("fwupgrade")) == 0) {
                        if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                            updater->setTargetVersion(updater->getNextVersion());
                        }
                    } else if(strcmp_P(action, PSTR("dayplot")) == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/dayplot"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateDayPlotJson(ds, json, BUF_SIZE_COMMON);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp_P(action, PSTR("monthplot")) == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateMonthPlotJson(ds, json, BUF_SIZE_COMMON);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    #if defined(ESP32)
                    } else if(strcmp_P(action, PSTR("getconfig")) == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/config"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateConfigurationJson(config, json, BUF_SIZE_COMMON);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp_P(action, PSTR("setconfig")) == 0 && obj.containsKey(F("config"))) {
                        JsonObject configObj = obj[F("config")];
                        handleConfigMessage(configObj);
                    #endif
                    }
                }
            }
        } else if(payload.equals(F("fwupgrade"))) {
            if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                updater->setTargetVersion(updater->getNextVersion());
            }
        } else if(payload.equals(F("dayplot"))) {
            char pubTopic[192];
            snprintf_P(pubTopic, 192, PSTR("%s/dayplot"), mqttConfig.publishTopic);
            AmsJsonGenerator::generateDayPlotJson(ds, json, BUF_SIZE_COMMON);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
        } else if(payload.equals(F("monthplot"))) {
            char pubTopic[192];
            snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
            AmsJsonGenerator::generateMonthPlotJson(ds, json, BUF_SIZE_COMMON);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
        }
    }
}

#if defined(ESP32)
void JsonMqttHandler::handleConfigMessage(JsonObject& configObj) {
    // General
    if(configObj.containsKey(F("g"))) {
        JsonObject generalObj = configObj[F("g")];
        if(generalObj.containsKey(F("t"))) {
            NtpConfig ntpConfig;
            config->getNtpConfig(ntpConfig);
            strlcpy(ntpConfig.timezone, generalObj[F("t")], sizeof(ntpConfig.timezone));
            config->setNtpConfig(ntpConfig);
        }
        if(generalObj.containsKey(F("h"))) {
            NetworkConfig networkConfig;
            config->getNetworkConfig(networkConfig);
            strlcpy(networkConfig.hostname, generalObj[F("h")], sizeof(networkConfig.hostname));
            config->setNetworkConfig(networkConfig);
        }

        WebConfig webConfig;
        config->getWebConfig(webConfig);
        if(generalObj.containsKey(F("s"))) {
            webConfig.security = generalObj[F("s")];
        }
        if(webConfig.security > 0) {
            if(generalObj.containsKey(F("u"))) {
                strlcpy(webConfig.username, generalObj[F("u")], sizeof(webConfig.username));
            }
            // Check if password is provided and that it is not empty and is not equal *** (which is used in the UI to indicate that the password is set but not shown)
            if(generalObj.containsKey(F("p")) && strlen(generalObj[F("p")]) > 0 && strcmp(generalObj[F("p")], "***") != 0) {
                strlcpy(webConfig.password, generalObj[F("p")], sizeof(webConfig.password));
            }
        }
        if(generalObj.containsKey(F("c"))) {
            strlcpy(webConfig.context, generalObj[F("c")], sizeof(webConfig.context));
        }
        config->setWebConfig(webConfig);
    }

    // Meter
    if(configObj.containsKey(F("m"))) {
        JsonObject meterObj = configObj[F("m")];
        MeterConfig newConfig;
        config->getMeterConfig(newConfig);

        if(meterObj.containsKey(F("o"))) {
            newConfig.source = meterObj[F("o")];
        }
        if(meterObj.containsKey(F("a"))) {
            newConfig.parser = meterObj[F("a")];
        }
        if(meterObj.containsKey(F("b"))) {
            newConfig.baud = meterObj[F("b")];
        }
        if(meterObj.containsKey(F("p"))) {
            newConfig.parity = meterObj[F("p")];
        }
        if(meterObj.containsKey(F("i"))) {
            newConfig.invert = meterObj[F("i")];
        }
        if(meterObj.containsKey(F("s"))) {
            newConfig.bufferSize = (int) meterObj[F("s")] / 64; // convert from bytes to 64 byte blocks
        }
        if(meterObj.containsKey(F("d"))) {
            newConfig.distributionSystem = meterObj[F("d")];
        }
        if(meterObj.containsKey(F("f"))) {
            newConfig.mainFuse = meterObj[F("f")];
        }
        if(meterObj.containsKey(F("r"))) {
            newConfig.productionCapacity = meterObj[F("r")];
        }
        if(meterObj.containsKey(F("e"))) {
            JsonObject encryptionObj = meterObj[F("e")];
            if(encryptionObj.containsKey(F("e"))) {
                bool enabled = encryptionObj[F("e")];
                if(enabled) {
                    if(encryptionObj.containsKey(F("k"))) {
                        String encryptionKeyHex = encryptionObj[F("k")];
                        if(encryptionKeyHex.length() == 16) {
                           fromHex(newConfig.encryptionKey, encryptionKeyHex, 16);
                        }
                    }
                    if(encryptionObj.containsKey(F("a"))) {
                        String authenticationKeyHex = encryptionObj[F("a")];
                        if(authenticationKeyHex.length() == 16) {
                            fromHex(newConfig.authenticationKey, authenticationKeyHex, 16);
                        }
                    }
                } else {
                    memset(newConfig.encryptionKey, 0, sizeof(newConfig.encryptionKey));
                    memset(newConfig.authenticationKey, 0, sizeof(newConfig.authenticationKey));
                }
            }
        }
        if(meterObj.containsKey(F("m"))) {
            JsonObject multipliersObj = meterObj[F("m")];
            bool enabled = multipliersObj[F("e")];
            if(enabled) {
                if(multipliersObj.containsKey(F("w"))) {
                    newConfig.wattageMultiplier = multipliersObj[F("w")];
                }
                if(multipliersObj.containsKey(F("v"))) {
                    newConfig.voltageMultiplier = multipliersObj[F("v")];
                }
                if(multipliersObj.containsKey(F("a"))) {
                    newConfig.amperageMultiplier = multipliersObj[F("a")];
                }
                if(multipliersObj.containsKey(F("c"))) {
                    newConfig.accumulatedMultiplier = multipliersObj[F("c")];
                }
            } else {
                newConfig.wattageMultiplier = 1.0;
                newConfig.voltageMultiplier = 1.0;
                newConfig.amperageMultiplier = 1.0;
                newConfig.accumulatedMultiplier = 1.0;
            }
        }

        config->setMeterConfig(newConfig);
    }

    // Network
    if(configObj.containsKey(F("n"))) {
        NetworkConfig newConfig;
        config->getNetworkConfig(newConfig);

        JsonObject networkObj = configObj[F("n")];
        if(networkObj.containsKey(F("c"))) {
            newConfig.mode = networkObj[F("c")];
        }
        if(networkObj.containsKey(F("m"))) {
            if(strcmp_P(networkObj[F("m")], PSTR("dhcp")) == 0) {
                newConfig.mode = 1;
            } else if(strcmp_P(networkObj[F("m")], PSTR("static")) == 0) {
                newConfig.mode = 2;
            }
        }

        if(networkObj.containsKey(F("i"))) {
            strlcpy(newConfig.ip, networkObj[F("i")], sizeof(newConfig.ip));
        }
        if(networkObj.containsKey(F("s"))) {
            strlcpy(newConfig.subnet, networkObj[F("s")], sizeof(newConfig.subnet));
        }
        if(networkObj.containsKey(F("g"))) {
            strlcpy(newConfig.gateway, networkObj[F("g")], sizeof(newConfig.gateway));
        }
        if(networkObj.containsKey(F("d1"))) {
            strlcpy(newConfig.dns1, networkObj[F("d1")], sizeof(newConfig.dns1));
        }
        if(networkObj.containsKey(F("d2"))) {
            strlcpy(newConfig.dns2, networkObj[F("d2")], sizeof(newConfig.dns2));
        }
        if(networkObj.containsKey(F("d"))) {
            newConfig.mdns = networkObj[F("d")];
        }
        if(networkObj.containsKey(F("x"))) {
            newConfig.ipv6 = networkObj[F("x")];
        }
        config->setNetworkConfig(newConfig);

        NtpConfig ntpConfig;
        config->getNtpConfig(ntpConfig);
        if(networkObj.containsKey(F("n1"))) {
            strlcpy(ntpConfig.server, networkObj[F("n1")], sizeof(ntpConfig.server));
            config->setNtpConfig(ntpConfig);
        }
        if(networkObj.containsKey(F("h"))) {
            ntpConfig.dhcp = networkObj[F("h")];
            config->setNtpConfig(ntpConfig);
        }
        config->getNtpConfig(ntpConfig);
    }

    // WiFi
    if(configObj.containsKey(F("w"))) {
        NetworkConfig newConfig;
        config->getNetworkConfig(newConfig);

        if(newConfig.mode == 1 || newConfig.mode == 2) {
            JsonObject wifiObj = configObj[F("w")];
            if(wifiObj.containsKey(F("s"))) {
                strlcpy(newConfig.ssid, wifiObj[F("s")], sizeof(newConfig.ssid));
            }
            // Check if PSK is provided and that it is not empty and is not equal *** (which is used in the UI to indicate that the password is set but not shown)
            if(wifiObj.containsKey(F("p")) && strlen(wifiObj[F("p")]) > 0 && strcmp(wifiObj[F("p")], "***") != 0) {
                strlcpy(newConfig.psk, wifiObj[F("p")], sizeof(newConfig.psk));
            }
            if(wifiObj.containsKey(F("w"))) {
                newConfig.power = wifiObj[F("w")];
            }
            if(wifiObj.containsKey(F("z"))) {
                newConfig.sleep = wifiObj[F("z")];
            }
            if(wifiObj.containsKey(F("b"))) {
                newConfig.use11b = wifiObj[F("b")];
            }
            config->setNetworkConfig(newConfig);
        }
    }

    // MQTT
    if(configObj.containsKey(F("q"))) {
        JsonObject mqttObj = configObj[F("q")];
        MqttConfig newConfig;
        config->getMqttConfig(newConfig);
        if(mqttObj.containsKey(F("p"))) {
            newConfig.port = mqttObj[F("p")];
        }
        if(mqttObj.containsKey(F("u"))) {
            strlcpy(newConfig.username, mqttObj[F("u")], sizeof(newConfig.username));
        }
        // Check if password is provided and that it is not empty and is not equal *** (which is used in the UI to indicate that the password is set but not shown)
        if(mqttObj.containsKey(F("a")) && strlen(mqttObj[F("a")]) > 0 && strcmp(mqttObj[F("a")], "***") != 0) {
            strlcpy(newConfig.password, mqttObj[F("a")], sizeof(newConfig.password));
        }
        if(mqttObj.containsKey(F("c"))) {
            strlcpy(newConfig.clientId, mqttObj[F("c")], sizeof(newConfig.clientId));
        }
        if(mqttObj.containsKey(F("b"))) {
            strlcpy(newConfig.publishTopic, mqttObj[F("b")], sizeof(newConfig.publishTopic));
        }
        if(mqttObj.containsKey(F("r"))) {
            strlcpy(newConfig.subscribeTopic, mqttObj[F("r")], sizeof(newConfig.subscribeTopic));
        }
        if(mqttObj.containsKey(F("m"))) {
            newConfig.payloadFormat = mqttObj[F("m")];
        }
        if(mqttObj.containsKey(F("t"))) {
            newConfig.stateUpdate = mqttObj[F("s")];
        }
        if(mqttObj.containsKey(F("d"))) {
            newConfig.stateUpdateInterval = mqttObj[F("d")];
        }
        if(mqttObj.containsKey(F("i"))) {
            newConfig.timeout = mqttObj[F("i")];
        }
        if(mqttObj.containsKey(F("k"))) {
            newConfig.keepalive = mqttObj[F("k")];
        }
        if(mqttObj.containsKey(F("e"))) {
            newConfig.rebootMinutes = mqttObj[F("e")];
        }
        config->setMqttConfig(newConfig);

        if(newConfig.payloadFormat == 3) { // Domiticz
            if(configObj.containsKey(F("o"))) {
                JsonObject domoticzObj = configObj[F("o")];
                DomoticzConfig domoticzConfig;
                config->getDomoticzConfig(domoticzConfig);
                if(domoticzObj.containsKey(F("e"))) {
                    domoticzConfig.elidx = domoticzObj[F("e")];
                }
                if(domoticzObj.containsKey(F("c"))) {
                    domoticzConfig.cl1idx = domoticzObj[F("c")];
                }
                if(domoticzObj.containsKey(F("u1"))) {
                    domoticzConfig.vl1idx = domoticzObj[F("u1")];
                }
                if(domoticzObj.containsKey(F("u2"))) {
                    domoticzConfig.vl2idx = domoticzObj[F("u2")];
                }
                if(domoticzObj.containsKey(F("u3"))) {
                    domoticzConfig.vl3idx = domoticzObj[F("u3")];
                }
                config->setDomoticzConfig(domoticzConfig);
            }
        } else if(newConfig.payloadFormat == 4) { // Home Assistant
            if(configObj.containsKey(F("h"))) {
                JsonObject haObj = configObj[F("h")];
                HomeAssistantConfig haConfig;
                config->getHomeAssistantConfig(haConfig);
                if(haObj.containsKey(F("t"))) {
                    strlcpy(haConfig.discoveryPrefix, haObj[F("t")], sizeof(haConfig.discoveryPrefix));
                }
                if(haObj.containsKey(F("h"))) {
                    strlcpy(haConfig.discoveryHostname, haObj[F("h")], sizeof(haConfig.discoveryHostname));
                }
                if(haObj.containsKey(F("n"))) {
                    strlcpy(haConfig.discoveryNameTag, haObj[F("n")], sizeof(haConfig.discoveryNameTag));
                }
                config->setHomeAssistantConfig(haConfig);
            }
        }
    }

    // Price service
    if(configObj.containsKey(F("p"))) {
        PriceServiceConfig newConfig;
        config->getPriceServiceConfig(newConfig);
        JsonObject priceServiceObj = configObj[F("p")];
        if(priceServiceObj.containsKey(F("e"))) {
            newConfig.enabled = priceServiceObj[F("e")];
        }
        if(priceServiceObj.containsKey(F("r"))) {
            strlcpy(newConfig.area, priceServiceObj[F("r")], sizeof(newConfig.area));
        }
        if(priceServiceObj.containsKey(F("c"))) {
            strlcpy(newConfig.currency, priceServiceObj[F("c")], sizeof(newConfig.currency));
        }
        if(priceServiceObj.containsKey(F("m"))) {
            newConfig.resolutionInMinutes = priceServiceObj[F("m")];
        }
        config->setPriceServiceConfig(newConfig);
    }

    // Thresholds
    if(configObj.containsKey(F("t"))) {
        EnergyAccountingConfig newConfig;
        config->getEnergyAccountingConfig(newConfig);
        JsonObject thresholdObj = configObj[F("t")];
        if(thresholdObj.containsKey(F("h"))) {
            newConfig.hours = thresholdObj[F("h")];
        }
        if(thresholdObj.containsKey(F("t"))) {
            JsonArray thresholdsArray = thresholdObj[F("t")].as<JsonArray>();
            for(size_t i = 0; i < thresholdsArray.size(); i++) {
                newConfig.thresholds[i] = thresholdsArray[i];
            }
        }
        config->setEnergyAccountingConfig(newConfig);
    }   

    // Debug
    if(configObj.containsKey(F("d"))) {
        DebugConfig newConfig;
        config->getDebugConfig(newConfig);

        JsonObject debugObj = configObj[F("d")];
        if(debugObj.containsKey(F("s"))) {
            newConfig.serial = debugObj[F("s")];
        }
        if(debugObj.containsKey(F("t"))) {
            newConfig.telnet = debugObj[F("t")];
        }
        if(debugObj.containsKey(F("l"))) {
            newConfig.level = debugObj[F("l")];
        }
        config->setDebugConfig(newConfig);
    }

    // Cloud
    if(configObj.containsKey(F("c"))) {
        CloudConfig newConfig;
        config->getCloudConfig(newConfig);

        JsonObject cloudObj = configObj[F("c")];
        if(cloudObj.containsKey(F("e"))) {
            newConfig.enabled = cloudObj[F("e")];
        }
        if(cloudObj.containsKey(F("p"))) {
            newConfig.proto = cloudObj[F("p")];
        }
        config->setCloudConfig(newConfig);

        if(cloudObj.containsKey(F("es"))) {
            SystemConfig sysConfig;
            config->getSystemConfig(sysConfig);
            sysConfig.energyspeedometer = cloudObj[F("es")];
            config->setSystemConfig(sysConfig);
        }
        if(cloudObj.containsKey(F("ze"))) {
            ZmartChargeConfig zmartConfig;
            config->getZmartChargeConfig(zmartConfig);
            zmartConfig.enabled = cloudObj[F("ze")];
            if(cloudObj.containsKey(F("zt"))) {
                strlcpy(zmartConfig.token, cloudObj[F("zt")], sizeof(zmartConfig.token));
            }
            if(cloudObj.containsKey(F("zu"))) {
                strlcpy(zmartConfig.baseUrl, cloudObj[F("zu")], sizeof(zmartConfig.baseUrl));
            }
            config->setZmartChargeConfig(zmartConfig);
        }
    }

    // UI
    if(configObj.containsKey(F("u"))) {
        UiConfig newConfig;
        config->getUiConfig(newConfig);
        JsonObject uiObj = configObj[F("u")];
        if(uiObj.containsKey(F("i"))) {
            newConfig.showImport = uiObj[F("i")];
        }
        if(uiObj.containsKey(F("e"))) {
            newConfig.showExport = uiObj[F("e")];
        }
        if(uiObj.containsKey(F("v"))) {
            newConfig.showVoltage = uiObj[F("v")];
        }
        if(uiObj.containsKey(F("a"))) {
            newConfig.showAmperage = uiObj[F("a")];
        }
        if(uiObj.containsKey(F("r"))) {
            newConfig.showReactive = uiObj[F("r")];
        }
        if(uiObj.containsKey(F("c"))) {
            newConfig.showRealtime = uiObj[F("c")];
        }
        if(uiObj.containsKey(F("t"))) {
            newConfig.showPeaks = uiObj[F("t")];
        }
        if(uiObj.containsKey(F("p"))) {
            newConfig.showPricePlot = uiObj[F("p")];
        }
        if(uiObj.containsKey(F("d"))) {
            newConfig.showDayPlot = uiObj[F("d")];
        }
        if(uiObj.containsKey(F("m"))) {
            newConfig.showMonthPlot = uiObj[F("m")];
        }
        if(uiObj.containsKey(F("s"))) {
            newConfig.showTemperaturePlot = uiObj[F("s")];
        }
        if(uiObj.containsKey(F("l"))) {
            newConfig.showRealtimePlot = uiObj[F("l")];
        }
        if(uiObj.containsKey(F("h"))) {
            newConfig.showPerPhasePower = uiObj[F("h")];
        }
        if(uiObj.containsKey(F("f"))) {
            newConfig.showPowerFactor = uiObj[F("f")];
        }
        if(uiObj.containsKey(F("k"))) {
            newConfig.darkMode = uiObj[F("k")];
        }
        if(uiObj.containsKey(F("lang"))) {
            strlcpy(newConfig.language, uiObj[F("lang")], sizeof(newConfig.language));
        }
        config->setUiConfig(newConfig);
    }

    // System
    if(configObj.containsKey(F("s"))) {
        SystemConfig sysConfig;
        config->getSystemConfig(sysConfig);
        JsonObject sysObj = configObj[F("s")];
        if(sysObj.containsKey(F("b"))) {
            sysConfig.boardType = sysObj[F("b")];
        }
        if(sysObj.containsKey(F("v"))) {
            sysConfig.vendorConfigured = sysObj[F("v")];
        }
        if(sysObj.containsKey(F("u"))) {
            sysConfig.userConfigured = sysObj[F("u")];
        }
        if(sysObj.containsKey(F("d"))) {
            sysConfig.dataCollectionConsent = sysObj[F("d")];
        }
        if(sysObj.containsKey(F("o"))) {
            strlcpy(sysConfig.country, sysObj[F("o")], sizeof(sysConfig.country));
        }
        if(sysObj.containsKey(F("c"))) {
            sysConfig.firmwareChannel = sysObj[F("c")];
        }
        config->setSystemConfig(sysConfig);
    }

    // GPIO
    if(configObj.containsKey(F("i"))) {
        JsonObject gpioObj = configObj[F("i")];
        GpioConfig newConfig;
        config->getGpioConfig(newConfig);
        if(gpioObj.containsKey(F("a"))) {
            newConfig.apPin = gpioObj[F("a")];
        }
        if(gpioObj.containsKey(F("l"))) {
            JsonObject ledObj = gpioObj[F("l")];
            if(ledObj.containsKey(F("p"))) {
                newConfig.ledPin = ledObj[F("p")];
            }
            if(ledObj.containsKey(F("i"))) {
                newConfig.ledInverted = ledObj[F("i")];
            }
        }
        if(gpioObj.containsKey(F("r"))) {
            JsonObject rgbLedObj = gpioObj[F("r")];
            if(rgbLedObj.containsKey(F("r"))) {
                newConfig.ledPinRed = rgbLedObj[F("r")];
            }
            if(rgbLedObj.containsKey(F("g"))) {
                newConfig.ledPinGreen = rgbLedObj[F("g")];
            }
            if(rgbLedObj.containsKey(F("b"))) {
                newConfig.ledPinBlue = rgbLedObj[F("b")];
            }
            if(rgbLedObj.containsKey(F("i"))) {
                newConfig.ledRgbInverted = rgbLedObj[F("i")];
            }
        }
        if(gpioObj.containsKey(F("d"))) {
            JsonObject ledDisableObj = gpioObj[F("d")];
            if(ledDisableObj.containsKey(F("d"))) {
                newConfig.ledDisablePin = ledDisableObj[F("d")];
            }
            if(ledDisableObj.containsKey(F("b"))) {
                newConfig.ledBehaviour = ledDisableObj[F("b")];
            }
        }
        if(gpioObj.containsKey(F("t"))) {
            JsonObject tempSensorObj = gpioObj[F("t")];
            if(tempSensorObj.containsKey(F("d"))) {
                newConfig.tempSensorPin = tempSensorObj[F("d")];
            }
            if(tempSensorObj.containsKey(F("a"))) {
                newConfig.tempAnalogSensorPin = tempSensorObj[F("a")];
            }
        }
        if(gpioObj.containsKey(F("v"))) {
            JsonObject vccObj = gpioObj[F("v")];
            if(vccObj.containsKey(F("p"))) {
                newConfig.vccPin = vccObj[F("p")];
            }
            if(vccObj.containsKey(F("o"))) {
                newConfig.vccOffset = vccObj[F("o")];
            }
            if(vccObj.containsKey(F("m"))) {
                newConfig.vccMultiplier = vccObj[F("m")];
            }
            if(vccObj.containsKey(F("d"))) {
                JsonObject vccDividerObj = vccObj[F("d")];
                if(vccDividerObj.containsKey(F("v"))) {
                    newConfig.vccResistorVcc = vccDividerObj[F("v")];
                }
                if(vccDividerObj.containsKey(F("g"))) {
                    newConfig.vccResistorGnd = vccDividerObj[F("g")];
                }
            }
        }
        config->setGpioConfig(newConfig);

        if(gpioObj.containsKey(F("h"))) {
            JsonObject hwObj = gpioObj[F("h")];
            MeterConfig meterConfig;
            config->getMeterConfig(meterConfig);
            if(hwObj.containsKey(F("p"))) {
                meterConfig.rxPin = hwObj[F("p")];
            }
            if(hwObj.containsKey(F("u"))) {
                meterConfig.rxPinPullup = hwObj[F("u")];
            }
            if(hwObj.containsKey(F("t"))) {
                meterConfig.txPin = hwObj[F("t")];
            }
            config->setMeterConfig(meterConfig);
        }
    }
}
#endif

void JsonMqttHandler::toJsonIsoTimestamp(time_t t, char* buf, size_t buflen) {
    memset(buf, 0, buflen);
    if(t > 0) {
        tmElements_t tm;
        breakTime(t, tm);
        snprintf_P(buf, buflen, PSTR("\"%04d-%02d-%02dT%02d:%02d:%02dZ\""), tm.Year+1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
    } else {
        snprintf_P(buf, buflen, PSTR("null"));
    }
}
