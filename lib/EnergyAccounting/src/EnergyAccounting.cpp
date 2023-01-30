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
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Initializing data at %lu\n", (int32_t) now);
        if(!load()) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Unable to load existing data\n");
            data = { 5, local.Month, 
                0, 0, 0, // Cost
                0, 0, 0, // Income
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
        } else if(debugger->isActive(RemoteDebug::DEBUG)) {
            for(uint8_t i = 0; i < 5; i++) {
                debugger->printf("(EnergyAccounting) Peak hour from day %d: %d\n", data.peaks[i].day, data.peaks[i].value*10);
            }
            debugger->printf("(EnergyAccounting) Loaded cost yesterday: %.2f, this month: %d, last month: %d\n", data.costYesterday / 10.0, data.costThisMonth, data.costLastMonth);
            debugger->printf("(EnergyAccounting) Loaded income yesterday: %.2f, this month: %d, last month: %d\n", data.incomeYesterday / 10.0, data.incomeThisMonth, data.incomeLastMonth);
        }
        init = true;
    }

    if(!initPrice && eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Initializing prices at %lu\n", (int32_t) now);
        calcDayCost();
    }

    if(local.Hour != currentHour && (amsData->getListType() >= 3 || local.Minute == 1)) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New local hour %d\n", local.Hour);

        tmElements_t oneHrAgo, oneHrAgoLocal;
        breakTime(now-3600, oneHrAgo);
        uint16_t val = ds->getHourImport(oneHrAgo.Hour) / 10;

        breakTime(tz->toLocal(now-3600), oneHrAgoLocal);
        ret |= updateMax(val, oneHrAgoLocal.Day);

        currentHour = local.Hour; // Need to be defined here so that day cost is correctly calculated
        if(local.Hour > 0) {
            calcDayCost();
        }

        use = 0;
        produce = 0;
        costHour = 0;
        incomeHour = 0;

        if(local.Day != currentDay) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New day %d\n", local.Day);
            data.costYesterday = costDay * 10;
            data.costThisMonth += costDay;
            costDay = 0;

            data.incomeYesterday = incomeDay * 10;
            data.incomeThisMonth += incomeDay;
            incomeDay = 0;

            currentDay = local.Day;
            ret = true;
        }

        if(local.Month != data.month) {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) New month %d\n", local.Month);
            data.costLastMonth = data.costThisMonth;
            data.costThisMonth = 0;
            data.incomeLastMonth = data.incomeThisMonth;
            data.incomeThisMonth = 0;
            for(uint8_t i = 0; i < 5; i++) {
                data.peaks[i] = { 0, 0 };
            }
            data.month = local.Month;
            currentThresholdIdx = 0;
            ret = true;
        }
    }

    unsigned long ms = this->lastUpdateMillis > amsData->getLastUpdateMillis() ? 0 : amsData->getLastUpdateMillis() - this->lastUpdateMillis;
    float kwhi = (amsData->getActiveImportPower() * (((float) ms) / 3600000.0)) / 1000.0;
    float kwhe = (amsData->getActiveExportPower() * (((float) ms) / 3600000.0)) / 1000.0;
    lastUpdateMillis = amsData->getLastUpdateMillis();
    if(kwhi > 0) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Adding %.4f kWh import\n", kwhi);
        use += kwhi;
        if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
            float price = eapi->getValueForHour(0);
            float cost = price * kwhi;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  and %.4f %s\n", cost / 100.0, eapi->getCurrency());
            costHour += cost;
            costDay += cost;
        }
    }
    if(kwhe > 0) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Adding %.4f kWh export\n", kwhe);
        produce += kwhe;
        if(eapi != NULL && eapi->getValueForHour(0) != ENTSOE_NO_VALUE) {
            float price = eapi->getValueForHour(0);
            float income = price * kwhe;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting)  and %.4f %s\n", income / 100.0, eapi->getCurrency());
            incomeHour += income;
            incomeDay += income;
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
        if(initPrice) {
            costDay = 0;
            incomeDay = 0;
        }
        for(int i = 0; i < currentHour; i++) {
            float price = eapi->getValueForHour(i - local.Hour);
            if(price == ENTSOE_NO_VALUE) break;
            breakTime(now - ((local.Hour - i) * 3600), utc);
            int16_t wh = ds->getHourImport(utc.Hour);
            costDay += price * (wh / 1000.0);

            wh = ds->getHourExport(utc.Hour);
            incomeDay += price * (wh / 1000.0);
        }
        initPrice = true;
    }
}

