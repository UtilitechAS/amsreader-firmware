#include "IEC6205621.h"
#include "Uptime.h"

IEC6205621::IEC6205621(const char* p, Timezone* tz, MeterConfig* meterConfig) {
	if(strlen(p) < 16)
		return;

	String payload(p+1);

	lastUpdateMillis = millis64();
	listId = payload.substring(payload.startsWith("/") ? 1 : 0, payload.indexOf("\n"));

	if(listId.startsWith(F("ADN"))) {
		meterType = AmsTypeAidon;
		listId = listId.substring(0,4);
	} else if(listId.startsWith(F("KFM"))) {
		meterType = AmsTypeKaifa;
		listId = listId.substring(0,4);
	} else if(listId.startsWith(F("KMP"))) {
		meterType = AmsTypeKamstrup;
		listId = listId.substring(0,4);
	} else if(listId.startsWith(F("KAM"))) {
		meterType = AmsTypeKamstrup;
		listId = listId.substring(0,4);
	} else if(listId.startsWith(F("ISk"))) {
		meterType = AmsTypeIskra;
		listId = listId.substring(0,5);
	} else if(listId.startsWith(F("XMX"))) {
		meterType = AmsTypeLandisGyr;
		listId = listId.substring(0,6);
	} else if(listId.startsWith(F("Ene")) || listId.startsWith(F("EST"))) {
		meterType = AmsTypeSagemcom;
		listId = listId.substring(0,4);
	} else if(listId.startsWith(F("LGF"))) {
		meterType = AmsTypeLandisGyr;
		listId = listId.substring(0,4);
	} else {
		meterType = AmsTypeUnknown;
		listId = listId.substring(0,4);
	}
	
	meterId = extract(payload, F("96.1.0"));
	if(meterId.isEmpty()) {
		meterId = extract(payload, F("0.0.5"));
	}

	meterModel = extract(payload, F("96.1.1"));
	if(meterModel.isEmpty()) {
		meterModel = extract(payload, F("96.1.7"));
		if(meterModel.isEmpty()) {
			meterModel = payload.substring(payload.indexOf(listId) + listId.length(), payload.indexOf(F("\n")));
			meterModel.trim();
		}
	}

	String timestamp = extract(payload, F("1.0.0"));
	if(timestamp.length() > 10) {
		tmElements_t tm;
		tm.Year = (timestamp.substring(0,2).toInt() + 2000) - 1970;
		tm.Month = timestamp.substring(4,6).toInt();
		tm.Day = timestamp.substring(2,4).toInt();
		tm.Hour = timestamp.substring(6,8).toInt();
		tm.Minute = timestamp.substring(8,10).toInt();
		tm.Second = timestamp.substring(10,12).toInt();
		meterTimestamp = makeTime(tm);
		if(tz != NULL) meterTimestamp = tz->toUTC(meterTimestamp);
	}

	activeImportPower = (uint16_t) (extractDouble(payload, F("1.7.0")));
	activeExportPower = (uint16_t) (extractDouble(payload, F("2.7.0")));
	reactiveImportPower = (uint16_t) (extractDouble(payload, F("3.7.0")));
	reactiveExportPower = (uint16_t) (extractDouble(payload, F("4.7.0")));

	if(activeImportPower > 0)
		listType = 1;
	
	l1voltage = extractFloat(payload, F("32.7.0"));
	l2voltage = extractFloat(payload, F("52.7.0"));
	l3voltage = extractFloat(payload, F("72.7.0"));

	l1current = extractFloat(payload, F("31.7.0"));
	l2current = extractFloat(payload, F("51.7.0"));
	l3current = extractFloat(payload, F("71.7.0"));

	l1activeImportPower = extractFloat(payload, F("21.7.0"));
	l2activeImportPower = extractFloat(payload, F("41.7.0"));
	l3activeImportPower = extractFloat(payload, F("61.7.0"));
	
	l1activeExportPower = extractFloat(payload, F("22.7.0"));
	l2activeExportPower = extractFloat(payload, F("42.7.0"));
	l3activeExportPower = extractFloat(payload, F("62.7.0"));
	
	if(l1voltage > 0 || l2voltage > 0 || l3voltage > 0)
		listType = 2;

	double val = 0.0;
	
	val = extractDouble(payload, F("1.8.0"));
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "1.8." + String(i,10));
		}
	}
	if(val > 0) activeImportCounter = val / 1000;

	val = extractDouble(payload, F("2.8.0"));
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "2.8." + String(i,10));
		}
	}
	if(val > 0) activeExportCounter = val / 1000;

	val = extractDouble(payload, F("3.8.0"));
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "3.8." + String(i,10));
		}
	}
	if(val > 0) reactiveImportCounter = val / 1000;

	val = extractDouble(payload, F("4.8.0"));
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "4.8." + String(i,10));
		}
	}
	if(val > 0) reactiveExportCounter = val / 1000;

	if(activeImportCounter > 0 || activeExportCounter > 0 || reactiveImportCounter > 0 || reactiveExportCounter > 0)
		listType = 3;

	if (l1activeImportPower > 0 || l2activeImportPower > 0 || l3activeImportPower > 0 || l1activeExportPower > 0 || l2activeExportPower > 0 || l3activeExportPower > 0)
		listType = 4;

    if(meterConfig->wattageMultiplier > 0) {
        activeImportPower = activeImportPower > 0 ? activeImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        activeExportPower = activeExportPower > 0 ? activeExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        reactiveImportPower = reactiveImportPower > 0 ? reactiveImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        reactiveExportPower = reactiveExportPower > 0 ? reactiveExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->voltageMultiplier > 0) {
        l1voltage = l1voltage > 0 ? l1voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
        l2voltage = l2voltage > 0 ? l2voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
        l3voltage = l3voltage > 0 ? l3voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->amperageMultiplier > 0) {
        l1current = l1current > 0 ? l1current * (meterConfig->amperageMultiplier / 1000.0) : 0;
        l2current = l2current > 0 ? l2current * (meterConfig->amperageMultiplier / 1000.0) : 0;
        l3current = l3current > 0 ? l3current * (meterConfig->amperageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->accumulatedMultiplier > 0) {
        activeImportCounter = activeImportCounter > 0 ? activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        activeExportCounter = activeExportCounter > 0 ? activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        reactiveImportCounter = reactiveImportCounter > 0 ? reactiveImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        reactiveExportCounter = reactiveExportCounter > 0 ? reactiveExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
    }

	threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
	twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);
}

String IEC6205621::extract(String payload, String obis) {
	int a = payload.indexOf(String(":" + obis + "("));
	if(a > 0) {
		int b = payload.indexOf(F(")"), a);
		if(b > a) {
			return payload.substring(a+obis.length()+2, b);
		}
	}
	return "";
}

double IEC6205621::extractDouble(String payload, String obis) {
	String str = extract(payload, obis);
	if(str.isEmpty()) {
		return 0.0;
	}

	int a = str.indexOf(F("*"));
	String val = str.substring(0,a);
	String unit = str.substring(a+1);

	return unit.startsWith(F("k")) ? val.toDouble() * 1000 : val.toDouble();
}

float IEC6205621::extractFloat(String payload, String obis) {
	String str = extract(payload, obis);
	if(str.isEmpty()) {
		return 0.0;
	}

	int a = str.indexOf(F("*"));
	String val = str.substring(0,a);
	String unit = str.substring(a+1);

	return unit.startsWith(F("k")) ? val.toFloat() * 1000 : val.toFloat();
}
