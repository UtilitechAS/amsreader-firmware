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
    int32_t costYesterday;
    int32_t costThisMonth;
    int32_t costLastMonth;
    int32_t incomeYesterday;
    int32_t incomeThisMonth;
    int32_t incomeLastMonth;
    uint32_t lastMonthImport;
    uint32_t lastMonthExport;
    uint8_t lastMonthAccuracy;
    EnergyAccountingPeak peaks[5];
};

struct EnergyAccountingData5 {
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

struct EnergyAccountingData2 {
    uint8_t version;
    uint8_t month;
    uint16_t maxHour;
    uint16_t costYesterday;
    uint16_t costThisMonth;
    uint16_t costLastMonth;
};

struct EnergyAccountingRealtimeData {
    uint8_t magic;
    uint8_t currentHour;
    uint8_t currentDay;
    uint8_t currentThresholdIdx;
    float use;
    float costHour;
    float costDay;
    float produce;
    float incomeHour;
    float incomeDay;
    unsigned long lastImportUpdateMillis;
    unsigned long lastExportUpdateMillis;
};


class EnergyAccounting {
public:
    EnergyAccounting(RemoteDebug*, EnergyAccountingRealtimeData*);
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
    float getUseLastMonth();

    float getProducedThisHour();
    float getProducedToday();
    float getProducedThisMonth();
    float getProducedLastMonth();

    float getCostThisHour();
    float getCostToday();
    float getCostYesterday();
    float getCostThisMonth();
    float getCostLastMonth();

    float getIncomeThisHour();
    float getIncomeToday();
    float getIncomeYesterday();
    float getIncomeThisMonth();
    float getIncomeLastMonth();

    float getMonthMax();
    uint8_t getCurrentThreshold();
    EnergyAccountingPeak getPeak(uint8_t);

    EnergyAccountingData getData();
    void setData(EnergyAccountingData&);

    void setFixedPrice(float price, String currency);
    float getPriceForHour(uint8_t h);

private:
    RemoteDebug* debugger = NULL;
    bool init = false, initPrice = false;
    AmsDataStorage *ds = NULL;
    EntsoeApi *eapi = NULL;
    EnergyAccountingConfig *config = NULL;
    Timezone *tz = NULL;
    EnergyAccountingData data = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    EnergyAccountingRealtimeData* realtimeData = NULL;
    float fixedPrice = 0;
    String currency = "";

    void calcDayCost();
    bool updateMax(uint16_t val, uint8_t day);
};

#endif
