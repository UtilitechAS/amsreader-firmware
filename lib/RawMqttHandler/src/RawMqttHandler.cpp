#include "RawMqttHandler.h"
#include "hexutils.h"
#include "Uptime.h"

bool RawMqttHandler::publish(AmsData* data, AmsData* meterState, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;
        
    if(data->getPackageTimestamp() > 0) {
        mqtt->publish(topic + "/meter/dlms/timestamp", String(data->getPackageTimestamp()));
    }
    switch(data->getListType()) {
        case 4:
            if(full || meterState->getL1ActiveImportPower() != data->getL1ActiveImportPower()) {
                mqtt->publish(topic + "/meter/import/l1", String(data->getL1ActiveImportPower(), 2));
            }
            if(full || meterState->getL2ActiveImportPower() != data->getL2ActiveImportPower()) {
                mqtt->publish(topic + "/meter/import/l2", String(data->getL2ActiveImportPower(), 2));
            }
            if(full || meterState->getL3ActiveImportPower() != data->getL3ActiveImportPower()) {
                mqtt->publish(topic + "/meter/import/l3", String(data->getL3ActiveImportPower(), 2));
            }
            if(full || meterState->getL1ActiveExportPower() != data->getL1ActiveExportPower()) {
                mqtt->publish(topic + "/meter/export/l1", String(data->getL1ActiveExportPower(), 2));
            }
            if(full || meterState->getL2ActiveExportPower() != data->getL2ActiveExportPower()) {
                mqtt->publish(topic + "/meter/export/l2", String(data->getL2ActiveExportPower(), 2));
            }
            if(full || meterState->getL3ActiveExportPower() != data->getL3ActiveExportPower()) {
                mqtt->publish(topic + "/meter/export/l3", String(data->getL3ActiveExportPower(), 2));
            }
            if(full || meterState->getPowerFactor() != data->getPowerFactor()) {
                mqtt->publish(topic + "/meter/powerfactor", String(data->getPowerFactor(), 2));
            }
            if(full || meterState->getL1PowerFactor() != data->getL1PowerFactor()) {
                mqtt->publish(topic + "/meter/l1/powerfactor", String(data->getL1PowerFactor(), 2));
            }
            if(full || meterState->getL2PowerFactor() != data->getL2PowerFactor()) {
                mqtt->publish(topic + "/meter/l2/powerfactor", String(data->getL2PowerFactor(), 2));
            }
            if(full || meterState->getL3PowerFactor() != data->getL3PowerFactor()) {
                mqtt->publish(topic + "/meter/l3/powerfactor", String(data->getL3PowerFactor(), 2));
            }
        case 3:
            // ID and type belongs to List 2, but I see no need to send that every 10s
            mqtt->publish(topic + "/meter/id", data->getMeterId(), true, 0);
            mqtt->publish(topic + "/meter/type", data->getMeterModel(), true, 0);
            mqtt->publish(topic + "/meter/clock", String(data->getMeterTimestamp()));
            mqtt->publish(topic + "/meter/import/reactive/accumulated", String(data->getReactiveImportCounter(), 3), true, 0);
            mqtt->publish(topic + "/meter/import/active/accumulated", String(data->getActiveImportCounter(), 3), true, 0);
            mqtt->publish(topic + "/meter/export/reactive/accumulated", String(data->getReactiveExportCounter(), 3), true, 0);
            mqtt->publish(topic + "/meter/export/active/accumulated", String(data->getActiveExportCounter(), 3), true, 0);
        case 2:
            // Only send data if changed. ID and Type is sent on the 10s interval only if changed
            if(full || meterState->getMeterId() != data->getMeterId()) {
                mqtt->publish(topic + "/meter/id", data->getMeterId());
            }
            if(full || meterState->getMeterModel() != data->getMeterModel()) {
                mqtt->publish(topic + "/meter/type", data->getMeterModel());
            }
            if(full || meterState->getL1Current() != data->getL1Current()) {
                mqtt->publish(topic + "/meter/l1/current", String(data->getL1Current(), 2));
            }
            if(full || meterState->getL1Voltage() != data->getL1Voltage()) {
                mqtt->publish(topic + "/meter/l1/voltage", String(data->getL1Voltage(), 2));
            }
            if(full || meterState->getL2Current() != data->getL2Current()) {
                mqtt->publish(topic + "/meter/l2/current", String(data->getL2Current(), 2));
            }
            if(full || meterState->getL2Voltage() != data->getL2Voltage()) {
                mqtt->publish(topic + "/meter/l2/voltage", String(data->getL2Voltage(), 2));
            }
            if(full || meterState->getL3Current() != data->getL3Current()) {
                mqtt->publish(topic + "/meter/l3/current", String(data->getL3Current(), 2));
            }
            if(full || meterState->getL3Voltage() != data->getL3Voltage()) {
                mqtt->publish(topic + "/meter/l3/voltage", String(data->getL3Voltage(), 2));
            }
            if(full || meterState->getReactiveExportPower() != data->getReactiveExportPower()) {
                mqtt->publish(topic + "/meter/export/reactive", String(data->getReactiveExportPower()));
            }
            if(full || meterState->getActiveExportPower() != data->getActiveExportPower()) {
                mqtt->publish(topic + "/meter/export/active", String(data->getActiveExportPower()));
            }
            if(full || meterState->getReactiveImportPower() != data->getReactiveImportPower()) {
                mqtt->publish(topic + "/meter/import/reactive", String(data->getReactiveImportPower()));
            }
        case 1:
            if(full || meterState->getActiveImportPower() != data->getActiveImportPower()) {
                mqtt->publish(topic + "/meter/import/active", String(data->getActiveImportPower()));
            }
    }
    mqtt->publish(topic + "/realtime/import/hour", String(ea->getUseThisHour(), 3));
    mqtt->publish(topic + "/realtime/import/day", String(ea->getUseToday(), 2));
    mqtt->publish(topic + "/realtime/import/month", String(ea->getUseThisMonth(), 1));
    uint8_t peakCount = ea->getConfig()->hours;
    if(peakCount > 5) peakCount = 5;
    for(uint8_t i = 1; i <= peakCount; i++) {
        mqtt->publish(topic + "/realtime/import/peak/" + String(i, 10), String(ea->getPeak(i).value / 100.0, 10), true, 0);
    }
    mqtt->publish(topic + "/realtime/import/threshold", String(ea->getCurrentThreshold(), 10), true, 0);
    mqtt->publish(topic + "/realtime/import/monthmax", String(ea->getMonthMax(), 3), true, 0);
    mqtt->publish(topic + "/realtime/export/hour", String(ea->getProducedThisHour(), 3));
    mqtt->publish(topic + "/realtime/export/day", String(ea->getProducedToday(), 2));
    mqtt->publish(topic + "/realtime/export/month", String(ea->getProducedThisMonth(), 1));
    return true;
}

