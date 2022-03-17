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
    void setup(AmsDataStorage *ds, EntsoeApi *eapi, EnergyAccountingConfig *config);
    void setTimezone(Timezone*);
    EnergyAccountingConfig* getConfig();
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
    uint8_t getCurrentThreshold();

    EnergyAccountingData getData();
    void setData(EnergyAccountingData&);

private:
    RemoteDebug* debugger = NULL;
    unsigned long lastUpdateMillis = 0;
    bool init = false, initPrice = false;
    AmsDataStorage *ds = NULL;
    EntsoeApi *eapi = NULL;
    EnergyAccountingConfig *config = NULL;
    Timezone *tz = NULL;
    uint8_t currentHour = 0, currentDay = 0, currentThresholdIdx = 0;
    double use, costHour, costDay;
    EnergyAccountingData data = { 0, 0, 0, 0, 0, 0 };

    bool load();
    bool calcDayUse();
    void calcDayCost();
};

#endif
