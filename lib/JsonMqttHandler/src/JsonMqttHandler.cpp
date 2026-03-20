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

    String peaks = "";
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 1; i <= peakCount; i++) {
        if(!peaks.isEmpty()) peaks += ",";
        peaks += String(ea->getPeak(i).value / 100.0, 2);
    }
    
    return snprintf_P(json+pos, BufferSize-pos, PSTR("%s\"%sh\":%.3f,\"%sd\":%.2f,\"%sm\":%.1f,\"%st\":%d,\"%sx\":%.2f,\"%she\":%.3f,\"%sde\":%.2f,\"%sme\":%.1f,\"peaks\":[%s]%s"),
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
        uint16_t pos = snprintf_P(json, BufferSize, PSTR("{\"id\":\"%s\","), WiFi.macAddress().c_str());

        for(uint8_t i = 0;i < 38; i++) {
            if(values[i] == PRICE_NO_VALUE) {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"pr_%d\":null,"), i);
            } else {
                pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"pr_%d\":%.4f,"), i, values[i]);
            }
        }

        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("\"pr_min\":%.4f,\"pr_max\":%.4f,\"pr_cheapest1hr\":%s,\"pr_cheapest3hr\":%s,\"pr_cheapest6hr\":%s}"),
            min == INT16_MAX ? 0.0 : min,
            max == INT16_MIN ? 0.0 : max,
            ts1hr,
            ts3hr,
            ts6hr
        );
    } else {
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
        if(hasExport && ps->isExportPricesDifferentFromImport()) {
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
        pos += snprintf_P(json+pos, BufferSize-pos, PSTR("],\"min\":%.4f,\"max\":%.4f,\"cheapest1hr\":%s,\"cheapest3hr\":%s,\"cheapest6hr\":%s}}"),
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

bool JsonMqttHandler::publishRaw(uint8_t* raw, size_t length) {
	if(strlen(mqttConfig.publishTopic) == 0 || !mqtt.connected())
		return false;
    
    if(length <= 0 || length > BufferSize) return false;

    String str = toHex(raw, length);

    snprintf_P(json, BufferSize, PSTR("{\"data\":\"%s\"}"), str.c_str());
    char topic[192];
    snprintf_P(topic, 192, PSTR("%s/data"), mqttConfig.publishTopic);
    bool ret = mqtt.publish(topic, json);
    loop();
    return ret;
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
    if(strlen(mqttConfig.publishTopic) == 0 || !connected())
		return;

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Received command [%s] to [%s]\n"), payload.c_str(), topic.c_str());

    if(topic.equals(subTopic)) {
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
                        AmsJsonGenerator::generateDayPlotJson(ds, json, BufferSize);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp_P(action, PSTR("monthplot")) == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateMonthPlotJson(ds, json, BufferSize);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp_P(action, PSTR("config")) == 0 && obj.containsKey(F("config"))) {
                        JsonObject configObj = obj[F("config")];
                        handleConfigMessage(configObj);
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
            AmsJsonGenerator::generateDayPlotJson(ds, json, BufferSize);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
        } else if(payload.equals(F("monthplot"))) {
            char pubTopic[192];
            snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
            AmsJsonGenerator::generateMonthPlotJson(ds, json, BufferSize);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
        }
    }
}

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
            if(generalObj.containsKey(F("p"))) {
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
            // TODO, string to enum
            newConfig.parity = meterObj[F("p")];
        }
        if(meterObj.containsKey(F("i"))) {
            newConfig.invert = meterObj[F("i")];
        }
        if(meterObj.containsKey(F("s"))) {
            newConfig.bufferSize = meterObj[F("s")] / 64; // convert from bytes to 64 byte blocks
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
                        // TODO
                    }
                    if(encryptionObj.containsKey(F("a"))) {
                        // TODO
                    }
                } else {
                    memset(newConfig.encryptionKey, 0, sizeof(newConfig.encryptionKey));
                    memset(newConfig.authenticationKey, 0, sizeof(newConfig.authenticationKey));
                }
            }
        }

        config->setMeterConfig(newConfig);
    }

    if(configObj.containsKey(F("system"))) {
        SystemConfig newConfig;
        config->getSystemConfig(newConfig);

        JsonObject systemObj = configObj[F("system")];
        if(systemObj.containsKey(F("country"))) {
            strlcpy(newConfig.country, systemObj[F("country")], sizeof(newConfig.country));
        }
        if(systemObj.containsKey(F("firmwareChannel"))) {
            newConfig.firmwareChannel = systemObj[F("firmwareChannel")];
        }
        config->setSystemConfig(newConfig);
    }

    if(configObj.containsKey(F("network"))) {
        NetworkConfig newConfig;
        config->getNetworkConfig(newConfig);

        JsonObject networkObj = configObj[F("network")];
        if(networkObj.containsKey(F("mode"))) {
            newConfig.mode = networkObj[F("mode")];
        }
        if(newConfig.mode == 1 || newConfig.mode == 2) {
            if(networkObj.containsKey(F("ssid"))) {
                strlcpy(newConfig.ssid, networkObj[F("ssid")], sizeof(newConfig.ssid));
            }
            if(networkObj.containsKey(F("psk"))) {
                strlcpy(newConfig.psk, networkObj[F("psk")], sizeof(newConfig.psk));
            }
            if(networkObj.containsKey(F("power"))) {
                newConfig.power = networkObj[F("power")];
            }
            if(networkObj.containsKey(F("sleep"))) {
                newConfig.sleep = networkObj[F("sleep")];
            }
            if(networkObj.containsKey(F("use11b"))) {
                newConfig.use11b = networkObj[F("use11b")];
            }
        }
        if(networkObj.containsKey(F("ip"))) {
            strlcpy(newConfig.ip, networkObj[F("ip")], sizeof(newConfig.ip));
        }
        if(networkObj.containsKey(F("gateway"))) {
            strlcpy(newConfig.gateway, networkObj[F("gateway")], sizeof(newConfig.gateway));
        }
        if(networkObj.containsKey(F("subnet"))) {
            strlcpy(newConfig.subnet, networkObj[F("subnet")], sizeof(newConfig.subnet));
        }
        if(networkObj.containsKey(F("dns1"))) {
            strlcpy(newConfig.dns1, networkObj[F("dns1")], sizeof(newConfig.dns1));
        }
        if(networkObj.containsKey(F("dns2"))) {
            strlcpy(newConfig.dns2, networkObj[F("dns2")], sizeof(newConfig.dns2));
        }
        if(networkObj.containsKey(F("mdns"))) {
            newConfig.mdns = networkObj[F("mdns")];
        }
        if(networkObj.containsKey(F("ipv6"))) {
            newConfig.ipv6 = networkObj[F("ipv6")];
        }
        config->setNetworkConfig(newConfig);
    }

    if(configObj.containsKey(F("meter"))) {
        MeterConfig newConfig;
        config->getMeterConfig(newConfig);

        JsonObject meterObj = configObj[F("meter")];
        if(meterObj.containsKey(F("baud"))) {
            newConfig.baud = meterObj[F("baud")];
        }
        if(meterObj.containsKey(F("parity"))) {
            newConfig.parity = meterObj[F("parity")];
        }
        if(meterObj.containsKey(F("invert"))) {
            newConfig.invert = meterObj[F("invert")];
        }
        if(meterObj.containsKey(F("distributionSystem"))) {
            newConfig.distributionSystem = meterObj[F("distributionSystem")];
        }
        if(meterObj.containsKey(F("mainFuse"))) {
            newConfig.mainFuse = meterObj[F("mainFuse")];
        }
        if(meterObj.containsKey(F("productionCapacity"))) {
            newConfig.productionCapacity = meterObj[F("productionCapacity")];
        }
        if(meterObj.containsKey(F("wattageMultiplier"))) {
            newConfig.wattageMultiplier = meterObj[F("wattageMultiplier")];
        }
        if(meterObj.containsKey(F("voltageMultiplier"))) {
            newConfig.voltageMultiplier = meterObj[F("voltageMultiplier")];
        }
        if(meterObj.containsKey(F("amperageMultiplier"))) {
            newConfig.amperageMultiplier = meterObj[F("amperageMultiplier")];
        }
        if(meterObj.containsKey(F("accumulatedMultiplier"))) {
            newConfig.accumulatedMultiplier = meterObj[F("accumulatedMultiplier")];
        }
        if(meterObj.containsKey(F("parser"))) {
            newConfig.parser = meterObj[F("parser")];
        }
        if(meterObj.containsKey(F("bufferSize"))) {
            newConfig.bufferSize = meterObj[F("bufferSize")];
        }
        if(meterObj.containsKey(F("rxPin"))) {
            newConfig.rxPin = meterObj[F("rxPin")];
        }
        if(meterObj.containsKey(F("rxPinPullup"))) {
            newConfig.rxPinPullup = meterObj[F("rxPinPullup")];
        }
        if(meterObj.containsKey(F("txPin"))) {
            newConfig.txPin = meterObj[F("txPin")];
        }
    }

    if(configObj.containsKey(F("mqtt"))) {
        MqttConfig newConfig;
        config->getMqttConfig(newConfig);

        JsonObject mqttObj = configObj[F("mqtt")];
        if(mqttObj.containsKey(F("host"))) {
            strlcpy(newConfig.host, mqttObj[F("host")], sizeof(newConfig.host));
        }
        if(mqttObj.containsKey(F("port"))) {
            newConfig.port = mqttObj[F("port")];
        }
        if(mqttObj.containsKey(F("clientId"))) {
            strlcpy(newConfig.clientId, mqttObj[F("clientId")], sizeof(newConfig.clientId));
        }
        if(mqttObj.containsKey(F("publishTopic"))) {
            strlcpy(newConfig.publishTopic, mqttObj[F("publishTopic")], sizeof(newConfig.publishTopic));
        }
        if(mqttObj.containsKey(F("subscribeTopic"))) {
            strlcpy(newConfig.subscribeTopic, mqttObj[F("subscribeTopic")], sizeof(newConfig.subscribeTopic));
        }
        if(mqttObj.containsKey(F("username"))) {
            strlcpy(newConfig.username, mqttObj[F("username")], sizeof(newConfig.username));
        }
        if(mqttObj.containsKey(F("password"))) {
            strlcpy(newConfig.password, mqttObj[F("password")], sizeof(newConfig.password));
        }
        if(mqttObj.containsKey(F("payloadFormat"))) {
            newConfig.payloadFormat = mqttObj[F("payloadFormat")];
        }
        if(mqttObj.containsKey(F("ssl"))) {
            newConfig.ssl = mqttObj[F("ssl")];
        }
        if(mqttObj.containsKey(F("stateUpdate"))) {
            newConfig.stateUpdate = mqttObj[F("stateUpdate")];
        }
        if(mqttObj.containsKey(F("stateUpdateInterval"))) {
            newConfig.stateUpdateInterval = mqttObj[F("stateUpdateInterval")];
        }
        if(mqttObj.containsKey(F("timeout"))) {
            newConfig.timeout = mqttObj[F("timeout")];
        }
        if(mqttObj.containsKey(F("keepalive"))) {
            newConfig.keepalive = mqttObj[F("keepalive")];
        }
        if(mqttObj.containsKey(F("rebootMinutes"))) {
            newConfig.rebootMinutes = mqttObj[F("rebootMinutes")];
        }
        config->setMqttConfig(newConfig);

        if(mqttObj.containsKey(F("domoticz"))) {
            DomoticzConfig newConfig;
            config->getDomoticzConfig(newConfig);
            JsonObject domoticzObj = mqttObj[F("domoticz")];
            if(domoticzObj.containsKey(F("elidx"))) {
                newConfig.elidx = domoticzObj[F("elidx")];
            }
            if(domoticzObj.containsKey(F("vl1idx"))) {
                newConfig.vl1idx = domoticzObj[F("vl1idx")];
            }
            if(domoticzObj.containsKey(F("vl2idx"))) {
                newConfig.vl2idx = domoticzObj[F("vl2idx")];
            }
            if(domoticzObj.containsKey(F("vl3idx"))) {
                newConfig.vl3idx = domoticzObj[F("vl3idx")];
            }
            if(domoticzObj.containsKey(F("cl1idx"))) {
                newConfig.cl1idx = domoticzObj[F("cl1idx")];
            }
            config->setDomoticzConfig(newConfig);
        }

        if(mqttObj.containsKey(F("homeAssistant"))) {
            HomeAssistantConfig newConfig;
            config->getHomeAssistantConfig(newConfig);
            JsonObject haObj = mqttObj[F("homeAssistant")];
            if(haObj.containsKey(F("discoveryPrefix"))) {
                strlcpy(newConfig.discoveryPrefix, haObj[F("discoveryPrefix")], sizeof(newConfig.discoveryPrefix));
            }
            if(haObj.containsKey(F("discoveryHostname"))) {
                strlcpy(newConfig.discoveryHostname, haObj[F("discoveryHostname")], sizeof(newConfig.discoveryHostname));
            }
            if(haObj.containsKey(F("discoveryNameTag"))) {
                strlcpy(newConfig.discoveryNameTag, haObj[F("discoveryNameTag")], sizeof(newConfig.discoveryNameTag));
            }
            config->setHomeAssistantConfig(newConfig);
        }
    }

    if(configObj.containsKey(F("debug"))) {
        DebugConfig newConfig;
        config->getDebugConfig(newConfig);

        JsonObject debugObj = configObj[F("debug")];
        if(debugObj.containsKey(F("telnet"))) {
            newConfig.telnet = debugObj[F("telnet")];
        }
        if(debugObj.containsKey(F("serial"))) {
            newConfig.serial = debugObj[F("serial")];
        }
        if(debugObj.containsKey(F("level"))) {
            newConfig.level = debugObj[F("level")];
        }
        config->setDebugConfig(newConfig);
    }

    if(configObj.containsKey(F("gpio"))) {
        GpioConfig newConfig;
        config->getGpioConfig(newConfig);

        JsonObject gpioObj = configObj[F("gpio")];
        if(gpioObj.containsKey(F("apPin"))) {
            newConfig.apPin = gpioObj[F("apPin")];
        }
        if(gpioObj.containsKey(F("ledPin"))) {
            newConfig.ledPin = gpioObj[F("ledPin")];
        }
        if(gpioObj.containsKey(F("ledInverted"))) {
            newConfig.ledInverted = gpioObj[F("ledInverted")];
        }
        if(gpioObj.containsKey(F("ledPinRed"))) {
            newConfig.ledPinRed = gpioObj[F("ledPinRed")];
        }
        if(gpioObj.containsKey(F("ledPinGreen"))) {
            newConfig.ledPinGreen = gpioObj[F("ledPinGreen")];
        }
        if(gpioObj.containsKey(F("ledPinBlue"))) {
            newConfig.ledPinBlue = gpioObj[F("ledPinBlue")];
        }
        if(gpioObj.containsKey(F("ledRgbInverted"))) {
            newConfig.ledRgbInverted = gpioObj[F("ledRgbInverted")];
        }
        if(gpioObj.containsKey(F("tempSensorPin"))) {
            newConfig.tempSensorPin = gpioObj[F("tempSensorPin")];
        }
        if(gpioObj.containsKey(F("tempAnalogSensorPin"))) {
            newConfig.tempAnalogSensorPin = gpioObj[F("tempAnalogSensorPin")];
        }
        if(gpioObj.containsKey(F("vccPin"))) {
            newConfig.vccPin = gpioObj[F("vccPin")];
        }
        if(gpioObj.containsKey(F("vccOffset"))) {
            newConfig.vccOffset = gpioObj[F("vccOffset")];
        }
        if(gpioObj.containsKey(F("vccMultiplier"))) {
            newConfig.vccMultiplier = gpioObj[F("vccMultiplier")];
        }
        if(gpioObj.containsKey(F("vccBootLimit"))) {
            newConfig.vccBootLimit = gpioObj[F("vccBootLimit")];
        }
        if(gpioObj.containsKey(F("vccResistorGnd"))) {
            newConfig.vccResistorGnd = gpioObj[F("vccResistorGnd")];
        }
        if(gpioObj.containsKey(F("vccResistorVcc"))) {
            newConfig.vccResistorVcc = gpioObj[F("vccResistorVcc")];
        }
        if(gpioObj.containsKey(F("ledDisablePin"))) {
            newConfig.ledDisablePin = gpioObj[F("ledDisablePin")];
        }
        if(gpioObj.containsKey(F("ledBehaviour"))) {
            newConfig.ledBehaviour = gpioObj[F("ledBehaviour")];
        }
        config->setGpioConfig(newConfig);
    }

    if(configObj.containsKey(F("ntp"))) {
        NtpConfig newConfig;
        config->getNtpConfig(newConfig);

        JsonObject ntpObj = configObj[F("ntp")];
        if(ntpObj.containsKey(F("enable"))) {
            newConfig.enable = ntpObj[F("enable")];
        }
        if(ntpObj.containsKey(F("dhcp"))) {
            newConfig.dhcp = ntpObj[F("dhcp")];
        }
        if(ntpObj.containsKey(F("server"))) {
            strlcpy(newConfig.server, ntpObj[F("server")], sizeof(newConfig.server));
        }
        config->setNtpConfig(newConfig);
    }

    if(configObj.containsKey(F("priceService"))) {
        PriceServiceConfig newConfig;
        config->getPriceServiceConfig(newConfig);
        JsonObject priceServiceObj = configObj[F("priceService")];
        if(priceServiceObj.containsKey(F("area"))) {
            strlcpy(newConfig.area, priceServiceObj[F("area")], sizeof(newConfig.area));
        }
        if(priceServiceObj.containsKey(F("currency"))) {
            strlcpy(newConfig.currency, priceServiceObj[F("currency")], sizeof(newConfig.currency));
        }
        if(priceServiceObj.containsKey(F("resolutionInMinutes"))) {
            newConfig.resolutionInMinutes = priceServiceObj[F("resolutionInMinutes")];
        }
        if(priceServiceObj.containsKey(F("enabled"))) {
            newConfig.enabled = priceServiceObj[F("enabled")];
        }
        config->setPriceServiceConfig(newConfig);
    }

    if(configObj.containsKey(F("cloud"))) {
        JsonObject cloudObj = configObj[F("cloud")];

        if(cloudObj.containsKey(F("amsleser"))) {
            CloudConfig newConfig;
            config->getCloudConfig(newConfig);

            JsonObject amsCloudObj = cloudObj[F("amsleser")];
            if(amsCloudObj.containsKey(F("enabled"))) {
                newConfig.enabled = amsCloudObj[F("enabled")];
            }
            if(amsCloudObj.containsKey(F("interval"))) {
                newConfig.interval = amsCloudObj[F("interval")];
            }
            if(amsCloudObj.containsKey(F("hostname"))) {
                strlcpy(newConfig.hostname, amsCloudObj[F("hostname")], sizeof(newConfig.hostname));
            }
            if(amsCloudObj.containsKey(F("port"))) {
                newConfig.port = amsCloudObj[F("port")];
            }
            if(amsCloudObj.containsKey(F("clientId"))) {
                strlcpy((char*)newConfig.clientId, amsCloudObj[F("clientId")], sizeof(newConfig.clientId));
            }
            if(amsCloudObj.containsKey(F("proto"))) {
                newConfig.proto = amsCloudObj[F("proto")];
            }
            config->setCloudConfig(newConfig);
        }

        if(cloudObj.containsKey(F("zmartcharge"))) {
            ZmartChargeConfig newConfig;
            config->getZmartChargeConfig(newConfig);
            JsonObject zmartChargeObj = cloudObj[F("zmartcharge")];
            if(zmartChargeObj.containsKey(F("enabled"))) {
                newConfig.enabled = zmartChargeObj[F("enabled")];
            }
            if(zmartChargeObj.containsKey(F("token"))) {
                strlcpy(newConfig.token, zmartChargeObj[F("token")], sizeof(newConfig.token));
            }
            if(zmartChargeObj.containsKey(F("baseUrl"))) {
                strlcpy(newConfig.baseUrl, zmartChargeObj[F("baseUrl")], sizeof(newConfig.baseUrl));
            }
            config->setZmartChargeConfig(newConfig);
        }

        if(cloudObj.containsKey(F("energyspeedometer"))) {
            SystemConfig newConfig;
            config->getSystemConfig(newConfig);
            JsonObject speedometerObj = cloudObj[F("energyspeedometer")];
            if(speedometerObj.containsKey(F("enabled"))) {
                newConfig.energyspeedometer = speedometerObj[F("enabled")];
            }
            config->setSystemConfig(newConfig);
        }
    }
}

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
