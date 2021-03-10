#include "RawMqttHandler.h"
#include "hexutils.h"
#include "Uptime.h"

bool RawMqttHandler::publish(AmsData* data, AmsData* meterState) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;
        
    if(data->getPackageTimestamp() > 0) {
        mqtt->publish(topic + "/meter/dlms/timestamp", String(data->getPackageTimestamp()));
    }
    switch(data->getListType()) {
        case 3:
            // ID and type belongs to List 2, but I see no need to send that every 10s
            mqtt->publish(topic + "/meter/id", data->getMeterId(), true, 0);
            mqtt->publish(topic + "/meter/type", data->getMeterModel(), true, 0);
            mqtt->publish(topic + "/meter/clock", String(data->getMeterTimestamp()));
            mqtt->publish(topic + "/meter/import/reactive/accumulated", String(data->getReactiveImportCounter(), 2), true, 0);
            mqtt->publish(topic + "/meter/import/active/accumulated", String(data->getActiveImportCounter(), 2), true, 0);
            mqtt->publish(topic + "/meter/export/reactive/accumulated", String(data->getReactiveExportCounter(), 2), true, 0);
            mqtt->publish(topic + "/meter/export/active/accumulated", String(data->getActiveExportCounter(), 2), true, 0);
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
	if(strcmp(eapi->getToken(), "") != 0)
		return false;

	time_t now = time(nullptr);

	float min1hr, min3hr, min6hr;
	uint8_t min1hrIdx = -1, min3hrIdx = -1, min6hrIdx = -1;
	float min = INT16_MAX, max = INT16_MIN;
	float values[24] = {0};
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
	if(min1hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min1hrIdx), tm);
		sprintf(ts1hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts3hr[21];
	if(min3hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min3hrIdx), tm);
		sprintf(ts3hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}
	char ts6hr[21];
	if(min6hrIdx != -1) {
		tmElements_t tm;
        breakTime(now + (SECS_PER_HOUR * min6hrIdx), tm);
		sprintf(ts6hr, "%04d-%02d-%02dT%02d:00:00Z", tm.Year+1970, tm.Month, tm.Day, tm.Hour);
	}

    for(int i = 0; i < 24; i++) {
        float val = values[i];
        if(val == ENTSOE_NO_VALUE) {
            mqtt->publish(topic + "/price/" + String(i), "");
            break;
        } else {
            mqtt->publish(topic + "/price/" + String(i), String(val, 4));
        }
        mqtt->loop();
        delay(10);
    }
    if(min != INT16_MAX) {
        mqtt->publish(topic + "/price/min", String(min, 4));
    }
    if(max != INT16_MIN) {
        mqtt->publish(topic + "/price/max", String(max, 4));
    }

    if(min1hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/1hr", String(ts1hr));
    }
    if(min3hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/3hr", String(ts3hr));
    }
    if(min6hrIdx != -1) {
        mqtt->publish(topic + "/price/cheapest/6hr", String(ts6hr));
    }
    return true;
}

bool RawMqttHandler::publishSystem(HwTools* hw) {
	if(topic.isEmpty() || !mqtt->connected())
		return false;

	mqtt->publish(topic + "/id", WiFi.macAddress());
	mqtt->publish(topic + "/uptime", String((unsigned long) millis64()/1000));
	float vcc = hw->getVcc();
	if(vcc > 0) {
		mqtt->publish(topic + "/vcc", String(vcc, 2));
	}
	mqtt->publish(topic + "/rssi", String(hw->getWifiRssi()));
    if(hw->getTemperature() > -85) {
		mqtt->publish(topic + "/temperature", String(hw->getTemperature(), 2));
    }
    return true;
}