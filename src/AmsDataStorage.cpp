#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "LittleFS.h"
#include "AmsStorage.h"
#include "version.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
    day.version = 3;
    month.version = 4;
    this->debugger = debugger;
}

void AmsDataStorage::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool AmsDataStorage::update(AmsData* data) {
    if(isHappy()) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Data is up to date\n");
        return false;
    }

    time_t now = time(nullptr);
    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Time is: %d\n", now);
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Timezone is missing\n");
        return false;
    }
    if(now < BUILD_EPOCH) {
        if(data->getMeterTimestamp() > BUILD_EPOCH) {
            now = data->getMeterTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using meter timestamp, which is: %d\n", now);
            }
        } else if(data->getPackageTimestamp() > BUILD_EPOCH) {
            now = data->getPackageTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using package timestamp, which is: %d\n", now);
            }
        }
    }
    if(now < BUILD_EPOCH) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf("(AmsDataStorage) Invalid time: %d\n", now);
        }
        return false;
    }

    tmElements_t utc, ltz, utcYesterday, ltzYesterDay;
    breakTime(now, utc);
    breakTime(tz->toLocal(now), ltz);
    breakTime(now-3600, utcYesterday);
    breakTime(tz->toLocal(now-3600), ltzYesterDay);

    // Clear hours between last update and now
    if(day.lastMeterReadTime > now) {
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf("(AmsDataStorage) Invalid future timestamp for day plot, resetting\n");
        }
        day.activeImport = data->getActiveImportCounter() * 1000;
        day.activeExport = data->getActiveExportCounter() * 1000;
        day.lastMeterReadTime = now;
        return true;
    } else {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) Last day update: %d\n", day.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(day.lastMeterReadTime, last);
        for(int i = last.Hour; i < utc.Hour; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf("(AmsDataStorage) Clearing hour: %d\n", i);
            }
            setHour(i, 0);
        }
    }

    // Clear days between last update and now
    if(month.lastMeterReadTime > now) {
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf("(AmsDataStorage) Invalid future timestamp for month plot, resetting\n");
        }
        month.activeImport = data->getActiveImportCounter() * 1000;
        month.activeExport = data->getActiveExportCounter() * 1000;
        month.lastMeterReadTime = now;
    } else {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) Last month update: %d\n", month.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(tz->toLocal(month.lastMeterReadTime), last);
        for(int i = last.Day; i < ltz.Day; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf("(AmsDataStorage) Clearing day: %d\n", i);
            }
            setDay(i, 0);
        }
    }

    if(data->getListType() < 3) return false;

    bool ret = false;

    // Update day plot
    if(!isDayHappy()) {
        if(day.activeImport == 0 || now - day.lastMeterReadTime > 86400) {
            day.activeImport = data->getActiveImportCounter() * 1000;
            day.activeExport = data->getActiveExportCounter() * 1000;
            day.lastMeterReadTime = now;
            if(debugger->isActive(RemoteDebug::WARNING)) {
                debugger->printf("(AmsDataStorage) Too long since last day update, clearing data\n");
            }
            for(int i = 0; i<24; i++) {
                setHour(i, 0);
            }
        } else if(now - day.lastMeterReadTime < 4000) {
            int16_t val = (((data->getActiveImportCounter() * 1000) - day.activeImport) - ((data->getActiveExportCounter() * 1000) - day.activeExport));
            setHour(utcYesterday.Hour, val);

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(AmsDataStorage) Usage for hour %d: %d\n", ltzYesterDay.Hour, val);

            day.activeImport = data->getActiveImportCounter() * 1000;
            day.activeExport = data->getActiveExportCounter() * 1000;
            day.lastMeterReadTime = now;
        } else {
            float mins = (now - day.lastMeterReadTime) / 60.0;
            uint16_t im = ((data->getActiveImportCounter() * 1000) - day.activeImport);
            uint16_t ex = ((data->getActiveExportCounter() * 1000) - day.activeExport);
            float ipm = im / mins;
            float epm = ex / mins;

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Since last day update, minutes: %.1f, import: %d (%.2f/min), export: %d (%.2f/min)\n", mins, im, ipm, ex, epm);
            }

            tmElements_t last;
            breakTime(day.lastMeterReadTime, last);
            day.lastMeterReadTime = day.lastMeterReadTime - (last.Minute * 60) - last.Second;
            time_t stopAt = now - (utc.Minute * 60) - utc.Second;
            while(day.lastMeterReadTime < stopAt) {
                time_t cur = min(day.lastMeterReadTime + 3600, stopAt);
                uint8_t minutes = round((cur - day.lastMeterReadTime) / 60.0);
                if(minutes < 1) break;

                breakTime(day.lastMeterReadTime, last);
                float val = ((ipm * minutes) - (epm * minutes));
                setHour(last.Hour, val);

                if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf("(AmsDataStorage) Estimated usage for hour %u: %.1f (%lu)\n", last.Hour, val, cur);
                }

                day.activeImport += ipm * minutes;
                day.activeExport += epm * minutes;
                day.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }

    // Update month plot
    if(ltz.Hour == 0 && !isMonthHappy()) {
        if(month.activeImport == 0 || now - month.lastMeterReadTime > 2678400) {
            month.activeImport = data->getActiveImportCounter() * 1000;
            month.activeExport = data->getActiveExportCounter() * 1000;
            month.lastMeterReadTime = now;
            if(debugger->isActive(RemoteDebug::WARNING)) {
                debugger->printf("(AmsDataStorage) Too long since last month update, clearing data\n");
            }
            for(int i = 1; i<=31; i++) {
                setDay(i, 0);
            }
        } else if(now - month.lastMeterReadTime < 86500 && now - month.lastMeterReadTime > 86300) {
            int32_t val = (month.activeImport == 0 ? 0 : ((data->getActiveImportCounter() * 1000) - month.activeImport) - ((data->getActiveExportCounter() * 1000) - month.activeExport));

            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf("(AmsDataStorage) Usage for day %d: %d\n", ltzYesterDay.Day, val);
            }

            setDay(ltzYesterDay.Day, val);
            month.activeImport = data->getActiveImportCounter() * 1000;
            month.activeExport = data->getActiveExportCounter() * 1000;
            month.lastMeterReadTime = now;
        } else {
            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint16_t im = ((data->getActiveImportCounter() * 1000) - month.activeImport);
            uint16_t ex = ((data->getActiveExportCounter() * 1000) - month.activeExport);
            float iph = im / hrs;
            float eph = ex / hrs;

            // There is something wacky going on when it ends up here. The total value (im) is way way lower than it should be, which in 
            // turn causes low values for all estimates. And then when it returns to the normal case above, the value is waaay higher.

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Since last month update, hours: %.1f, import: %d (%.2f/hr), export: %d (%.2f/hr)\n", hrs, im, iph, ex, eph);
            }

            // Make sure last month read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(month.lastMeterReadTime), last);
            month.lastMeterReadTime = month.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Last month read after resetting to midnight: %lu\n", month.lastMeterReadTime);
            }

            time_t stopAt = now - (ltz.Hour * 3600) - (ltz.Minute * 60) - ltz.Second;
            while(month.lastMeterReadTime < stopAt) {
                time_t cur = min(month.lastMeterReadTime + 86400, stopAt);
                uint8_t hours = round((cur - month.lastMeterReadTime) / 3600.0);

                breakTime(tz->toLocal(month.lastMeterReadTime), last);
                float val = ((iph * hours) - (eph * hours));
                setDay(last.Day, val);

                if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf("(AmsDataStorage) Estimated usage for day %u: %.1f (%lu)\n", last.Day, val, cur);
                }

                month.activeImport += iph * hours;
                month.activeExport += eph * hours;
                month.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }
    return ret;
}

