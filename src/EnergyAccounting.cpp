#include "EnergyAccounting.h"
#include "LittleFS.h"
#include "AmsStorage.h"
#include "version.h"

EnergyAccounting::EnergyAccounting(RemoteDebug* debugger) {
    data.version = 1;
    this->debugger = debugger;
}

void EnergyAccounting::setup(AmsDataStorage *ds, EntsoeApi *eapi, EnergyAccountingConfig *config) {
    this->ds = ds;
    this->eapi = eapi;
    this->config = config;
    this->currentThresholdIdx = 0;
}

EnergyAccountingConfig* EnergyAccounting::getConfig() {
    return config;
}

void EnergyAccounting::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool EnergyAccounting::update(AmsData* amsData) {
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return false;
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Timezone is missing\n");
        return false;
    }

    bool ret = false;
    tmElements_t local;
    breakTime(tz->toLocal(now), local);

    if(!init) {
        currentHour = local.Hour;
        currentDay = local.Day;
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Initializing data at %lld\n", (int64_t) now);
        if(!load()) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Unable to load existing data");
            data = { 1, local.Month, 0, 0, 0, 0 };
            if(calcDayUse()) ret = true;
        }
        init = true;
    }

    if(!initPrice && eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Initializing prices at %lld\n", (int64_t) now);
        calcDayCost();
    }

    if(amsData->getListType() >= 3 && local.Hour != currentHour) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New local hour %d\n", local.Hour);

        if(calcDayUse()) ret = true;
        if(local.Hour > 0) {
            calcDayCost();
        }

        use = 0;
        costHour = 0;
        currentHour = local.Hour;

        if(local.Day != currentDay) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New day %d\n", local.Day);
            data.costYesterday = costDay * 100;
            data.costThisMonth += costDay * 100;
            costDay = 0;
            currentDay = local.Day;
            ret = true;
        }

        if(local.Month != data.month) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New month %d\n", local.Month);
            data.costLastMonth = data.costThisMonth;
            data.costThisMonth = 0;
            data.maxHour = 0;
            data.month = local.Month;
            currentThresholdIdx = 0;
            ret = true;
        }
    }

    unsigned long ms = this->lastUpdateMillis > amsData->getLastUpdateMillis() ? 0 : amsData->getLastUpdateMillis() - this->lastUpdateMillis;
    float kwh = (amsData->getActiveImportPower() * (((float) ms) / 3600000.0)) / 1000.0;
    lastUpdateMillis = amsData->getLastUpdateMillis();
    if(kwh > 0) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Adding %.4f kWh\n", kwh);
        use += kwh;
        if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
            float price = eapi->getValueForHour(0);
            float cost = price * kwh;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  and %.4f %s\n", cost / 100.0, eapi->getCurrency());
            costHour += cost;
            costDay += cost;
        }
    }

    if(config != NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  calculating threshold, currently at %d\n", currentThresholdIdx);
        while(getMonthMax() > config->thresholds[currentThresholdIdx] && currentThresholdIdx < 10) currentThresholdIdx++;
        while(use > config->thresholds[currentThresholdIdx] && currentThresholdIdx < 10) currentThresholdIdx++;
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  new threshold %d\n", currentThresholdIdx);
    }

    if(use > data.maxHour) {
        data.maxHour = use; // Not really a good idea to use calculated value, but when you pass midnight and have the highest use at hour 23, it will not be included through 'calcDayUse'
    }

    return ret;
}

bool EnergyAccounting::calcDayUse() {
    time_t now = time(nullptr);
    tmElements_t local, utc;
    breakTime(tz->toLocal(now), local);

    bool ret = false;
    uint8_t lim = local.Day == 1 ? local.Hour : 24;
    for(int i = 0; i < lim; i++) {
        breakTime(now - ((lim - i) * 3600), utc);
        int16_t val = ds->getHourImport(utc.Hour) / 10.0;
        if(val > data.maxHour) {
            data.maxHour = val;
            ret = true;
        }
    }
    return ret;
}

void EnergyAccounting::calcDayCost() {
    time_t now = time(nullptr);
    tmElements_t local, utc;
    breakTime(tz->toLocal(now), local);

    if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
        if(initPrice) costDay = 0;
        for(int i = 0; i < local.Hour; i++) {
            float price = eapi->getValueForHour(i - local.Hour);
            if(price == ENTSOE_NO_VALUE) break;
            breakTime(now - ((local.Hour - i) * 3600), utc);
            int16_t wh = ds->getHourImport(utc.Hour);
            costDay += price * (wh / 1000.0);
        }
        initPrice = true;
    }
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
    if(now < BUILD_EPOCH) return 0;
    tmElements_t local, utc;
    breakTime(tz->toLocal(now), local);
    for(int i = 0; i < local.Hour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourImport(utc.Hour) / 1000.0;
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
    if(now < BUILD_EPOCH) return 0;
    tmElements_t tm;
    if(tz != NULL)
        breakTime(tz->toLocal(now), tm);
    else
        breakTime(now, tm);
    float ret = 0;
    for(int i = 0; i < tm.Day; i++) {
        ret += ds->getDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}

double EnergyAccounting::getCostThisMonth() {
    return (data.costThisMonth / 100.0) + getCostToday();
}

double EnergyAccounting::getCostLastMonth() {
    return data.costLastMonth / 100.0;
}

uint8_t EnergyAccounting::getCurrentThreshold() {
    if(config == NULL)
        return 0;
    return config->thresholds[currentThresholdIdx];
}

float EnergyAccounting::getMonthMax() {
    return data.maxHour / 100.0;
}

bool EnergyAccounting::load() {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf("(EnergyAccounting) Unable to load LittleFS\n");
        }
        return false;
    }

    bool ret = false;
    if(LittleFS.exists(FILE_ENERGYACCOUNTING)) {
        File file = LittleFS.open(FILE_ENERGYACCOUNTING, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        EnergyAccountingData* data = (EnergyAccountingData*) buf;
        file.close();

        if(data->version == 1) {
            memcpy(&this->data, data, sizeof(this->data));
            ret = true;
        } else {
            ret = false;
        }
    }

    LittleFS.end();

    return ret;
}

bool EnergyAccounting::save() {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf("(EnergyAccounting) Unable to load LittleFS\n");
        }
        return false;
    }
    {
        File file = LittleFS.open(FILE_ENERGYACCOUNTING, "w");
        char buf[sizeof(data)];
        memcpy(buf, &data, sizeof(data));
        for(unsigned long i = 0; i < sizeof(data); i++) {
            file.write(buf[i]);
        }
        file.close();
    }

    LittleFS.end();
    return true;
}

EnergyAccountingData EnergyAccounting::getData() {
    return this->data;
}

void EnergyAccounting::setData(EnergyAccountingData& data) {
    this->data = data;
}
