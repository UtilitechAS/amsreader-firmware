#include "AmsJsonGenerator.h"

void AmsJsonGenerator::generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize) {
		uint16_t pos = snprintf_P(buf, bufSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 0; i < 24; i++) {
			pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getHourImport(i) / 1000.0, i, ds->getHourExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, bufSize-pos, PSTR("}"));
}

void AmsJsonGenerator::generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize) {
		uint16_t pos = snprintf_P(buf, bufSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 1; i < 32; i++) {
			pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getDayImport(i) / 1000.0, i, ds->getDayExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, bufSize-pos, PSTR("}"));
}
