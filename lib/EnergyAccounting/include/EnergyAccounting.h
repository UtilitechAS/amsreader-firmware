#ifndef _ENERGYACCOUNTING_H
#define _ENERGYACCOUNTING_H

#include "Arduino.h"
#include "AmsData.h"
#include "AmsDataStorage.h"
#include "EntsoeApi.h"

struct EnergyAccountingPeak {
    uint8_t day;
    uint16_t value;
};

struct EnergyAccountingData {
    uint8_t version;
    uint8_t month;
    uint16_t costYesterday;
    uint16_t costThisMonth;
    uint16_t costLastMonth;
    uint16_t incomeYesterday;
    uint16_t incomeThisMonth;
    uint16_t incomeLastMonth;
    EnergyAccountingPeak peaks[5];
};

struct EnergyAccountingData4 {
    uint8_t version;
    uint8_t month;
    uint16_t costYesterday;
    uint16_t costThisMonth;
    uint16_t costLastMonth;
    EnergyAccountingPeak peaks[5];
};

struct EnergyAccountingData1 {
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
    void setup(AmsDataStorage *ds, EnergyAccountingConfig *config);
    void setEapi(EntsoeApi *eapi);
    void setTimezone(Timezone*);
    EnergyAccountingConfig* getConfig();
    bool update(AmsData* amsData);
    bool load();
    bool save();
    bool isInitialized();

    float getUseThisHour();
    float getUseToday();
    float getUseThisMonth();

    float getProducedThisHour();
    float getProducedToday();
    float getProducedThisMonth();

    float getCostThisHour();
    float getCostToday();
    float getCostYesterday();
    float getCostThisMonth();
    uint16_t getCostLastMonth();

    float getIncomeThisHour();
    float getIncomeToday();
    float getIncomeYesterday();
    float getIncomeThisMonth();
    uint16_t getIncomeLastMonth();

    float getMonthMax();
    uint8_t getCurrentThreshold();
    EnergyAccountingPeak getPeak(uint8_t);

    EnergyAccountingData getData();
    void setData(EnergyAccountingData&);

    void setFixedPrice(float price, String currency);
    float getPriceForHour(uint8_t h);

private:
    RemoteDebug* debugger = NULL;
    unsigned long lastUpdateMillis = 0;
    bool init = false, initPrice = false;
    AmsDataStorage *ds = NULL;
    EntsoeApi *eapi = NULL;
    EnergyAccountingConfig *config = NULL;
    Timezone *tz = NULL;
    uint8_t currentHour = 0, currentDay = 0, currentThresholdIdx = 0;
    float use = 0, costHour = 0, costDay = 0;
    float produce = 0, incomeHour = 0, incomeDay = 0;
    EnergyAccountingData data = { 0, 0, 0, 0, 0, 0 };
    float fixedPrice = 0;
    String currency = "";

    void calcDayCost();
    bool updateMax(uint16_t val, uint8_t day);
};

#endif
