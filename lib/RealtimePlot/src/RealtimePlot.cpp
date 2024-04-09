/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "RealtimePlot.h"
#include <stdlib.h>

RealtimePlot::RealtimePlot() {
    values = (int8_t*) malloc(REALTIME_SIZE);
    scaling = (uint8_t*) malloc(REALTIME_SIZE);
    memset(values, 0, REALTIME_SIZE);
    memset(scaling, 0, REALTIME_SIZE);
}

void RealtimePlot::update(AmsData& data) {
    unsigned long now = millis();
    uint16_t pos = (now / REALTIME_SAMPLE) % REALTIME_SIZE;
    if(lastMillis == 0) {
        lastMillis = now;
        lastReading = data.getActiveImportCounter() - data.getActiveExportCounter();
        lastPos = pos;
        return;
    }
    if(pos == lastPos && data.isCounterEstimated()) return;

    unsigned long ms = now - lastMillis;
    int32_t val; // A bit hacky this one, but just to avoid spikes at end of hour. Will mostly be correct
    if(data.isCounterEstimated()) {
        val = ((data.getActiveImportCounter() - data.getActiveExportCounter() - lastReading) * 1000) / (((float) ms) / 3600000.0);
    } else {
        val = data.getActiveImportPower() - data.getActiveExportPower();
    }
    uint8_t scale = 0;
    int32_t update = val / pow(10, scale);
    while(update > INT8_MAX || update < INT8_MIN) {
        update = val / pow(10, ++scale);
    }
    if(pos < lastPos) {
        for(uint16_t i = lastPos+1; i < REALTIME_SIZE; i++) {
            values[i] = update;
            scaling[i] = scale;
        }
        for(uint16_t i = 0; i <= pos; i++) {
            values[i] = update;
            scaling[i] = scale;
        }
    } else {
        for(uint16_t i = lastPos+1; i <= pos; i++) {
            values[i] = update;
            scaling[i] = scale;
        }
    }

    lastMillis = now;
    lastReading = data.getActiveImportCounter() - data.getActiveExportCounter();
    lastPos = pos;
}

int32_t RealtimePlot::getValue(uint16_t req) {
    if(req > REALTIME_SIZE) return 0;

    unsigned long now = millis();
    if(req * REALTIME_SAMPLE > now) return 0;
    unsigned long reqTime = now - (req * REALTIME_SAMPLE);

    uint16_t pos = (now / REALTIME_SAMPLE) % REALTIME_SIZE;
    uint16_t getPos;
    if(reqTime > lastMillis) {
        getPos = lastPos;
    } else if(req > pos) {
        getPos = REALTIME_SIZE + pos - req;
    } else {
        getPos = pos - req;
    }
    return values[getPos] * pow(10, scaling[getPos]);
}

int16_t RealtimePlot::getSize() {
    return REALTIME_SIZE;
}
