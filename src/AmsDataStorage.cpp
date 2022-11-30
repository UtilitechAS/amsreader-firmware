#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "LittleFS.h"
#include "AmsStorage.h"
#include "version.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
    day.version = 4;
    month.version = 5;
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
    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Time is: %lld\n", (int64_t) now);
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Timezone is missing\n");
        return false;
    }
    if(now < BUILD_EPOCH) {
        if(data->getMeterTimestamp() > BUILD_EPOCH) {
            now = data->getMeterTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using meter timestamp, which is: %lld\n", (int64_t) now);
            }
        } else if(data->getPackageTimestamp() > BUILD_EPOCH) {
            now = data->getPackageTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Using package timestamp, which is: %lld\n", (int64_t) now);
            }
        }
    }
    if(now < BUILD_EPOCH) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf("(AmsDataStorage) Invalid time: %lld\n", (int64_t) now);
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
            debugger->printf("(AmsDataStorage) Last day update: %lld\n", (int64_t) day.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(day.lastMeterReadTime, last);
        for(int i = last.Hour; i < utc.Hour; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf("(AmsDataStorage) Clearing hour: %d\n", i);
            }
            setHourImport(i, 0);
            setHourExport(i, 0);
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
            debugger->printf("(AmsDataStorage) Last month update: %lld\n", (int64_t) month.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(tz->toLocal(month.lastMeterReadTime), last);
        for(int i = last.Day; i < ltz.Day; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf("(AmsDataStorage) Clearing day: %d\n", i);
            }
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    }

    if(data->getListType() < 3) {
        debugger->printf("(AmsDataStorage) Not enough data in list type: %d\n", data->getListType());
        return false;
    }

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
                setHourImport(i, 0);
                setHourExport(i, 0);
            }
        } else if(now - day.lastMeterReadTime < 4000) {
            uint32_t imp = (data->getActiveImportCounter() * 1000) - day.activeImport;
            uint32_t exp = (data->getActiveExportCounter() * 1000) - day.activeExport;
            setHourImport(utcYesterday.Hour, imp);
            setHourExport(utcYesterday.Hour, exp);

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(AmsDataStorage) Usage for hour %d: %d - %d\n", ltzYesterDay.Hour, imp, exp);
            day.activeImport = data->getActiveImportCounter() * 1000;
            day.activeExport = data->getActiveExportCounter() * 1000;
            day.lastMeterReadTime = now;
        } else {
            float mins = (now - day.lastMeterReadTime) / 60.0;
            uint32_t im = (data->getActiveImportCounter() * 1000) - day.activeImport;
            uint32_t ex = (data->getActiveExportCounter() * 1000) - day.activeExport;
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
                float imp = (ipm * minutes);
                float exp = (epm * minutes);
                setHourImport(last.Hour, imp);
                setHourExport(last.Hour, exp);

                if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf("(AmsDataStorage) Estimated usage for hour %u: %.1f - %.1f (%lld)\n", last.Hour, imp, exp, (int64_t) cur);
                }

                day.activeImport += imp;
                day.activeExport += exp;
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
                setDayImport(i, 0);
                setDayExport(i, 0);
            }
        } else if(now - month.lastMeterReadTime < 86500 && now - month.lastMeterReadTime > 86300) {
            int32_t imp = (data->getActiveImportCounter() * 1000) - month.activeImport;
            int32_t exp = (data->getActiveExportCounter() * 1000) - month.activeExport;

            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf("(AmsDataStorage) Usage for day %d: %d - %d\n", ltzYesterDay.Day, imp, exp);
            }

            setDayImport(ltzYesterDay.Day, imp);
            setDayExport(ltzYesterDay.Day, exp);
            month.activeImport = data->getActiveImportCounter() * 1000;
            month.activeExport = data->getActiveExportCounter() * 1000;
            month.lastMeterReadTime = now;
        } else {
            // Make sure last month read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(month.lastMeterReadTime), last);
            month.lastMeterReadTime = month.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Last month read after resetting to midnight: %lld\n", (int64_t) month.lastMeterReadTime);
            }

            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint32_t im = (data->getActiveImportCounter() * 1000) - month.activeImport;
            uint32_t ex = (data->getActiveExportCounter() * 1000) - month.activeExport;
            float iph = im / hrs;
            float eph = ex / hrs;

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf("(AmsDataStorage) Since last month update, hours: %.1f, import: %d (%.2f/hr), export: %d (%.2f/hr)\n", hrs, im, iph, ex, eph);
            }

            time_t stopAt = now - (ltz.Hour * 3600) - (ltz.Minute * 60) - ltz.Second;
            while(month.lastMeterReadTime < stopAt) {
                time_t cur = min(month.lastMeterReadTime + 86400, stopAt);
                uint8_t hours = round((cur - month.lastMeterReadTime) / 3600.0);

                breakTime(tz->toLocal(month.lastMeterReadTime), last);
                float imp = (iph * hours);
                float exp = (eph * hours);
                setDayImport(last.Day, imp);
                setDayExport(last.Day, exp);

                if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf("(AmsDataStorage) Estimated usage for day %u: %.1f - %.1f (%lld)\n", last.Day, imp, exp, (int64_t) cur);
                }

                month.activeImport += imp;
                month.activeExport += exp;
                month.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }
    return ret;
}

        //EHorvat new 5 Minute plot
