#ifndef _AMSDATA_H
#define _AMSDATA_H

#include "Arduino.h"
#include <Timezone.h>
#include "HanReader.h"

#define METER_TYPE_KAIFA 1
#define METER_TYPE_AIDON 2
#define METER_TYPE_KAMSTRUP 3
#define METER_TYPE_OMNIPOWER 4

class AmsData {
public:
    AmsData();
    AmsData(uint8_t meterType, bool substituteMissing, HanReader& hanReader);

    void apply(AmsData& other);

    unsigned long getLastUpdateMillis();

    time_t getPackageTimestamp();

    uint8_t getListType();

    String getListId();
    String getMeterId();
    String getMeterType();

    time_t getMeterTimestamp();

    uint16_t getActiveImportPower();
    uint16_t getReactiveImportPower();
    uint16_t getActiveExportPower();
    uint16_t getReactiveExportPower();

    float getL1Voltage();
    float getL2Voltage();
    float getL3Voltage();

    float getL1Current();
    float getL2Current();
    float getL3Current();

    float getActiveImportCounter();
    float getReactiveImportCounter();
    float getActiveExportCounter();
    float getReactiveExportCounter();

    bool isThreePhase();

private:
    unsigned long lastUpdateMillis = 0;
    uint8_t listType = 0;
    time_t packageTimestamp = 0;
    String listId, meterId, meterType;
    time_t meterTimestamp = 0;
    uint16_t activeImportPower = 0, reactiveImportPower = 0, activeExportPower = 0, reactiveExportPower = 0;
    float l1voltage = 0, l2voltage = 0, l3voltage = 0, l1current = 0, l2current = 0, l3current = 0;
    float activeImportCounter = 0, reactiveImportCounter = 0, activeExportCounter = 0, reactiveExportCounter = 0;
    bool threePhase = false, counterEstimated = false;

    void extractFromKaifa(HanReader& hanReader, uint8_t listSize);
    void extractFromAidon(HanReader& hanReader, uint8_t listSize, bool substituteMissing);
    void extractFromKamstrup(HanReader& hanReader, uint8_t listSize, bool substituteMissing);
    void extractFromOmnipower(HanReader& hanReader, uint8_t listSize);
};

#endif