bool RawMqttHandler::publishTemperatures(AmsConfiguration* config, HwTools* hw) {
    uint8_t c = hw->getTempSensorCount();
    for(int i = 0; i < c; i++) {
        TempSensorData* data = hw->getTempSensorData(i);
        if(data != NULL && data->lastValidRead > -85) {
            if(data->changed || full) {
                mqtt->publish(topic + "/temperature/" + toHex(data->address), String(data->lastValidRead, 2));
                data->changed = false;
            }
        }
    }
    return c > 0;
}

bool RawMqttHandler::publishPrices(EntsoeApi* eapi) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;
	if(eapi->getValueForHour(0) == ENTSOE_NO_VALUE)
		return false;

	time_t now = time(nullptr);

	float min1hr = 0.0, min3hr = 0.0, min6hr = 0.0;
	int8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[34];
    for(int i = 0;i < 34; i++) values[i] = ENTSOE_NO_VALUE;
	for(uint8_t i = 0; i < 34; i++) {
		float val = eapi->getValueForHour(now, i);
		values[i] = val;

        if(i > 23) continue;
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
	if(min1hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min1hrIdx);
        //Serial.printf("1hr: %d %lu\n", min1hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[24];
	if(min3hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min3hrIdx);
        //Serial.printf("3hr: %d %lu\n", min3hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[24];
	if(min6hrIdx > -1) {
        time_t ts = now + (SECS_PER_HOUR * min6hrIdx);
        //Serial.printf("6hr: %d %lu\n", min6hrIdx, ts);
		tmElements_t tm;
        breakTime(ts, tm);
		sprintf(ts6hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

    for(int i = 0; i < 34; i++) {
        float val = values[i];
        if(val == ENTSOE_NO_VALUE) {
            mqtt->publish(topic + "/price/" + String(i), "", true, 0);
        } else {
            mqtt->publish(topic + "/price/" + String(i), String(val, 4), true, 0);
        }
        mqtt->loop();
        delay(10);
    }
    if(min != INT16_MAX) {
        mqtt->publish(topic + "/price/min", String(min, 4), true, 0);
    }
    if(max != INT16_MIN) {
        mqtt->publish(topic + "/price/max", String(max, 4), true, 0);
    }

    if(min1hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/1hr", String(ts1hr), true, 0);
    }
    if(min3hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/3hr", String(ts3hr), true, 0);
    }
    if(min6hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/6hr", String(ts6hr), true, 0);
    }
    return true;
}

bool RawMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

	mqtt->publish(topic + "/id", WiFi.macAddress(), true, 0);
	mqtt->publish(topic + "/uptime", String((unsigned long) millis64()/1000));
	float vcc = hw->getVcc();
	if(vcc > 0) {
		mqtt->publish(topic + "/vcc", String(vcc, 2));
	}
	mqtt->publish(topic + "/mem", String(ESP.getFreeHeap()));
	mqtt->publish(topic + "/rssi", String(hw->getWifiRssi()));
    if(hw->getTemperature() > -85) {
		mqtt->publish(topic + "/temperature", String(hw->getTemperature(), 2));
    }
    return true;
}