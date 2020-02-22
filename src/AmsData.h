#ifndef _AMSDATA_H
#define _AMSDATA_H

#include "Arduino.h"
#include "HanReader.h"

#define METER_TYPE_KAIFA 1
#define METER_TYPE_AIDON 2
#define METER_TYPE_KAMSTRUP 3

class AmsData {
public:
    AmsData();
    AmsData(int meterType, HanReader& hanReader);

    void apply(AmsData& other);

    unsigned long getLastUpdateMillis();

    unsigned long getPackageTimestamp();

    int getListType();

    String getListId();
    String getMeterId();
    String getMeterType();

    unsigned long getMeterTimestamp();

    int getActiveImportPower();
    int getReactiveImportPower();
    int getActiveExportPower();
    int getReactiveExportPower();

    double getL1Voltage();
    double getL2Voltage();
    double getL3Voltage();

    double getL1Current();
    double getL2Current();
    double getL3Current();

    double getActiveImportCounter();
    double getReactiveImportCounter();
    double getActiveExportCounter();
    double getReactiveExportCounter();

    bool isThreePhase();

private:
    unsigned long lastUpdateMillis;
    int listType;
    unsigned long packageTimestamp;
    String listId, meterId, meterType;
    unsigned long meterTimestamp;
    int activeImportPower, reactiveImportPower, activeExportPower, reactiveExportPower;
    double l1voltage, l2voltage, l3voltage, l1current, l2current, l3current;
    double activeImportCounter, reactiveImportCounter, activeExportCounter, reactiveExportCounter;
    bool threePhase;

    void extractFromKaifa(HanReader& hanReader, int listSize);
    void extractFromAidon(HanReader& hanReader, int listSize);
    void extractFromKamstrup(HanReader& hanReader, int listSize);
};

#endif
