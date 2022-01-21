#ifndef _ENERGYACCOUNTING_H
#define _ENERGYACCOUNTING_H

#include "Arduino.h"
#include "AmsData.h"
#include "AmsDataStorage.h"
#include "entsoe/EntsoeApi.h"

struct EnergyAccountingData {
    uint8_t version;
    uint8_t month;
    uint16_t maxHour;
    uint16_t costYesterday;
    uint16_t costThisMonth;
    uint16_t costLastMonth;
};

class EnergyAccounting {
public:
    EnergyAccounting(RemoteDebug*);
    void setup(AmsDataStorage *ds, EntsoeApi *eapi);
    bool update(AmsData* amsData);
    bool save();

    double getUseThisHour();
    double getCostThisHour();
    double getUseToday();
    double getCostToday();
    double getCostYesterday();
    double getUseThisMonth();
    double getCostThisMonth();
    double getCostLastMonth();

    float getMonthMax();
    float getCurrentThreshold();

private:
    RemoteDebug* debugger = NULL;
    unsigned long lastUpdateMillis = 0;
    bool init = false, initPrice = false;
    AmsDataStorage *ds = NULL;
    EntsoeApi *eapi = NULL;
    uint8_t thresholds[5] = {50, 100, 150, 200, 250};
    uint8_t currentHour = 0, currentDay = 0, currentThresholdIdx = 0;
    double use, costHour, costDay;
    EnergyAccountingData data;

    bool load();
};

#endif
