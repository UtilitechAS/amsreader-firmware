/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "RawMqttHandler.h"
#include "hexutils.h"
#include "Uptime.h"
#include "FirmwareVersion.h"

bool RawMqttHandler::publish(AmsData* update, AmsData* previousState, EnergyAccounting* ea, PriceService* ps) {
	if(topic.isEmpty() || !connected())
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
        
    if(data.getPackageTimestamp() > 0) {
        mqtt.publish(topic + "/meter/dlms/timestamp", String(data.getPackageTimestamp()));
    }
    switch(data.getListType()) {
        case 4:
            publishList4(&data, previousState);
            loop();
        case 3:
            publishList3(&data, previousState);
            loop();
        case 2:
            publishList2(&data, previousState);
            loop();
        case 1:
            publishList1(&data, previousState);
            loop();
    }

    if(data.getListType() >= 2 && data.getActiveExportPower() > 0.0) {
        hasExport = true;
    }

    if(data.getListType() >= 3 && data.getActiveExportCounter() > 0.0) {
        hasExport = true;
    }

    if(ea->isInitialized()) {
        publishRealtime(ea);
        loop();
    }
    return true;
}

bool RawMqttHandler::publishList1(AmsData* data, AmsData* meterState) {
    if(full || meterState->getActiveImportPower() != data->getActiveImportPower()) {
        mqtt.publish(topic + "/meter/import/active", String(data->getActiveImportPower()));
    }
    return true;
}

bool RawMqttHandler::publishList2(AmsData* data, AmsData* meterState) {
    // Only send data if changed. ID and Type is sent on the 10s interval only if changed
    if(full || meterState->getMeterId() != data->getMeterId()) {
        mqtt.publish(topic + "/meter/id", data->getMeterId());
    }
    if(full || meterState->getMeterModel() != data->getMeterModel()) {
        mqtt.publish(topic + "/meter/type", data->getMeterModel());
    }
    loop();
    if(full || meterState->getL1Current() != data->getL1Current()) {
        mqtt.publish(topic + "/meter/l1/current", String(data->getL1Current(), 2));
    }
    if(full || meterState->getL1Voltage() != data->getL1Voltage()) {
        mqtt.publish(topic + "/meter/l1/voltage", String(data->getL1Voltage(), 2));
    }
    loop();
    if(full || meterState->getL2Current() != data->getL2Current()) {
        mqtt.publish(topic + "/meter/l2/current", String(data->getL2Current(), 2));
    }
    if(full || meterState->getL2Voltage() != data->getL2Voltage()) {
        mqtt.publish(topic + "/meter/l2/voltage", String(data->getL2Voltage(), 2));
    }
    loop();
    if(full || meterState->getL3Current() != data->getL3Current()) {
        mqtt.publish(topic + "/meter/l3/current", String(data->getL3Current(), 2));
    }
    if(full || meterState->getL3Voltage() != data->getL3Voltage()) {
        mqtt.publish(topic + "/meter/l3/voltage", String(data->getL3Voltage(), 2));
    }
    loop();
    if(full || meterState->getReactiveExportPower() != data->getReactiveExportPower()) {
        mqtt.publish(topic + "/meter/export/reactive", String(data->getReactiveExportPower()));
    }
    if(full || meterState->getActiveExportPower() != data->getActiveExportPower()) {
        mqtt.publish(topic + "/meter/export/active", String(data->getActiveExportPower()));
    }
    if(full || meterState->getReactiveImportPower() != data->getReactiveImportPower()) {
        mqtt.publish(topic + "/meter/import/reactive", String(data->getReactiveImportPower()));
    }
    return true;
}

bool RawMqttHandler::publishList3(AmsData* data, AmsData* meterState) {
    // ID and type belongs to List 2, but I see no need to send that every 10s
    mqtt.publish(topic + "/meter/id", data->getMeterId(), true, 0);
    mqtt.publish(topic + "/meter/type", data->getMeterModel(), true, 0);
    mqtt.publish(topic + "/meter/clock", String(data->getMeterTimestamp()));
    mqtt.publish(topic + "/meter/import/reactive/accumulated", String(data->getReactiveImportCounter(), 3), true, 0);
    mqtt.publish(topic + "/meter/import/active/accumulated", String(data->getActiveImportCounter(), 3), true, 0);
    mqtt.publish(topic + "/meter/export/reactive/accumulated", String(data->getReactiveExportCounter(), 3), true, 0);
    mqtt.publish(topic + "/meter/export/active/accumulated", String(data->getActiveExportCounter(), 3), true, 0);
    return true;
}

