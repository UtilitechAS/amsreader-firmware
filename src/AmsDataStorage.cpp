#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "EEPROM.h"
#include "LittleFS.h"
#include "AmsStorage.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
    day.version = 3;
    month.version = 4;
    this->debugger = debugger;
}

void AmsDataStorage::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool AmsDataStorage::update(AmsData* data) {
    if(data->getListType() != 3) return false;
    time_t now = time(nullptr);
    if(debugger->isActive(RemoteDebug::DEBUG)) {
        debugger->printf("(AmsDataStorage) Time is: %d\n", now);
    }
    if(now < EPOCH_2021_01_01) {
        if(data->getMeterTimestamp() > 0) {
            now = data->getMeterTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using meter timestamp, which is: %d\n", now);
            }
        } else if(data->getPackageTimestamp() > 0) {
            now = data->getPackageTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using package timestamp, which is: %d\n", now);
            }
        }
    }
    if(now-day.lastMeterReadTime < 3595) {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) It is only %d seconds since last update, ignoring\n", (now-day.lastMeterReadTime));
        }
        return false;
    }

    tmElements_t tm, last;
    breakTime(now, tm);
    breakTime(day.lastMeterReadTime, last);
    for(int i = last.Hour; i < tm.Hour; i++) {
        debugger->printf("(AmsDataStorage) Clearing hour: %d\n", i);
        setHour(i, 0);
    }

    // Update day plot
    if(day.activeImport == 0 || now - day.lastMeterReadTime > 86400) {
        day.activeImport = data->getActiveImportCounter() * 1000;
        day.activeExport = data->getActiveExportCounter() * 1000;
        day.lastMeterReadTime = now;
        for(int i = 0; i<24; i++) {
            setHour(i, 0);
        }
    } else if(now - day.lastMeterReadTime < 4000) {
        breakTime(now - 3600, tm);
        int16_t val = (((data->getActiveImportCounter() * 1000) - day.activeImport) - ((data->getActiveExportCounter() * 1000) - day.activeExport));
        setHour(tm.Hour, val);

        if(debugger->isActive(RemoteDebug::INFO)) {
            debugger->printf("(AmsDataStorage) Usage for hour %d: %d\n", tm.Hour, val);
        }

        day.activeImport = data->getActiveImportCounter() * 1000;
        day.activeExport = data->getActiveExportCounter() * 1000;
        day.lastMeterReadTime = now;
    } else {
        float mins = (now - day.lastMeterReadTime) / 60.0;
        uint16_t im = ((data->getActiveImportCounter() * 1000) - day.activeImport);
        uint16_t ex = ((data->getActiveExportCounter() * 1000) - day.activeExport);
        float ipm = im / mins;
        float epm = ex / mins;

        while(now - day.lastMeterReadTime > 3590) {
            time_t cur = day.lastMeterReadTime + 3600;
            tmElements_t tm;
            breakTime(cur, tm);
            uint8_t minutes = 60 - tm.Minute;
            float val = ((ipm * minutes) - (epm * minutes));
            setHour(tm.Hour-1, val);

            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf("(AmsDataStorage) Estimated usage for hour %d: %d\n", tm.Hour, val);
            }

            day.activeImport += ipm * minutes;
            day.activeExport += epm * minutes;
            day.lastMeterReadTime += 60 * minutes;
        }
    }

    // Update month plot
    if(tz != NULL) {
        time_t local = tz->toLocal(now);
        breakTime(local, tm);
    } else {
        breakTime(now, tm);
    }
    breakTime(month.lastMeterReadTime, last);
    for(int i = last.Day; i < tm.Day; i++) {
        debugger->printf("(AmsDataStorage) Clearing day: %d\n", i);
        setDay(i, 0);
    }
    if(tm.Hour == 0 && now-month.lastMeterReadTime > 86300) {
        if(month.activeImport == 0 || now - month.lastMeterReadTime > 2678400) {
            month.activeImport = data->getActiveImportCounter() * 1000;
            month.activeExport = data->getActiveExportCounter() * 1000;
            month.lastMeterReadTime = now;
            if(debugger->isActive(RemoteDebug::WARNING)) {
                debugger->printf("(AmsDataStorage) Too long since last update, clearing data\n");
            }
            for(int i = 0; i<31; i++) {
                setDay(i, 0);
            }
        } else if(now - month.lastMeterReadTime < 87000) {
            int32_t val = (month.activeImport == 0 ? 0 : ((data->getActiveImportCounter() * 1000) - month.activeImport) - ((data->getActiveExportCounter() * 1000) - month.activeExport));

            //if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf("(AmsDataStorage) Usage for day %d: %d\n", tm.Day, val);
            //}

            time_t yesterday = now - 3600;
            breakTime(yesterday, tm);
            setDay(tm.Day, val);

            month.activeImport = data->getActiveImportCounter() * 1000;
            month.activeExport = data->getActiveExportCounter() * 1000;
            month.lastMeterReadTime = now;
        } else {
            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint16_t im = ((data->getActiveImportCounter() * 1000) - month.activeImport);
            uint16_t ex = ((data->getActiveExportCounter() * 1000) - month.activeExport);
            float iph = im / hrs;
            float eph = ex / hrs;

            while(now - month.lastMeterReadTime > 86000) {
                time_t cur = month.lastMeterReadTime + 86400;
                tmElements_t tm;
                breakTime(cur, tm);
                uint8_t hours = 24 - tm.Hour;
                float val = ((iph * hours) - (eph * hours));
                setDay(tm.Day-1, val);

                //if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf("(AmsDataStorage) Estimated usage for day %d: %d\n", tm.Day, val);
                //}

                month.activeImport += iph * hours;
                month.activeExport += eph * hours;
                month.lastMeterReadTime += 24 * hours;
            }
        }
    }
    return true;
}

