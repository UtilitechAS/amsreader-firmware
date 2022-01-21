#include "EnergyAccounting.h"

EnergyAccounting::EnergyAccounting(RemoteDebug* debugger) {
    this->debugger = debugger;
}

void EnergyAccounting::setup(AmsDataStorage *ds, EntsoeApi *eapi) {
    this->ds = ds;
    this->eapi = eapi;
}

bool EnergyAccounting::update(AmsData* amsData) {
    time_t now = time(nullptr);
    if(now < EPOCH_2021_01_01) return false;

    bool ret = false;
    tmElements_t tm;
    breakTime(now, tm);

    if(!init) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Initializing data at %lu\n", now);
        if(!load()) {
            data = { 1, tm.Month, 0, 0, 0, 0 };
            currentHour = tm.Hour;
            currentDay = tm.Day;

            for(int i = 0; i < tm.Hour; i++) {
                int16_t val = ds->getHour(i) / 10.0;
                if(val > data.maxHour) {
                    data.maxHour = val;
                    ret = true;
                }
            }
        }
        init = true;
    }

    if(!initPrice && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Initializing prices at %lu\n", now);
        for(int i = 0; i < tm.Hour; i++) {
            float price = eapi->getValueForHour(i-tm.Hour);
            if(price == ENTSOE_NO_VALUE) break;
            int16_t wh = ds->getHour(i);
            double kwh = wh / 1000.0;
            costDay += price * kwh;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("  Hour: %d, wh: %d, kwh; %.2f, price: %.2f, costDay: %.4f\n", i, wh, kwh, price, costDay);
        }
        initPrice = true;
    }

    if(amsData->getListType() >= 3 && tm.Hour != currentHour) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New hour %d\n", tm.Hour);
        if(tm.Hour > 0) {
            if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
                costDay = 0;
                for(int i = 0; i < tm.Hour; i++) {
                    float price = eapi->getValueForHour(i-tm.Hour);
                    if(price == ENTSOE_NO_VALUE) break;
                    int16_t wh = ds->getHour(i);
                    costDay += price * (wh / 1000.0);
                }
            }
        }

        for(int i = 0; i < tm.Hour; i++) {
            int16_t val = ds->getHour(i) / 10.0;
            if(val > data.maxHour) {
                data.maxHour = val;
                ret = true;
            }
        }

        use = 0;
        costHour = 0;
        currentHour = tm.Hour;

        if(tm.Day != currentDay) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New day %d\n", tm.Day);
            data.costYesterday = costDay * 100;
            data.costThisMonth += costDay * 100;
            costDay = 0;
            currentDay = tm.Day;
            ret = true;
        }

        if(tm.Month != data.month) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New month %d\n", tm.Month);
            data.costLastMonth = data.costThisMonth;
            data.costThisMonth = 0;
            data.maxHour = 0;
            data.month = tm.Month;
            currentThresholdIdx = 0;
            ret = true;
        }
    }

    unsigned long ms = this->lastUpdateMillis > amsData->getLastUpdateMillis() ? 0 : amsData->getLastUpdateMillis() - this->lastUpdateMillis;
    float kwh = (amsData->getActiveImportPower() * (((float) ms) / 3600000.0)) / 1000.0;
    lastUpdateMillis = amsData->getLastUpdateMillis();
    if(kwh > 0) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Adding %.4f kWh\n", kwh);
        use += kwh;
        if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
            float price = eapi->getValueForHour(0);
            float cost = price * kwh;
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting)  and %.4f %s\n", cost / 100.0, eapi->getCurrency());
            costHour += cost;
            costDay += cost;
        }
    }

    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  calculating threshold, currently at %d\n", currentThresholdIdx);
    while(getMonthMax() > thresholds[currentThresholdIdx] / 10.0 && currentThresholdIdx < 5) currentThresholdIdx++;
    while(use > thresholds[currentThresholdIdx] / 10.0 && currentThresholdIdx < 5) currentThresholdIdx++;
    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  new threshold %d\n", currentThresholdIdx);

    return ret;
}

double EnergyAccounting::getUseThisHour() {
    return use;
}

double EnergyAccounting::getCostThisHour() {
    return costHour;
}

double EnergyAccounting::getUseToday() {
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < EPOCH_2021_01_01) return 0;
    tmElements_t tm;
    breakTime(now, tm);
    for(int i = 0; i < tm.Hour; i++) {
        ret += ds->getHour(i) / 1000.0;
    }
    return ret + getUseThisHour();
}

double EnergyAccounting::getCostToday() {
    return costDay;
}

double EnergyAccounting::getCostYesterday() {
    return data.costYesterday / 100.0;
}

double EnergyAccounting::getUseThisMonth() {
    time_t now = time(nullptr);
    if(now < EPOCH_2021_01_01) return 0;
    tmElements_t tm;
    breakTime(now, tm);
    float ret = 0;
    for(int i = 0; i < tm.Day; i++) {
        ret += ds->getDay(i) / 1000.0;
    }
    return ret + getUseToday();
}

double EnergyAccounting::getCostThisMonth() {
    return (data.costThisMonth / 100.0) + getCostToday();
}

double EnergyAccounting::getCostLastMonth() {
    return data.costLastMonth / 100.0;
}

float EnergyAccounting::getCurrentThreshold() {
    return thresholds[currentThresholdIdx] / 10.0;
}

float EnergyAccounting::getMonthMax() {
    return data.maxHour / 100.0;
}

bool EnergyAccounting::load() {
    return false; // TODO
}

bool EnergyAccounting::save() {
    return false; // TODO
}
