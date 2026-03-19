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
#include "ArduinoJson.h"

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
            DynamicJsonDocument doc(BufferSize);
            DeserializationError error = deserializeJson(doc, payload);
            if(error) {
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::ERROR))
                #endif
                debugger->printf_P(PSTR(" - failed to parse JSON: %s\n"), error.c_str());
                return;
            } else {
                JsonObject obj = doc.as<JsonObject>();
                if(obj.containsKey("action")) {
                    const char* action = obj["action"];
                    if(strcmp(action, "fwupgrade") == 0) {
                        if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                            updater->setTargetVersion(updater->getNextVersion());
                        }
                    } else if(strcmp(action, "dayplot") == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/dayplot"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateDayPlotJson(ds, json, BufferSize);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp(action, "monthplot") == 0) {
                        char pubTopic[192];
                        snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
                        AmsJsonGenerator::generateMonthPlotJson(ds, json, BufferSize);
                        bool ret = mqtt.publish(pubTopic, json);
                        loop();
                    } else if(strcmp(action, "config") == 0 && obj.containsKey("config")) {
                        JsonObject configObj = obj["config"];

                        if(configObj.containsKey("system")) {
                            SystemConfig newConfig;
                            config->getSystemConfig(newConfig);

                            JsonObject systemObj = configObj["system"];
                            if(systemObj.containsKey("country")) {
                                strlcpy(newConfig.country, systemObj["country"], sizeof(newConfig.country));
                            }
                            if(systemObj.containsKey("firmwareChannel")) {
                                newConfig.firmwareChannel = systemObj["firmwareChannel"];
                            }
                            config->setSystemConfig(newConfig);
                        }

                        if(configObj.containsKey("network")) {
                            NetworkConfig newConfig;
                            config->getNetworkConfig(newConfig);

                            JsonObject networkObj = configObj["network"];
                            if(networkObj.containsKey("mode")) {
                                newConfig.mode = networkObj["mode"];
                            }
                            if(newConfig.mode == 1 || newConfig.mode == 2) {
                                if(networkObj.containsKey("ssid")) {
                                    strlcpy(newConfig.ssid, networkObj["ssid"], sizeof(newConfig.ssid));
                                }
                                if(networkObj.containsKey("psk")) {
                                    strlcpy(newConfig.psk, networkObj["psk"], sizeof(newConfig.psk));
                                }
                                if(networkObj.containsKey("power")) {
                                    newConfig.power = networkObj["power"];
                                }
                                if(networkObj.containsKey("sleep")) {
                                    newConfig.sleep = networkObj["sleep"];
                                }
                                if(networkObj.containsKey("use11b")) {
                                    newConfig.use11b = networkObj["use11b"];
                                }
                            }
                            if(networkObj.containsKey("ip")) {
                                strlcpy(newConfig.ip, networkObj["ip"], sizeof(newConfig.ip));
                            }
                            if(networkObj.containsKey("gateway")) {
                                strlcpy(newConfig.gateway, networkObj["gateway"], sizeof(newConfig.gateway));
                            }
                            if(networkObj.containsKey("subnet")) {
                                strlcpy(newConfig.subnet, networkObj["subnet"], sizeof(newConfig.subnet));
                            }
                            if(networkObj.containsKey("dns1")) {
                                strlcpy(newConfig.dns1, networkObj["dns1"], sizeof(newConfig.dns1));
                            }
                            if(networkObj.containsKey("dns2")) {
                                strlcpy(newConfig.dns2, networkObj["dns2"], sizeof(newConfig.dns2));
                            }
                            if(networkObj.containsKey("hostname")) {
                                strlcpy(newConfig.hostname, networkObj["hostname"], sizeof(newConfig.hostname));
                            }
                            if(networkObj.containsKey("mdns")) {
                                newConfig.mdns = networkObj["mdns"];
                            }
                            if(networkObj.containsKey("ipv6")) {
                                newConfig.ipv6 = networkObj["ipv6"];
                            }
                            config->setNetworkConfig(newConfig);
                        }

                        if(configObj.containsKey("web")) {
                            WebConfig newConfig;
                            config->getWebConfig(newConfig);

                            JsonObject webObj = configObj["web"];
                            if(webObj.containsKey("security")) {
                                newConfig.security = webObj["security"];
                            }
                            if(newConfig.security > 0) {
                                if(webObj.containsKey("username")) {
                                    strlcpy(newConfig.username, webObj["username"], sizeof(newConfig.username));
                                }
                                if(webObj.containsKey("password")) {
                                    strlcpy(newConfig.password, webObj["password"], sizeof(newConfig.password));
                                }
                            }
                            if(webObj.containsKey("context")) {
                                strlcpy(newConfig.context, webObj["context"], sizeof(newConfig.context));
                            }
                            config->setWebConfig(newConfig);
                        }

                        if(configObj.containsKey("meter")) {
                            MeterConfig newConfig;
                            config->getMeterConfig(newConfig);

                            JsonObject meterObj = configObj["meter"];
                            if(meterObj.containsKey("baud")) {
                                newConfig.baud = meterObj["baud"];
                            }
                            if(meterObj.containsKey("parity")) {
                                newConfig.parity = meterObj["parity"];
                            }
                            if(meterObj.containsKey("invert")) {
                                newConfig.invert = meterObj["invert"];
                            }
                            if(meterObj.containsKey("distributionSystem")) {
                                newConfig.distributionSystem = meterObj["distributionSystem"];
                            }
                            if(meterObj.containsKey("mainFuse")) {
                                newConfig.mainFuse = meterObj["mainFuse"];
                            }
                            if(meterObj.containsKey("productionCapacity")) {
                                newConfig.productionCapacity = meterObj["productionCapacity"];
                            }
                            if(meterObj.containsKey("wattageMultiplier")) {
                                newConfig.wattageMultiplier = meterObj["wattageMultiplier"];
                            }
                            if(meterObj.containsKey("voltageMultiplier")) {
                                newConfig.voltageMultiplier = meterObj["voltageMultiplier"];
                            }
                            if(meterObj.containsKey("amperageMultiplier")) {
                                newConfig.amperageMultiplier = meterObj["amperageMultiplier"];
                            }
                            if(meterObj.containsKey("accumulatedMultiplier")) {
                                newConfig.accumulatedMultiplier = meterObj["accumulatedMultiplier"];
                            }
                            if(meterObj.containsKey("parser")) {
                                newConfig.parser = meterObj["parser"];
                            }
                            if(meterObj.containsKey("bufferSize")) {
                                newConfig.bufferSize = meterObj["bufferSize"];
                            }
                            if(meterObj.containsKey("rxPin")) {
                                newConfig.rxPin = meterObj["rxPin"];
                            }
                            if(meterObj.containsKey("rxPinPullup")) {
                                newConfig.rxPinPullup = meterObj["rxPinPullup"];
                            }
                            if(meterObj.containsKey("txPin")) {
                                newConfig.txPin = meterObj["txPin"];
                            }
                        }

                        if(configObj.containsKey("mqtt")) {
                            MqttConfig newConfig;
                            config->getMqttConfig(newConfig);

                            JsonObject mqttObj = configObj["mqtt"];
                            if(mqttObj.containsKey("host")) {
                                strlcpy(newConfig.host, mqttObj["host"], sizeof(newConfig.host));
                            }
                            if(mqttObj.containsKey("port")) {
                                newConfig.port = mqttObj["port"];
                            }
                            if(mqttObj.containsKey("clientId")) {
                                strlcpy(newConfig.clientId, mqttObj["clientId"], sizeof(newConfig.clientId));
                            }
                            if(mqttObj.containsKey("publishTopic")) {
                                strlcpy(newConfig.publishTopic, mqttObj["publishTopic"], sizeof(newConfig.publishTopic));
                            }
                            if(mqttObj.containsKey("subscribeTopic")) {
                                strlcpy(newConfig.subscribeTopic, mqttObj["subscribeTopic"], sizeof(newConfig.subscribeTopic));
                            }
                            if(mqttObj.containsKey("username")) {
                                strlcpy(newConfig.username, mqttObj["username"], sizeof(newConfig.username));
                            }
                            if(mqttObj.containsKey("password")) {
                                strlcpy(newConfig.password, mqttObj["password"], sizeof(newConfig.password));
                            }
                            if(mqttObj.containsKey("payloadFormat")) {
                                newConfig.payloadFormat = mqttObj["payloadFormat"];
                            }
                            if(mqttObj.containsKey("ssl")) {
                                newConfig.ssl = mqttObj["ssl"];
                            }
                            if(mqttObj.containsKey("stateUpdate")) {
                                newConfig.stateUpdate = mqttObj["stateUpdate"];
                            }
                            if(mqttObj.containsKey("stateUpdateInterval")) {
                                newConfig.stateUpdateInterval = mqttObj["stateUpdateInterval"];
                            }
                            if(mqttObj.containsKey("timeout")) {
                                newConfig.timeout = mqttObj["timeout"];
                            }
                            if(mqttObj.containsKey("keepalive")) {
                                newConfig.keepalive = mqttObj["keepalive"];
                            }
                            if(mqttObj.containsKey("rebootMinutes")) {
                                newConfig.rebootMinutes = mqttObj["rebootMinutes"];
                            }
                            config->setMqttConfig(newConfig);

                            if(mqttObj.containsKey("domoticz")) {
                                DomoticzConfig newConfig;
                                config->getDomoticzConfig(newConfig);
                                JsonObject domoticzObj = mqttObj["domoticz"];
                                if(domoticzObj.containsKey("elidx")) {
                                    newConfig.elidx = domoticzObj["elidx"];
                                }
                                if(domoticzObj.containsKey("vl1idx")) { 
                                    newConfig.vl1idx = domoticzObj["vl1idx"];
                                }
                                if(domoticzObj.containsKey("vl2idx")) {
                                    newConfig.vl2idx = domoticzObj["vl2idx"];
                                }
                                if(domoticzObj.containsKey("vl3idx")) {
                                    newConfig.vl3idx = domoticzObj["vl3idx"];
                                }
                                if(domoticzObj.containsKey("cl1idx")) {
                                    newConfig.cl1idx = domoticzObj["cl1idx"];
                                }
                                config->setDomoticzConfig(newConfig);
                            }

                            if(mqttObj.containsKey("homeAssistant")) {
                                HomeAssistantConfig newConfig;
                                config->getHomeAssistantConfig(newConfig);
                                JsonObject haObj = mqttObj["homeAssistant"];
                                if(haObj.containsKey("discoveryPrefix")) {
                                    strlcpy(newConfig.discoveryPrefix, haObj["discoveryPrefix"], sizeof(newConfig.discoveryPrefix));
                                }
                                if(haObj.containsKey("discoveryHostname")) {
                                    strlcpy(newConfig.discoveryHostname, haObj["discoveryHostname"], sizeof(newConfig.discoveryHostname));
                                }
                                if(haObj.containsKey("discoveryNameTag")) {
                                    strlcpy(newConfig.discoveryNameTag, haObj["discoveryNameTag"], sizeof(newConfig.discoveryNameTag));
                                }
                                config->setHomeAssistantConfig(newConfig);
                            }
                        }
                    
                        if(configObj.containsKey("debug")) {
                            DebugConfig newConfig;
                            config->getDebugConfig(newConfig);

                            JsonObject debugObj = configObj["debug"];
                            if(debugObj.containsKey("telnet")) {
                                newConfig.telnet = debugObj["telnet"];
                            }
                            if(debugObj.containsKey("serial")) {
                                newConfig.serial = debugObj["serial"];
                            }
                            if(debugObj.containsKey("level")) {
                                newConfig.level = debugObj["level"];
                            }
                            config->setDebugConfig(newConfig);
                        }

                        if(configObj.containsKey("gpio")) {
                            GpioConfig newConfig;
                            config->getGpioConfig(newConfig);

                            JsonObject gpioObj = configObj["gpio"];
                            if(gpioObj.containsKey("apPin")) {
                                newConfig.apPin = gpioObj["apPin"];
                            }
                            if(gpioObj.containsKey("ledPin")) {
                                newConfig.ledPin = gpioObj["ledPin"];
                            }
                            if(gpioObj.containsKey("ledInverted")) {
                                newConfig.ledInverted = gpioObj["ledInverted"];
                            }
                            if(gpioObj.containsKey("ledPinRed")) {
                                newConfig.ledPinRed = gpioObj["ledPinRed"];
                            }
                            if(gpioObj.containsKey("ledPinGreen")) {
                                newConfig.ledPinGreen = gpioObj["ledPinGreen"];
                            }
                            if(gpioObj.containsKey("ledPinBlue")) {
                                newConfig.ledPinBlue = gpioObj["ledPinBlue"];
                            }
                            if(gpioObj.containsKey("ledRgbInverted")) {
                                newConfig.ledRgbInverted = gpioObj["ledRgbInverted"];
                            }
                            if(gpioObj.containsKey("tempSensorPin")) {
                                newConfig.tempSensorPin = gpioObj["tempSensorPin"];
                            }
                            if(gpioObj.containsKey("tempAnalogSensorPin")) {
                                newConfig.tempAnalogSensorPin = gpioObj["tempAnalogSensorPin"];
                            }
                            if(gpioObj.containsKey("vccPin")) {
                                newConfig.vccPin = gpioObj["vccPin"];
                            }
                            if(gpioObj.containsKey("vccOffset")) {
                                newConfig.vccOffset = gpioObj["vccOffset"];
                            }
                            if(gpioObj.containsKey("vccMultiplier")) {
                                newConfig.vccMultiplier = gpioObj["vccMultiplier"];
                            }
                            if(gpioObj.containsKey("vccBootLimit")) {
                                newConfig.vccBootLimit = gpioObj["vccBootLimit"];
                            }
                            if(gpioObj.containsKey("vccResistorGnd")) {
                                newConfig.vccResistorGnd = gpioObj["vccResistorGnd"];
                            }
                            if(gpioObj.containsKey("vccResistorVcc")) {
                                newConfig.vccResistorVcc = gpioObj["vccResistorVcc"];
                            }
                            if(gpioObj.containsKey("ledDisablePin")) {
                                newConfig.ledDisablePin = gpioObj["ledDisablePin"];
                            }
                            if(gpioObj.containsKey("ledBehaviour")) {
                                newConfig.ledBehaviour = gpioObj["ledBehaviour"];
                            }
                            config->setGpioConfig(newConfig);
                        }

                        if(configObj.containsKey("ntp")) {
                            NtpConfig newConfig;
                            config->getNtpConfig(newConfig);

                            JsonObject ntpObj = configObj["ntp"];
                            if(ntpObj.containsKey("enable")) {
                                newConfig.enable = ntpObj["enable"];
                            }
                            if(ntpObj.containsKey("dhcp")) {
                                newConfig.dhcp = ntpObj["dhcp"];
                            }
                            if(ntpObj.containsKey("server")) {
                                strlcpy(newConfig.server, ntpObj["server"], sizeof(newConfig.server));
                            }
                            if(ntpObj.containsKey("timezone")) {
                                strlcpy(newConfig.timezone, ntpObj["timezone"], sizeof(newConfig.timezone));
                            }
                            config->setNtpConfig(newConfig);
                        }

                        if(configObj.containsKey("priceService")) {
                            PriceServiceConfig newConfig;
                            config->getPriceServiceConfig(newConfig);
                            JsonObject priceServiceObj = configObj["priceService"];
                            if(priceServiceObj.containsKey("area")) {
                                strlcpy(newConfig.area, priceServiceObj["area"], sizeof(newConfig.area));
                            }
                            if(priceServiceObj.containsKey("currency")) {
                                strlcpy(newConfig.currency, priceServiceObj["currency"], sizeof(newConfig.currency));
                            }
                            if(priceServiceObj.containsKey("resolutionInMinutes")) {
                                newConfig.resolutionInMinutes = priceServiceObj["resolutionInMinutes"];
                            }
                            if(priceServiceObj.containsKey("enabled")) {
                                newConfig.enabled = priceServiceObj["enabled"];
                            }
                            config->setPriceServiceConfig(newConfig);
                        }

                        if(configObj.containsKey("cloud")) {
                            JsonObject cloudObj = configObj["cloud"];

                            if(cloudObj.containsKey("amsleser")) {
                                CloudConfig newConfig;
                                config->getCloudConfig(newConfig);

                                JsonObject amsCloudObj = cloudObj["amsleser"];
                                if(amsCloudObj.containsKey("enabled")) {
                                    newConfig.enabled = amsCloudObj["enabled"];
                                }
                                if(amsCloudObj.containsKey("interval")) {
                                    newConfig.interval = amsCloudObj["interval"];
                                }
                                if(amsCloudObj.containsKey("hostname")) {
                                    strlcpy(newConfig.hostname, amsCloudObj["hostname"], sizeof(newConfig.hostname));
                                }
                                if(amsCloudObj.containsKey("port")) {
                                    newConfig.port = amsCloudObj["port"];
                                }
                                if(amsCloudObj.containsKey("clientId")) {
                                    strlcpy((char*)newConfig.clientId, amsCloudObj["clientId"], sizeof(newConfig.clientId));
                                }
                                if(amsCloudObj.containsKey("proto")) {
                                    newConfig.proto = amsCloudObj["proto"];
                                }
                                config->setCloudConfig(newConfig);
                            }

                            if(cloudObj.containsKey("zmartCharge")) {
                                ZmartChargeConfig newConfig;
                                config->getZmartChargeConfig(newConfig);
                                JsonObject zmartChargeObj = cloudObj["zmartCharge"];
                                if(zmartChargeObj.containsKey("enabled")) {
                                    newConfig.enabled = zmartChargeObj["enabled"];
                                }
                                if(zmartChargeObj.containsKey("token")) {
                                    strlcpy(newConfig.token, zmartChargeObj["token"], sizeof(newConfig.token));
                                }
                                if(zmartChargeObj.containsKey("baseUrl")) {
                                    strlcpy(newConfig.baseUrl, zmartChargeObj["baseUrl"], sizeof(newConfig.baseUrl));
                                }
                                config->setZmartChargeConfig(newConfig);
                            }
                        }
                    }
                }
            }
        } else if(payload.equals("fwupgrade")) {
            if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                updater->setTargetVersion(updater->getNextVersion());
            }
        } else if(payload.equals("dayplot")) {
            char pubTopic[192];
            snprintf_P(pubTopic, 192, PSTR("%s/dayplot"), mqttConfig.publishTopic);
            AmsJsonGenerator::generateDayPlotJson(ds, json, BufferSize);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
        } else if(payload.equals("monthplot")) {
            char pubTopic[192];
            snprintf_P(pubTopic, 192, PSTR("%s/monthplot"), mqttConfig.publishTopic);
            AmsJsonGenerator::generateMonthPlotJson(ds, json, BufferSize);
            bool ret = mqtt.publish(pubTopic, json);
            loop();
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