double EnergyAccounting::getUseThisHour() {
    return use;
}

double EnergyAccounting::getUseToday() {
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return 0.0;
    if(tz == NULL) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(int i = 0; i < currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourImport(utc.Hour) / 1000.0;
    }
    return ret + getUseThisHour();
}

double EnergyAccounting::getUseThisMonth() {
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return 0.0;
    float ret = 0;
    for(int i = 0; i < currentDay; i++) {
        ret += ds->getDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}

double EnergyAccounting::getProducedThisHour() {
    return produce;
}

double EnergyAccounting::getProducedToday() {
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return 0.0;
    tmElements_t utc;
    for(int i = 0; i < currentHour; i++) {
        breakTime(now - ((currentHour - i) * 3600), utc);
        ret += ds->getHourExport(utc.Hour) / 1000.0;
    }
    return ret + getProducedThisHour();
}

double EnergyAccounting::getProducedThisMonth() {
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return 0.0;
    float ret = 0;
    for(int i = 0; i < currentDay; i++) {
        ret += ds->getDayExport(i) / 1000.0;
    }
    return ret + getProducedToday();
}


double EnergyAccounting::getCostThisHour() {
    return costHour;
}

double EnergyAccounting::getCostToday() {
    return costDay;
}

double EnergyAccounting::getCostYesterday() {
    return data.costYesterday / 10.0;
}

double EnergyAccounting::getCostThisMonth() {
    return data.costThisMonth + getCostToday();
}

uint16_t EnergyAccounting::getCostLastMonth() {
    return data.costLastMonth;
}

double EnergyAccounting::getIncomeThisHour() {
    return incomeHour;
}

double EnergyAccounting::getIncomeToday() {
    return incomeDay;
}

double EnergyAccounting::getIncomeYesterday() {
    return data.incomeYesterday / 10.0;
}

double EnergyAccounting::getIncomeThisMonth() {
    return data.incomeThisMonth + getIncomeToday();
}

uint16_t EnergyAccounting::getIncomeLastMonth() {
    return data.incomeLastMonth;
}

uint8_t EnergyAccounting::getCurrentThreshold() {
    if(config == NULL)
        return 0;
    return config->thresholds[currentThresholdIdx];
}

float EnergyAccounting::getMonthMax() {
    if(config == NULL)
        return 0.0;
    uint8_t count = 0;
    uint32_t maxHour = 0.0;
    bool included[5] = { false, false, false, false, false };

    for(uint8_t x = 0;x < min((uint8_t) 5, config->hours); x++) {
        uint8_t maxIdx = 0;
        uint16_t maxVal = 0;
        for(uint8_t i = 0; i < 5; i++) {
            if(included[i]) continue;
            if(data.peaks[i].day == 0) continue;
            if(data.peaks[i].value > maxVal) {
                maxVal = data.peaks[i].value;
                maxIdx = i;
            }
        }
        if(maxVal > 0) {
            included[maxIdx] = true;
            count++;
        }
    }

    for(uint8_t i = 0; i < 5; i++) {
        if(!included[i]) continue;
        maxHour += data.peaks[i].value;
    }
    return maxHour > 0 ? maxHour / count / 100.0 : 0.0;
}

EnergyAccountingPeak EnergyAccounting::getPeak(uint8_t num) {
    if(config == NULL)
        return EnergyAccountingPeak({0,0});
    if(num < 1 || num > 5) return EnergyAccountingPeak({0,0});

    uint8_t count = 0;
    bool included[5] = { false, false, false, false, false };

    for(uint8_t x = 0;x < min((uint8_t) 5, config->hours); x++) {
        uint8_t maxIdx = 0;
        uint16_t maxVal = 0;
        for(uint8_t i = 0; i < 5; i++) {
            if(included[i]) continue;
            if(data.peaks[i].value > maxVal) {
                maxVal = data.peaks[i].value;
                maxIdx = i;
            }
        }
        if(maxVal > 0) {
            included[maxIdx] = true;
            count++;
        }
    }

    uint8_t pos = 0;
    for(uint8_t i = 0; i < 5; i++) {
        if(!included[i]) continue;
        pos++;
        if(pos == num) {
            return data.peaks[i];
        }
    }
    return EnergyAccountingPeak({0,0});
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

        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EnergyAccounting) Data version %d\n", buf[0]);
        if(buf[0] == 5) {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            memcpy(&this->data, data, sizeof(this->data));
            ret = true;
        } else if(buf[0] == 4) {
            EnergyAccountingData4* data = (EnergyAccountingData4*) buf;
            this->data = { 5, data->month, 
                data->costYesterday,
                data->costThisMonth,
                data->costLastMonth,
                0,0,0, // Income from production
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        } else if(buf[0] == 3) {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            this->data = { 5, data->month, 
                (uint16_t) (data->costYesterday / 10), (uint16_t) (data->costThisMonth / 100), (uint16_t) (data->costLastMonth / 100),
                0,0,0, // Income from production
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        } else {
            data = { 5, 0, 
                0, 0, 0, // Cost
                0,0,0, // Income from production
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
            if(buf[0] == 2) {
                EnergyAccountingData1* data = (EnergyAccountingData1*) buf;
                this->data.month = data->month;
                this->data.costYesterday = (uint16_t) (data->costYesterday / 10);
                this->data.costThisMonth = (uint16_t) (data->costThisMonth / 100);
                this->data.costLastMonth = (uint16_t) (data->costLastMonth / 100);
                uint8_t b = 0;
                for(uint8_t i = sizeof(this->data); i < file.size(); i+=2) {
                    this->data.peaks[b].day = b;
                    memcpy(&this->data.peaks[b].value, buf+i, 2);
                    b++;
                    if(b >= config->hours || b >= 5) break;
                }
                ret = true;
            } else if(buf[0] == 1) {
                EnergyAccountingData1* data = (EnergyAccountingData1*) buf;
                this->data.month = data->month;
                this->data.costYesterday = (uint16_t) (data->costYesterday / 10);
                this->data.costThisMonth = (uint16_t) (data->costThisMonth / 100);
                this->data.costLastMonth = (uint16_t) (data->costLastMonth / 100);
                this->data.peaks[0].day = 1;
                this->data.peaks[0].value = data->maxHour;
                ret = true;
            } else {
                if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf("(EnergyAccounting) Unknown version\n");
                ret = false;
            }
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
        char buf[sizeof(data)];
        memcpy(buf, &data, sizeof(data));
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

bool EnergyAccounting::updateMax(uint16_t val, uint8_t day) {
    for(uint8_t i = 0; i < 5; i++) {
        if(data.peaks[i].day == day || data.peaks[i].day == 0) {
            if(val > data.peaks[i].value) {
                if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Adding new max %d for day %d which is larger than %d\n", val*10, day, data.peaks[i].value*10);
                data.peaks[i].day = day;
                data.peaks[i].value = val;
                return true;
            }
            return false;
        }
    }
    uint16_t test = 0;
    uint8_t idx = 255;
    for(uint8_t i = 0; i < 5; i++) {
        if(val > data.peaks[i].value) {
            if(test < data.peaks[i].value) {
                test = data.peaks[i].value;
                idx = i;
            }
        }
    }
    if(idx < 5) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EnergyAccounting) Adding new max %d for day %d\n", val*10, day);
        data.peaks[idx].value = val;
        data.peaks[idx].day = day;
        return true;
    }
    return false;
}