void AmsDataStorage::update5Minute(uint16_t wattimport, uint16_t wattexport) {
    for(int i = 59; i>0; i--) {
        Min5.mImport[i] = Min5.mImport[i-1] ;   
        Min5.mExport[i] = Min5.mExport[i-1] ;   
    }
    Min5.mImport[0] = wattimport;
    Min5.mExport[0] = wattexport;
}
int32_t AmsDataStorage::get5MinuteImp(uint8_t element) {
    if(element < 0 || element > 59) return 0;
    return Min5.mImport[element];
}
int32_t AmsDataStorage::get5MinuteExp(uint8_t element) {
    if(element < 0 || element > 59) return 0;
    return Min5.mExport[element];
}

void AmsDataStorage::setHourImport(uint8_t hour, int32_t val) {
    if(hour < 0 || hour > 24) return;
    day.hImport[hour] = val / 10;
}

int32_t AmsDataStorage::getHourImport(uint8_t hour) {
    if(hour < 0 || hour > 24) return 0;
    return day.hImport[hour] * 10;
}

void AmsDataStorage::setHourExport(uint8_t hour, int32_t val) {
    if(hour < 0 || hour > 24) return;
    day.hExport[hour] = val / 10;
}

int32_t AmsDataStorage::getHourExport(uint8_t hour) {
    if(hour < 0 || hour > 24) return 0;
    return day.hExport[hour] * 10;
}

void AmsDataStorage::setDayImport(uint8_t day, int32_t val) {
    if(day < 1 || day > 31) return;
    month.dImport[day-1] = val / 10;
}

int32_t AmsDataStorage::getDayImport(uint8_t day) {
    if(day < 1 || day > 31) return 0;
    return (month.dImport[day-1] * 10);
}

void AmsDataStorage::setDayExport(uint8_t day, int32_t val) {
    if(day < 1 || day > 31) return;
    month.dExport[day-1] = val / 10;
}

int32_t AmsDataStorage::getDayExport(uint8_t day) {
    if(day < 1 || day > 31) return 0;
    return (month.dExport[day-1] * 10);
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
        for(unsigned long i = 0; i < sizeof(day); i++) {
            file.write(buf[i]);
        }
        file.close();
    }
    {
        File file = LittleFS.open(FILE_MONTHPLOT, "w");
        char buf[sizeof(month)];
        memcpy(buf, &month, sizeof(month));
        for(unsigned long i = 0; i < sizeof(month); i++) {
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
    if(day.version == 4) {
        this->day = day;
        return true;
    } else if(day.version == 3) {
        this->day = day;
        for(uint8_t i = 0; i < 24; i++) this->day.hExport[i] = 0;
        this->day.version = 4;
        return true;
    }
    return false;
}

bool AmsDataStorage::setMonthData(MonthDataPoints& month) {
    if(month.version == 5) {
        this->month = month;
        return true;
    } else if(month.version == 4) {
        this->month = month;
        for(uint8_t i = 0; i < 31; i++) this->month.dExport[i] = 0;
        this->month.version = 5;
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
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Day %lld < %lld\n", (int64_t) now, (int64_t) day.lastMeterReadTime);
        return false;
    }
    if(now-day.lastMeterReadTime > 3600) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Day %lld - %lld > 3600\n", (int64_t) now, (int64_t) day.lastMeterReadTime);
        return false;
    }
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(day.lastMeterReadTime), last);
    if(tm.Hour > last.Hour) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Day %d > %d\n", tm.Hour, last.Hour);
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
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Month %lld < %lld\n", (int64_t) now, (int64_t) month.lastMeterReadTime);
        return false;
    }
    if(now-month.lastMeterReadTime > 86400) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Month %lld - %lld > 3600\n", (int64_t) now, (int64_t) month.lastMeterReadTime);
        return false;
    }
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(month.lastMeterReadTime), last);
    if(tm.Day > last.Day) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Month %d > %d\n", tm.Day, last.Day);
        return false;
    }

    return true;
}
