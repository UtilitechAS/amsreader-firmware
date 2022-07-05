#include "EnergyAccounting.h"
#include "LittleFS.h"
#include "AmsStorage.h"
#include "version.h"

EnergyAccounting::EnergyAccounting(RemoteDebug* debugger) {
    data.version = 1;
    this->debugger = debugger;
}

void EnergyAccounting::setup(AmsDataStorage *ds, EnergyAccountingConfig *config) {
    this->ds = ds;
    this->config = config;
    this->currentThresholdIdx = 0;
    uint16_t *maxHours = new uint16_t[config->hours];
    for(uint8_t i = 0; i < config->hours; i++) {
        maxHours[i] = 0;
    }
    if(this->maxHours != NULL) {
        for(uint8_t i = 0; i < sizeof(this->maxHours)/2 && i < config->hours; i++) {
            maxHours[i] = this->maxHours[i];
        }
        delete(this->maxHours);
    }
    this->maxHours = maxHours;
}

void EnergyAccounting::setEapi(EntsoeApi *eapi) {
    this->eapi = eapi;
}

EnergyAccountingConfig* EnergyAccounting::getConfig() {
    return config;
}

void EnergyAccounting::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool EnergyAccounting::update(AmsData* amsData) {
    if(config == NULL) return false;
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
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Initializing data at %lld\n", (int64_t) now);
        if(!load()) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Unable to load existing data\n");
            data = { 2, local.Month, 0, 0, 0, 0 };
            for(uint8_t i = 0; i < config->hours; i++) {
                maxHours[i] = 0;
                break;
            }
        } else if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(EnergyAccounting) Loaded max calculated from %d hours with highest consumption\n", config->hours);
            for(uint8_t i = 0; i < config->hours; i++) {
                debugger->printf("(EnergyAccounting)  hour %d: %d\n", i+1, maxHours[i]*10);
            }
            debugger->printf("(EnergyAccounting) Loaded cost yesterday: %d, this month: %d, last month: %d\n", data.costYesterday, data.costThisMonth, data.costLastMonth);
        }
        init = true;
    }

    if(!initPrice && eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Initializing prices at %lld\n", (int64_t) now);
        calcDayCost();
    }

    if(local.Hour != currentHour && (amsData->getListType() >= 3 || local.Minute == 1)) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New local hour %d\n", local.Hour);

        tmElements_t oneHrAgo;
        breakTime(now-3600, oneHrAgo);
        uint32_t val = ds->getHourImport(oneHrAgo.Hour) / 10;
        for(uint8_t i = 0; i < config->hours; i++) {
            if(val > maxHours[i]) {
                if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Adding new max (%d) which is larger than %d\n", val*10, maxHours[i]*10);
                maxHours[i] = val;
                ret = true;
                break;
            }
        }
        if(debugger->isActive(RemoteDebug::INFO)) {
            debugger->printf("(EnergyAccounting) Current max calculated from %d hours with highest consumption\n", config->hours);
            for(uint8_t i = 0; i < config->hours; i++) {
                debugger->printf("(EnergyAccounting)  hour %d: %d\n", i+1, maxHours[i]*10);
            }
        }

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
            for(uint8_t i = 0; i < config->hours; i++) {
                maxHours[i] = 0;
            }
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
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  new threshold %d\n", currentThresholdIdx);
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
    uint8_t count = 0;
    uint32_t maxHour = 0.0;
    for(uint8_t i = 0; i < config->hours; i++) {
        if(maxHours[i] > 0) {
            maxHour += maxHours[i];
            count++;
        }
    }
    return maxHour > 0 ? maxHour / count / 100.0 : 0.0;
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

        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Data version %d\n", data->version);
        if(data->version == 2) {
            memcpy(&this->data, data, sizeof(this->data));
            uint8_t b = 0;
            for(uint8_t i = sizeof(this->data); i < file.size(); i+=2) {
                memcpy(&this->maxHours[b++], buf+i, 2);
                if(b > config->hours) break;
            }
            ret = true;
        } else if(data->version == 1) {
            memcpy(&this->data, data, sizeof(this->data));
            for(uint8_t i = 0; i < config->hours; i++) {
                maxHours[i] = data->unused;
            }
            data->version = 2;
            ret = true;
        } else {
            if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf("(EnergyAccounting) Unknown version\n");
            ret = false;
        }

        file.close();
    } else {
        if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf("(EnergyAccounting) File not found\n");
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
        char buf[sizeof(data)+sizeof(this->maxHours)];
        memcpy(buf, &data, sizeof(data));
        for(uint8_t i = 0; i < config->hours; i++) {
            memcpy(buf+sizeof(data)+(i*2), &this->maxHours[i], 2);
        }
        for(uint8_t i = 0; i < sizeof(buf); i++) {
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

uint16_t * EnergyAccounting::getMaxHours() {
    return maxHours;
}

void EnergyAccounting::setMaxHours(uint16_t * maxHours) {
    for(uint8_t i = 0; i < config->hours; i++) {
        this->maxHours[i] = maxHours[i];
    }
}
