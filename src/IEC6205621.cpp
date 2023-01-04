#include "IEC6205621.h"

IEC6205621::IEC6205621(const char* p) {
	if(strlen(p) < 16)
		return;

	String payload(p+1);

	lastUpdateMillis = millis();
	listId = payload.substring(payload.startsWith("/") ? 1 : 0, payload.indexOf("\n"));

	if(listId.startsWith("ADN")) {
		meterType = AmsTypeAidon;
		listId = listId.substring(0,4);
	} else if(listId.startsWith("KFM")) {
		meterType = AmsTypeKaifa;
		listId = listId.substring(0,4);
	} else if(listId.startsWith("KMP")) {
		meterType = AmsTypeKamstrup;
		listId = listId.substring(0,4);
	} else if(listId.startsWith("ISk")) {
		meterType = AmsTypeIskra;
		listId = listId.substring(0,5);
	} else if(listId.startsWith("XMX")) {
		meterType = AmsTypeLandisGyr;
		listId = listId.substring(0,6);
	} else if(listId.startsWith("Ene") || listId.startsWith("EST")) {
		meterType = AmsTypeSagemcom;
		listId = listId.substring(0,4);
	} else if(listId.startsWith("LGF")) {
		meterType = AmsTypeLandisGyr;
		listId = listId.substring(0,4);
	} else {
		meterType = AmsTypeUnknown;
		listId = listId.substring(0,4);
	}
	
	meterId = extract(payload, "96.1.0");
	if(meterId.isEmpty()) {
		meterId = extract(payload, "0.0.5");
	}

	meterModel = extract(payload, "96.1.1");
	if(meterModel.isEmpty()) {
		meterModel = extract(payload, "96.1.7");
		if(meterModel.isEmpty()) {
			meterModel = payload.substring(payload.indexOf(listId) + listId.length(), payload.indexOf("\n"));
			meterModel.trim();
		}
	}

	String timestamp = extract(payload, "1.0.0");
	if(timestamp.length() > 10) {
		tmElements_t tm;
		tm.Year = (timestamp.substring(0,2).toInt() + 2000) - 1970;
		tm.Month = timestamp.substring(4,6).toInt();
		tm.Day = timestamp.substring(2,4).toInt();
		tm.Hour = timestamp.substring(6,8).toInt();
		tm.Minute = timestamp.substring(8,10).toInt();
		tm.Second = timestamp.substring(10,12).toInt();
		meterTimestamp = makeTime(tm); // TODO: Adjust for time zone
	}

	activeImportPower = (uint16_t) (extractDouble(payload, "1.7.0"));
	activeExportPower = (uint16_t) (extractDouble(payload, "2.7.0"));
	reactiveImportPower = (uint16_t) (extractDouble(payload, "3.7.0"));
	reactiveExportPower = (uint16_t) (extractDouble(payload, "4.7.0"));

	if(activeImportPower > 0)
		listType = 1;
	
	l1voltage = extractDouble(payload, "32.7.0");
	l2voltage = extractDouble(payload, "52.7.0");
	l3voltage = extractDouble(payload, "72.7.0");

	l1current = extractDouble(payload, "31.7.0");
	l2current = extractDouble(payload, "51.7.0");
	l3current = extractDouble(payload, "71.7.0");

	l1activeImportPower = extractDouble(payload, "21.7.0");
	l2activeImportPower = extractDouble(payload, "41.7.0");
	l3activeImportPower = extractDouble(payload, "61.7.0");
	
	l1activeExportPower = extractDouble(payload, "22.7.0");
	l2activeExportPower = extractDouble(payload, "42.7.0");
	l3activeExportPower = extractDouble(payload, "62.7.0");
	
	if(l1voltage > 0 || l2voltage > 0 || l3voltage > 0)
		listType = 2;

	double val = 0.0;
	
	val = extractDouble(payload, "1.8.0");
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "1.8." + String(i,10));
		}
	}
	if(val > 0) activeImportCounter = val / 1000;

	val = extractDouble(payload, "2.8.0");
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "2.8." + String(i,10));
		}
	}
	if(val > 0) activeExportCounter = val / 1000;

	val = extractDouble(payload, "3.8.0");
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "3.8." + String(i,10));
		}
	}
	if(val > 0) reactiveImportCounter = val / 1000;

	val = extractDouble(payload, "4.8.0");
	if(val == 0) {
		for(int i = 1; i < 9; i++) {
			val += extractDouble(payload, "4.8." + String(i,10));
		}
	}
	if(val > 0) reactiveExportCounter = val / 1000;

	if(activeImportCounter > 0 || activeExportCounter > 0 || reactiveImportCounter > 0 || reactiveExportCounter > 0)
		listType = 3;

	threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
	twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);

	if(threePhase) {
		if(l2current == 0 && l1current != 0 && l3current != 0) {
			l2current = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
		}
	}

	if (l1activeImportPower > 0 || l2activeImportPower > 0 || l3activeImportPower > 0 || l1activeExportPower > 0 || l2activeExportPower > 0 || l3activeExportPower > 0)
		listType = 4;
}

String IEC6205621::extract(String payload, String obis) {
	int a = payload.indexOf(String(":" + obis + "("));
	if(a > 0) {
		int b = payload.indexOf(")", a);
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

	int a = str.indexOf("*");
	String val = str.substring(0,a);
	String unit = str.substring(a+1);

	return unit.startsWith("k") ? val.toDouble() * 1000 : val.toDouble();
}
