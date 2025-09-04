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
    realtimeData = rtd;
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
        realtimeData->lastImportUpdateMillis = 0;
        realtimeData->lastExportUpdateMillis = 0;
        realtimeData->currentHour = local.Hour;
        realtimeData->currentDay = local.Day;
        if(!load()) {
            data = { 7, local.Month, 
                0, 0, 0, 0, // Cost
                0, 0, 0, 0, // Income
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0, // Peak 5
                0 // Last updated
            };
        }
        init = true;
    }

    if(!initPrice && ps != NULL && ps->hasPrice()) {
        calcDayCost();
    }

    if(local.Hour != realtimeData->currentHour && (amsData->getListType() >= 3 || local.Minute == 1)) {
        tmElements_t oneHrAgo, oneHrAgoLocal;
        breakTime(now-3600, oneHrAgo);
        uint16_t val = round(ds->getHourImport(oneHrAgo.Hour) / 10.0);

        breakTime(tz->toLocal(now-3600), oneHrAgoLocal);
        ret |= updateMax(val, oneHrAgoLocal.Day);

        realtimeData->currentHour = local.Hour; // Need to be defined here so that day cost is correctly calculated

        realtimeData->use = 0;
        realtimeData->produce = 0;
        realtimeData->costHour = 0;
        realtimeData->incomeHour = 0;

        uint8_t prevDay = realtimeData->currentDay;
        if(local.Day != realtimeData->currentDay) {
            data.costYesterday = realtimeData->costDay * 100;
            data.costThisMonth += realtimeData->costDay * 100;
            realtimeData->costDay = 0;

            data.incomeYesterday = realtimeData->incomeDay * 100;
            data.incomeThisMonth += realtimeData->incomeDay * 100;
            realtimeData->incomeDay = 0;

            realtimeData->currentDay = local.Day;
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
            realtimeData->currentThresholdIdx = 0;
            ret = true;
        }

        if(ret) {
            data.costToday = realtimeData->costDay * 100;
            data.incomeToday = realtimeData->incomeDay * 100;
            data.lastUpdated = now;
        }
    }

    if(realtimeData->lastImportUpdateMillis < amsData->getLastUpdateMillis()) {
        unsigned long ms = amsData->getLastUpdateMillis() - realtimeData->lastImportUpdateMillis;
        float kwhi = (amsData->getActiveImportPower() * (((float) ms) / 3600000.0)) / 1000.0;
        if(kwhi > 0) {
            realtimeData->use += kwhi;
            float importPrice = ps == NULL ? PRICE_NO_VALUE : ps->getCurrentPrice(PRICE_DIRECTION_IMPORT);
            if(importPrice != PRICE_NO_VALUE) {
                float cost = importPrice * kwhi;
                realtimeData->costHour += cost;
                realtimeData->costDay += cost;
            }
        }
        realtimeData->lastImportUpdateMillis = amsData->getLastUpdateMillis();
    }

    if(amsData->getListType() > 1 && realtimeData->lastExportUpdateMillis < amsData->getLastUpdateMillis()) {
        unsigned long ms = amsData->getLastUpdateMillis() - realtimeData->lastExportUpdateMillis;
        float kwhe = (amsData->getActiveExportPower() * (((float) ms) / 3600000.0)) / 1000.0;
        if(kwhe > 0) {
            realtimeData->produce += kwhe;
            float exportPrice = ps == NULL ? PRICE_NO_VALUE : ps->getCurrentPrice(PRICE_DIRECTION_EXPORT);
            if(exportPrice != PRICE_NO_VALUE) {
                float income = exportPrice * kwhe;
                realtimeData->incomeHour += income;
                realtimeData->incomeDay += income;
            }
        }
        realtimeData->lastExportUpdateMillis = amsData->getLastUpdateMillis();
    }

    if(config != NULL) {
        while(getMonthMax() > config->thresholds[realtimeData->currentThresholdIdx] && realtimeData->currentThresholdIdx < 10) realtimeData->currentThresholdIdx++;
    }

    return ret;
}