void AmsDataStorage::setHour(uint8_t hour, int16_t val) {
    day.points[hour] = val / 10;
}

int16_t AmsDataStorage::getHour(uint8_t hour) {
    return day.points[hour] * 10;
}

void AmsDataStorage::setDay(uint8_t day, int32_t val) {
    month.points[day-1] = val / 10;
}

int32_t AmsDataStorage::getDay(uint8_t day) {
    return (month.points[day-1] * 10);
}

bool AmsDataStorage::load(AmsData* meterState) {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf("(AmsDataStorage) Unable to load LittleFS\n");
        }
        return false;
    }
    bool ret = false;
    if(LittleFS.exists(FILE_DAYPLOT)) {
        File file = LittleFS.open(FILE_DAYPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        DayDataPoints* day = (DayDataPoints*) buf;
        file.close();

        if(day->version == 3) {
            memcpy(&this->day, day, sizeof(this->day));
            ret = true;
        } else {
            ret = false;
        }
    }

    if(LittleFS.exists(FILE_MONTHPLOT)) {
        File file = LittleFS.open(FILE_MONTHPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        MonthDataPoints* month = (MonthDataPoints*) buf;
        file.close();

        if(month->version == 4) {
            memcpy(&this->month, month, sizeof(this->month));
            ret = true;
        } else {
            ret = false;
        }
    }

    LittleFS.end();

    return ret;
}

bool AmsDataStorage::save() {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf("(AmsDataStorage) Unable to load LittleFS\n");
        }
        return false;
    }
    {
        File file = LittleFS.open(FILE_DAYPLOT, "w");
        char buf[sizeof(day)];
        memcpy(buf, &day, sizeof(day));
        for(int i = 0; i < sizeof(day); i++) {
            file.write(buf[i]);
        }
        file.close();
    }
    {
        File file = LittleFS.open(FILE_MONTHPLOT, "w");
        char buf[sizeof(month)];
        memcpy(buf, &month, sizeof(month));
        for(int i = 0; i < sizeof(month); i++) {
            file.write(buf[i]);
        }
        file.close();
    }

    LittleFS.end();
    return true;
}
