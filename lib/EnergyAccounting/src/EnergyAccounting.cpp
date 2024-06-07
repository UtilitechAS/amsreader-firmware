/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "EnergyAccounting.h"
#include "LittleFS.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"

#if defined(AMS_REMOTE_DEBUG)
EnergyAccounting::EnergyAccounting(RemoteDebug* debugger, EnergyAccountingRealtimeData* rtd) {
#else
EnergyAccounting::EnergyAccounting(Stream* Stream, EnergyAccountingRealtimeData* rtd) {
#endif
    data.version = 1;
    this->debugger = debugger;
    if(rtd->magic != 0x6A) {
        rtd->magic = 0x6A;
        rtd->currentHour = 0;
        rtd->currentDay = 0;
        rtd->currentThresholdIdx = 0;
        rtd->use = 0;
        rtd->costHour = 0;
        rtd->costDay = 0;
        rtd->produce = 0;
        rtd->incomeHour = 0;
        rtd->incomeDay = 0;
        rtd->lastImportUpdateMillis = 0;
        rtd->lastExportUpdateMillis = 0;
    }
    this->realtimeData = rtd;
}

void EnergyAccounting::setup(AmsDataStorage *ds, EnergyAccountingConfig *config) {
    this->ds = ds;
    this->config = config;
}

void EnergyAccounting::setPriceService(PriceService *ps) {
    this->ps = ps;
}

EnergyAccountingConfig* EnergyAccounting::getConfig() {
    return config;
}

void EnergyAccounting::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool EnergyAccounting::isInitialized() {
    return this->init;
}

bool EnergyAccounting::update(AmsData* amsData) {
    if(config == NULL) return false;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;
    if(tz == NULL) {
        return false;
    }

    bool ret = false;
    tmElements_t local;
    breakTime(tz->toLocal(now), local);

    if(!init) {
        this->realtimeData->lastImportUpdateMillis = 0;
        this->realtimeData->lastExportUpdateMillis = 0;
        this->realtimeData->currentHour = local.Hour;
        this->realtimeData->currentDay = local.Day;
        if(!load()) {
            data = { 6, local.Month, 
                0, 0, 0, // Cost
                0, 0, 0, // Income
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
        }
        init = true;
    }

    float importPrice = getPriceForHour(PRICE_DIRECTION_IMPORT, 0);
    if(!initPrice && importPrice != PRICE_NO_VALUE) {
        calcDayCost();
    }

    if(local.Hour != this->realtimeData->currentHour && (amsData->getListType() >= 3 || local.Minute == 1)) {
        tmElements_t oneHrAgo, oneHrAgoLocal;
        breakTime(now-3600, oneHrAgo);
        uint16_t val = round(ds->getHourImport(oneHrAgo.Hour) / 10.0);

        breakTime(tz->toLocal(now-3600), oneHrAgoLocal);
        ret |= updateMax(val, oneHrAgoLocal.Day);

        this->realtimeData->currentHour = local.Hour; // Need to be defined here so that day cost is correctly calculated
        if(local.Hour > 0) {
            calcDayCost();
        }

        this->realtimeData->use = 0;
        this->realtimeData->produce = 0;
        this->realtimeData->costHour = 0;
        this->realtimeData->incomeHour = 0;

        uint8_t prevDay = this->realtimeData->currentDay;
        if(local.Day != this->realtimeData->currentDay) {
            data.costYesterday = this->realtimeData->costDay * 100;
            data.costThisMonth += this->realtimeData->costDay * 100;
            this->realtimeData->costDay = 0;

            data.incomeYesterday = this->realtimeData->incomeDay * 100;
            data.incomeThisMonth += this->realtimeData->incomeDay * 100;
            this->realtimeData->incomeDay = 0;

            this->realtimeData->currentDay = local.Day;
            ret = true;
        }

        if(local.Month != data.month) {
            data.costLastMonth = data.costThisMonth;
            data.costThisMonth = 0;
            data.incomeLastMonth = data.incomeThisMonth;
            data.incomeThisMonth = 0;
            for(uint8_t i = 0; i < 5; i++) {
                data.peaks[i] = { 0, 0 };
            }

            uint64_t totalImport = 0, totalExport = 0;
            for(uint8_t i = 1; i <= prevDay; i++) {
                totalImport += ds->getDayImport(i);
                totalExport += ds->getDayExport(i);
            }
            uint8_t accuracy = 0;
            uint64_t importUpdate = totalImport, exportUpdate = totalExport;
            while(importUpdate > UINT32_MAX || exportUpdate > UINT32_MAX) {
                accuracy++;
                importUpdate = totalImport / pow(10, accuracy);
                exportUpdate = totalExport / pow(10, accuracy);
            }
            data.lastMonthImport = importUpdate;
            data.lastMonthExport = exportUpdate;
            data.lastMonthAccuracy = accuracy;

            data.month = local.Month;
            this->realtimeData->currentThresholdIdx = 0;
            ret = true;
        }
    }

    if(this->realtimeData->lastImportUpdateMillis < amsData->getLastUpdateMillis()) {
        unsigned long ms = amsData->getLastUpdateMillis() - this->realtimeData->lastImportUpdateMillis;
        float kwhi = (amsData->getActiveImportPower() * (((float) ms) / 3600000.0)) / 1000.0;
        if(kwhi > 0) {
            this->realtimeData->use += kwhi;
            if(importPrice != PRICE_NO_VALUE) {
                float cost = importPrice * kwhi;
                this->realtimeData->costHour += cost;
                this->realtimeData->costDay += cost;
            }
        }
        this->realtimeData->lastImportUpdateMillis = amsData->getLastUpdateMillis();
    }

    if(amsData->getListType() > 1 && this->realtimeData->lastExportUpdateMillis < amsData->getLastUpdateMillis()) {
        unsigned long ms = amsData->getLastUpdateMillis() - this->realtimeData->lastExportUpdateMillis;
        float kwhe = (amsData->getActiveExportPower() * (((float) ms) / 3600000.0)) / 1000.0;
        if(kwhe > 0) {
            this->realtimeData->produce += kwhe;
            float exportPrice = getPriceForHour(PRICE_DIRECTION_EXPORT, 0);
            if(exportPrice != PRICE_NO_VALUE) {
                float income = exportPrice * kwhe;
                this->realtimeData->incomeHour += income;
                this->realtimeData->incomeDay += income;
            }
        }
        this->realtimeData->lastExportUpdateMillis = amsData->getLastUpdateMillis();
    }

    if(config != NULL) {
        while(getMonthMax() > config->thresholds[this->realtimeData->currentThresholdIdx] && this->realtimeData->currentThresholdIdx < 10) this->realtimeData->currentThresholdIdx++;
    }

    return ret;
}

void EnergyAccounting::calcDayCost() {
    time_t now = time(nullptr);
    tmElements_t local, utc;
    if(tz == NULL) return;
    breakTime(tz->toLocal(now), local);

    if(getPriceForHour(PRICE_DIRECTION_IMPORT, 0) != PRICE_NO_VALUE) {
        if(initPrice) {
            this->realtimeData->costDay = 0;
            this->realtimeData->incomeDay = 0;
        }
        for(uint8_t i = 0; i < this->realtimeData->currentHour; i++) {
            breakTime(now - ((local.Hour - i) * 3600), utc);

            float priceIn = getPriceForHour(PRICE_DIRECTION_IMPORT, i - local.Hour);
            if(priceIn != PRICE_NO_VALUE) {
                int16_t wh = ds->getHourImport(utc.Hour);
                this->realtimeData->costDay += priceIn * (wh / 1000.0);
            }

            float priceOut = getPriceForHour(PRICE_DIRECTION_EXPORT, i - local.Hour);
            if(priceOut != PRICE_NO_VALUE) {
                int16_t wh = ds->getHourExport(utc.Hour);
                this->realtimeData->incomeDay += priceOut * (wh / 1000.0);
            }
        }
        initPrice = true;
    }
}

float EnergyAccounting::getUseThisHour() {
    return this->realtimeData->use;
}

float EnergyAccounting::getUseToday() {
    if(tz == NULL) return 0.0;
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < this->realtimeData->currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourImport(utc.Hour) / 1000.0;
    }
    return ret + getUseThisHour();
}

float EnergyAccounting::getUseThisMonth() {
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < this->realtimeData->currentDay; i++) {
        ret += ds->getDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}

float EnergyAccounting::getUseLastMonth() {
    return (data.lastMonthImport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getProducedThisHour() {
    return this->realtimeData->produce;
}

float EnergyAccounting::getProducedToday() {
    if(tz == NULL) return 0.0;
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < this->realtimeData->currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourExport(utc.Hour) / 1000.0;
    }
    return ret + getProducedThisHour();
}

float EnergyAccounting::getProducedThisMonth() {
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < this->realtimeData->currentDay; i++) {
        ret += ds->getDayExport(i) / 1000.0;
    }
    return ret + getProducedToday();
}

float EnergyAccounting::getProducedLastMonth() {
    return (data.lastMonthExport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getCostThisHour() {
    return this->realtimeData->costHour;
}

float EnergyAccounting::getCostToday() {
    return this->realtimeData->costDay;
}

float EnergyAccounting::getCostYesterday() {
    return data.costYesterday / 100.0;
}

float EnergyAccounting::getCostThisMonth() {
    return (data.costThisMonth / 100.0) + getCostToday();
}

float EnergyAccounting::getCostLastMonth() {
    return data.costLastMonth / 100.0;
}

float EnergyAccounting::getIncomeThisHour() {
    return this->realtimeData->incomeHour;
}

float EnergyAccounting::getIncomeToday() {
    return this->realtimeData->incomeDay;
}

float EnergyAccounting::getIncomeYesterday() {
    return data.incomeYesterday / 100.0;
}

float EnergyAccounting::getIncomeThisMonth() {
    return (data.incomeThisMonth / 100.0) + getIncomeToday();
}

float EnergyAccounting::getIncomeLastMonth() {
    return data.incomeLastMonth / 100.0;
}

uint8_t EnergyAccounting::getCurrentThreshold() {
    if(config == NULL)
        return 0;
    return config->thresholds[this->realtimeData->currentThresholdIdx];
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
        return false;
    }

    bool ret = false;
    if(LittleFS.exists(FILE_ENERGYACCOUNTING)) {
        File file = LittleFS.open(FILE_ENERGYACCOUNTING, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());

        if(buf[0] == 6) {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            memcpy(&this->data, data, sizeof(this->data));
            ret = true;
        } else if(buf[0] == 5) {
            EnergyAccountingData5* data = (EnergyAccountingData5*) buf;
            this->data = { 6, data->month, 
                ((uint32_t) data->costYesterday) * 10,
                ((uint32_t) data->costThisMonth) * 100,
                ((uint32_t) data->costLastMonth) * 100,
                ((uint32_t) data->incomeYesterday) * 10,
                ((uint32_t) data->incomeThisMonth) * 100,
                ((uint32_t) data->incomeLastMonth) * 100,
                0,0,0, // Last month import, export and accuracy
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        } else if(buf[0] == 4) {
            EnergyAccountingData4* data = (EnergyAccountingData4*) buf;
            this->data = { 5, data->month, 
                ((uint32_t) data->costYesterday) * 10,
                ((uint32_t) data->costThisMonth) * 100,
                ((uint32_t) data->costLastMonth) * 100,
                0,0,0, // Income from production
                0,0,0, // Last month import, export and accuracy
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
                data->costYesterday * 10,
                data->costThisMonth,
                data->costLastMonth,
                0,0,0, // Income from production
                0,0,0, // Last month import, export and accuracy
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
                0,0,0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
            if(buf[0] == 2) {
                EnergyAccountingData2* data = (EnergyAccountingData2*) buf;
                this->data.month = data->month;
                this->data.costYesterday = data->costYesterday * 10;
                this->data.costThisMonth = data->costThisMonth;
                this->data.costLastMonth = data->costLastMonth;
                uint8_t b = 0;
                for(uint8_t i = sizeof(this->data); i < file.size(); i+=2) {
                    this->data.peaks[b].day = b;
                    memcpy(&this->data.peaks[b].value, buf+i, 2);
                    b++;
                    if(b >= config->hours || b >= 5) break;
                }
                ret = true;
            } else {
                ret = false;
            }
        }

        file.close();
    }

    return ret;
}

bool EnergyAccounting::save() {
    if(!LittleFS.begin()) {
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
                data.peaks[i].day = day;
                data.peaks[i].value = val;
                return true;
            }
            return false;
        }
    }
    uint16_t test = val;
    uint8_t idx = 255;
    for(uint8_t i = 0; i < 5; i++) {
        if(val > data.peaks[i].value) {
            if(test > data.peaks[i].value) {
                test = data.peaks[i].value;
                idx = i;
            }
        }
    }
    if(idx < 5) {
        data.peaks[idx].value = val;
        data.peaks[idx].day = day;
        return true;
    }
    return false;
}

void EnergyAccounting::setCurrency(String currency) {
    this->currency = currency;
}

float EnergyAccounting::getPriceForHour(uint8_t d, uint8_t h) {
    if(ps == NULL) return PRICE_NO_VALUE;
    return ps->getValueForHour(d, h);
}