void EnergyAccounting::calcDayCost() {
    time_t now = time(nullptr);
    tmElements_t local, utc, lastUpdateUtc;
    if(tz == NULL) return;
    breakTime(tz->toLocal(now), local);
    if(ps == NULL) return;

    if(ps->hasPrice()) {
        breakTime(data.lastUpdated, lastUpdateUtc);
        uint8_t calcFromHour = 0;
        if(lastUpdateUtc.Day != local.Day || lastUpdateUtc.Month != local.Month || lastUpdateUtc.Year != local.Year) {
            realtimeData->costDay = 0;
            realtimeData->incomeDay = 0;
            calcFromHour = 0;
        } else {
            realtimeData->costDay = data.costToday / 100.0;
            realtimeData->incomeDay = data.incomeToday / 100.0;
            calcFromHour = lastUpdateUtc.Hour;
        }
        for(uint8_t i = calcFromHour; i < realtimeData->currentHour; i++) {
            breakTime(now - ((local.Hour - i) * 3600), utc);

            float priceIn = ps->getPriceForRelativeHour(PRICE_DIRECTION_IMPORT, i - local.Hour);
            if(priceIn != PRICE_NO_VALUE) {
                int16_t wh = ds->getHourImport(utc.Hour);
                realtimeData->costDay += priceIn * (wh / 1000.0);
            }

            float priceOut = ps->getPriceForRelativeHour(PRICE_DIRECTION_EXPORT, i - local.Hour);
            if(priceOut != PRICE_NO_VALUE) {
                int16_t wh = ds->getHourExport(utc.Hour);
                realtimeData->incomeDay += priceOut * (wh / 1000.0);
            }
        }
        initPrice = true;
    }
}

float EnergyAccounting::getUseThisHour() {
    return realtimeData->use;
}

float EnergyAccounting::getUseToday() {
    if(tz == NULL) return 0.0;
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < realtimeData->currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourImport(utc.Hour) / 1000.0;
    }
    return ret + getUseThisHour();
}

float EnergyAccounting::getUseThisMonth() {
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < realtimeData->currentDay; i++) {
        ret += ds->getDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}

float EnergyAccounting::getUseLastMonth() {
    return (data.lastMonthImport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getProducedThisHour() {
    return realtimeData->produce;
}

float EnergyAccounting::getProducedToday() {
    if(tz == NULL) return 0.0;
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < realtimeData->currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourExport(utc.Hour) / 1000.0;
    }
    return ret + getProducedThisHour();
}

float EnergyAccounting::getProducedThisMonth() {
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < realtimeData->currentDay; i++) {
        ret += ds->getDayExport(i) / 1000.0;
    }
    return ret + getProducedToday();
}

float EnergyAccounting::getProducedLastMonth() {
    return (data.lastMonthExport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getCostThisHour() {
    return realtimeData->costHour;
}

float EnergyAccounting::getCostToday() {
    return realtimeData->costDay;
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
    return realtimeData->incomeHour;
}

float EnergyAccounting::getIncomeToday() {
    return realtimeData->incomeDay;
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
    return config->thresholds[realtimeData->currentThresholdIdx];
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

        if(buf[0] == 7) {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            memcpy(&this->data, data, sizeof(this->data));
            ret = true;
        } else if(buf[0] == 6) {
            EnergyAccountingData6* data = (EnergyAccountingData6*) buf;
            this->data = { 7, data->month, 
                0, // Cost today
                data->costYesterday,
                data->costThisMonth,
                data->costLastMonth,
                0, // Income today
                data->incomeYesterday,
                data->incomeThisMonth,
                data->incomeLastMonth,
                data->lastMonthImport,
                data->lastMonthExport,
                data->lastMonthAccuracy,
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value,
                0 // Last updated
            };
            ret = true;
        } else if(buf[0] == 5) {
            EnergyAccountingData5* data = (EnergyAccountingData5*) buf;
            this->data = { 7, data->month, 
                0, // Cost today
                ((int32_t) data->costYesterday) * 10,
                ((int32_t) data->costThisMonth) * 100,
                ((int32_t) data->costLastMonth) * 100,
                0, // Income today
                ((int32_t) data->incomeYesterday) * 10,
                ((int32_t) data->incomeThisMonth) * 100,
                ((int32_t) data->incomeLastMonth) * 100,
                0,0,0, // Last month import, export and accuracy
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value,
                0 // Last updated
            };
            ret = true;
        } else {
            data = { 7, 0, 
                0,0,0,0, // Cost
                0,0,0,0, // Income from production
                0,0,0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0, // Peak 5
                0 // Last updated
            };
            ret = false;
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