bool RawMqttHandler::publishList4(AmsData* data, AmsData* meterState) {
        if(full || meterState->getL1ActiveImportPower() != data->getL1ActiveImportPower()) {
            mqtt.publish(topic + "/meter/import/l1", String(data->getL1ActiveImportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL2ActiveImportPower() != data->getL2ActiveImportPower()) {
            mqtt.publish(topic + "/meter/import/l2", String(data->getL2ActiveImportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL3ActiveImportPower() != data->getL3ActiveImportPower()) {
            mqtt.publish(topic + "/meter/import/l3", String(data->getL3ActiveImportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL1ActiveExportPower() != data->getL1ActiveExportPower()) {
            mqtt.publish(topic + "/meter/export/l1", String(data->getL1ActiveExportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL2ActiveExportPower() != data->getL2ActiveExportPower()) {
            mqtt.publish(topic + "/meter/export/l2", String(data->getL2ActiveExportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL3ActiveExportPower() != data->getL3ActiveExportPower()) {
            mqtt.publish(topic + "/meter/export/l3", String(data->getL3ActiveExportPower()));
            mqtt.loop();
        }
        if(full || meterState->getL1ActiveImportCounter() != data->getL1ActiveImportCounter()) {
            mqtt.publish(topic + "/meter/import/l1/accumulated", String(data->getL1ActiveImportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL2ActiveImportCounter() != data->getL2ActiveImportCounter()) {
            mqtt.publish(topic + "/meter/import/l2/accumulated", String(data->getL2ActiveImportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL3ActiveImportCounter() != data->getL3ActiveImportCounter()) {
            mqtt.publish(topic + "/meter/import/l3/accumulated", String(data->getL3ActiveImportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL1ActiveExportCounter() != data->getL1ActiveExportCounter()) {
            mqtt.publish(topic + "/meter/export/l1/accumulated", String(data->getL1ActiveExportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL2ActiveExportCounter() != data->getL2ActiveExportCounter()) {
            mqtt.publish(topic + "/meter/export/l2/accumulated", String(data->getL2ActiveExportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL3ActiveExportCounter() != data->getL3ActiveExportCounter()) {
            mqtt.publish(topic + "/meter/export/l3/accumulated", String(data->getL3ActiveExportCounter(), 2));
            mqtt.loop();
        }
        if(full || meterState->getPowerFactor() != data->getPowerFactor()) {
            mqtt.publish(topic + "/meter/powerfactor", String(data->getPowerFactor(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL1PowerFactor() != data->getL1PowerFactor()) {
            mqtt.publish(topic + "/meter/l1/powerfactor", String(data->getL1PowerFactor(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL2PowerFactor() != data->getL2PowerFactor()) {
            mqtt.publish(topic + "/meter/l2/powerfactor", String(data->getL2PowerFactor(), 2));
            mqtt.loop();
        }
        if(full || meterState->getL3PowerFactor() != data->getL3PowerFactor()) {
            mqtt.publish(topic + "/meter/l3/powerfactor", String(data->getL3PowerFactor(), 2));
            mqtt.loop();
        }
        return true;
}

bool RawMqttHandler::publishRealtime(EnergyAccounting* ea) {
    mqtt.publish(topic + "/realtime/import/hour", String(ea->getUseThisHour(), 3));
    mqtt.loop();
    mqtt.publish(topic + "/realtime/import/day", String(ea->getUseToday(), 2));
    mqtt.loop();
    mqtt.publish(topic + "/realtime/import/month", String(ea->getUseThisMonth(), 1));
    mqtt.loop();
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 1; i <= peakCount; i++) {
        mqtt.publish(topic + "/realtime/import/peak/" + String(i, 10), String(ea->getPeak(i).value / 100.0, 10), true, 0);
        mqtt.loop();
    }
    mqtt.publish(topic + "/realtime/import/threshold", String(ea->getCurrentThreshold(), 10), true, 0);
    mqtt.loop();
    mqtt.publish(topic + "/realtime/import/monthmax", String(ea->getMonthMax(), 3), true, 0);
    mqtt.loop();
    mqtt.publish(topic + "/realtime/export/hour", String(ea->getProducedThisHour(), 3));
    mqtt.loop();
    mqtt.publish(topic + "/realtime/export/day", String(ea->getProducedToday(), 2));
    mqtt.loop();
    mqtt.publish(topic + "/realtime/export/month", String(ea->getProducedThisMonth(), 1));
    mqtt.loop();
    uint32_t now = millis();
    if(lastThresholdPublish == 0 || now-lastThresholdPublish > 3600000) {
        EnergyAccountingConfig* conf = ea->getConfig();
        for(uint8_t i = 0; i < 9; i++) {
            mqtt.publish(topic + "/realtime/import/thresholds/" + String(i+1, 10), String(conf->thresholds[i], 10), true, 0);
            mqtt.loop();
        }
        lastThresholdPublish = now;
    }
    return true;
}

bool RawMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
    uint8_t c = hw->getTempSensorCount();
    for(int i = 0; i < c; i++) {
        TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL && data->lastValidRead > -85) {
            if(data->changed || full) {
                mqtt.publish(topic + "/temperature/" + toHex(data->address), String(data->lastValidRead, 2));
                mqtt.loop();
                data->changed = false;
            }
        }
    }
    return c > 0;
}

bool RawMqttHandler::publishPrices(PriceService* ps) {
	if(topic.isEmpty() || !connected())
		return false;
	if(!ps->hasPrice())
		return false;

	time_t now = time(nullptr);

	float min1hr = 0.0, min3hr = 0.0, min6hr = 0.0;
	int8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[34];
    for(int i = 0;i < 34; i++) values[i] = PRICE_NO_VALUE;
	for(uint8_t i = 0; i < 34; i++) {
		float val = ps->getPriceForRelativeHour(PRICE_DIRECTION_IMPORT, i);
		values[i] = val;

        if(i > 23) continue;
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
	if(min1hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min1hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[24];
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[24];
	if(min6hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min6hrIdx);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts6hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

    mqtt.publish(topic + "/price/resolution", String(ps->getResolutionInMinutes()), true, 0);
    mqtt.loop();

    uint8_t relativeIndex = 0;
    uint8_t startIndex = ps->getCurrentPricePointIndex();
    uint8_t numberOfPoints = ps->getNumberOfPointsAvailable();
	for(int i = startIndex; i < numberOfPoints; i++) {
		float importVal = ps->getPricePoint(PRICE_DIRECTION_IMPORT, i);
        if(importVal == PRICE_NO_VALUE) {
            mqtt.publish(topic + "/price/import/" + String(relativeIndex), "", true, 0);
            mqtt.loop();
        } else {
            mqtt.publish(topic + "/price/import/" + String(relativeIndex), String(importVal, 4), true, 0);
            mqtt.loop();
        }

        if(hasExport && ps->isExportPricesDifferentFromImport()) {
            float exportVal = ps->getPricePoint(PRICE_DIRECTION_EXPORT, i);
            if(exportVal == PRICE_NO_VALUE) {
                mqtt.publish(topic + "/price/export/" + String(relativeIndex), "", true, 0);
                mqtt.loop();
            } else {
                mqtt.publish(topic + "/price/export/" + String(relativeIndex), String(exportVal, 4), true, 0);
                mqtt.loop();
            }
        }
        relativeIndex++;
    }
    if(min != INT16_MAX) {
        mqtt.publish(topic + "/price/min", String(min, 4), true, 0);
        mqtt.loop();
    }
    if(max != INT16_MIN) {
        mqtt.publish(topic + "/price/max", String(max, 4), true, 0);
        mqtt.loop();
    }
    if(min1hrIdx != -1) {
        mqtt.publish(topic + "/price/cheapest/1hr", String(ts1hr), true, 0);
        mqtt.loop();
    }
    if(min3hrIdx != -1) {
        mqtt.publish(topic + "/price/cheapest/3hr", String(ts3hr), true, 0);
        mqtt.loop();
    }
    if(min6hrIdx != -1) {
        mqtt.publish(topic + "/price/cheapest/6hr", String(ts6hr), true, 0);
        mqtt.loop();
    }
    return true;
}

bool RawMqttHandler::publishSystem(HwTools* hw, PriceService* ps, EnergyAccounting* ea) {
	if(topic.isEmpty() || !connected())
		return false;

	mqtt.publish(topic + "/id", WiFi.macAddress(), true, 0);
    mqtt.loop();
	mqtt.publish(topic + "/uptime", String((uint32_t) (millis64()/1000)));
    mqtt.loop();
	float vcc = hw->getVcc();
	if(vcc > 0) {
		mqtt.publish(topic + "/vcc", String(vcc, 2));
        mqtt.loop();
	}
	mqtt.publish(topic + "/mem", String(ESP.getFreeHeap()));
    mqtt.loop();
	mqtt.publish(topic + "/rssi", String(hw->getWifiRssi()));
    mqtt.loop();
    if(hw->getTemperature() > -85) {
		mqtt.publish(topic + "/temperature", String(hw->getTemperature(), 2));
        mqtt.loop();
    }
    return true;
}

uint8_t RawMqttHandler::getFormat() {
    return full ? 3 : 2;
}

bool RawMqttHandler::publishRaw(String data) {
    return false;
}

void RawMqttHandler::onMessage(String &topic, String &payload) {
    if(topic.equals(subTopic)) {
        if(payload.equals("fwupgrade")) {
            if(strcmp(updater->getNextVersion(), FirmwareVersion::VersionString) != 0) {
                updater->setTargetVersion(updater->getNextVersion());
            }
        }
    }
}