void AmsDataStorage::setHour(uint8_t hour, int32_t val) {
    if(hour < 0 || hour > 24) return;
    day.points[hour] = val / 10;
}

int32_t AmsDataStorage::getHour(uint8_t hour) {
    if(hour < 0 || hour > 24) return 0;
    return day.points[hour] * 10;
}

void AmsDataStorage::setDay(uint8_t day, int32_t val) {
    if(day < 1 || day > 31) return;
    month.points[day-1] = val / 10;
}

int32_t AmsDataStorage::getDay(uint8_t day) {
    if(day < 1 || day > 31) return 0;
    return (month.points[day-1] * 10);
}

bool AmsDataStorage::load() {
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
        ret = setDayData(*day);
    }

    if(LittleFS.exists(FILE_MONTHPLOT)) {
        File file = LittleFS.open(FILE_MONTHPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        MonthDataPoints* month = (MonthDataPoints*) buf;
        file.close();
        ret = ret && setMonthData(*month);
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

DayDataPoints AmsDataStorage::getDayData() {
    return day;
}

MonthDataPoints AmsDataStorage::getMonthData() {
    return month;
}

bool AmsDataStorage::setDayData(DayDataPoints& day) {
    if(day.version == 3) {
        this->day = day;
        return true;
    }
    return false;
}

bool AmsDataStorage::setMonthData(MonthDataPoints& month) {
    if(month.version == 4) {
        this->month = month;
        return true;
    }
    return false;
}

bool AmsDataStorage::isHappy() {
    return isDayHappy() && isMonthHappy();
}

bool AmsDataStorage::isDayHappy() {
    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return false;
    tmElements_t tm, last;

    if(now < day.lastMeterReadTime) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Day %lu < %lu\n", now, day.lastMeterReadTime);
        return false;
    }
    if(now-day.lastMeterReadTime > 3600) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Day %lu - %lu > 3600\n", now, day.lastMeterReadTime);
        return false;
    }
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(day.lastMeterReadTime), last);
    if(tm.Hour > last.Hour) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Day %d > %d\n", tm.Hour, last.Hour);
        return false;
    }

    return true;
}

bool AmsDataStorage::isMonthHappy() {
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Timezone is missing\n");
        return false;
    }

    time_t now = time(nullptr);
    if(now < BUILD_EPOCH) return false;
    tmElements_t tm, last;
    
    if(now < month.lastMeterReadTime) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Month %lu < %lu\n", now, month.lastMeterReadTime);
        return false;
    }
    if(now-month.lastMeterReadTime > 86400) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Month %lu - %lu > 3600\n", now, month.lastMeterReadTime);
        return false;
    }
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(month.lastMeterReadTime), last);
    if(tm.Day > last.Day) {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(AmsDataStorage) Month %d > %d\n", tm.Day, last.Day);
        return false;
    }

    return true;
}
