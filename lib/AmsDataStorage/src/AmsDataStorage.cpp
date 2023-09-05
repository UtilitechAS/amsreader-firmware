#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "LittleFS.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
    day.version = 5;
    day.accuracy = 1;
    month.version = 6;
    month.accuracy = 1;
    this->debugger = debugger;
}

void AmsDataStorage::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool AmsDataStorage::update(AmsData* data) {
    if(isHappy()) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Data is up to date\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Time is: %lu\n"), (int32_t) now);
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }
    if(now < FirmwareVersion::BuildEpoch) {
        if(data->getMeterTimestamp() > FirmwareVersion::BuildEpoch) {
            now = data->getMeterTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Using meter timestamp, which is: %lu\n"), (int32_t) now);
            }
        } else if(data->getPackageTimestamp() > FirmwareVersion::BuildEpoch) {
            now = data->getPackageTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Using package timestamp, which is: %lu\n"), (int32_t) now);
            }
        }
    }
    if(now < FirmwareVersion::BuildEpoch) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid time: %lu\n"), (int32_t) now);
        }
        return false;
    }

    tmElements_t utc, ltz, utcYesterday, ltzYesterDay;
    breakTime(now, utc);
    breakTime(tz->toLocal(now), ltz);
    breakTime(now-3600, utcYesterday);
    breakTime(tz->toLocal(now-3600), ltzYesterDay);

    uint32_t importCounter = data->getActiveImportCounter() * 1000;
    uint32_t exportCounter = data->getActiveExportCounter() * 1000;

    // Clear hours between last update and now
    if(day.lastMeterReadTime > now) {
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid future timestamp for day plot, resetting\n"));
        }
        day.activeImport = importCounter;
        day.activeExport = exportCounter;
        day.lastMeterReadTime = now;
    } else if(day.activeImport == 0 || now - day.lastMeterReadTime > 86400) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf_P(PSTR("(AmsDataStorage) %lu == 0 || %lu - %lu > 86400"), day.activeImport, now, day.lastMeterReadTime);
        }
        day.activeImport = importCounter;
        day.activeExport = exportCounter;
        day.lastMeterReadTime = now;
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Too long since last day update, clearing data\n"));
        }
        for(int i = 0; i<24; i++) {
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    } else {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Last day update: %lu\n"), (int32_t) day.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(day.lastMeterReadTime, last);
        uint8_t endHour = utc.Hour;
        if(last.Hour > utc.Hour){
            for(int i = 0; i < utc.Hour; i++) {
                if(debugger->isActive(RemoteDebug::VERBOSE)) {
                    debugger->printf_P(PSTR("(AmsDataStorage) Clearing hour: %d\n"), i);
                }
                setHourImport(i, 0);
                setHourExport(i, 0);
            }
            endHour = 24;
        }
        for(int i = last.Hour; i < endHour; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Clearing hour: %d\n"), i);
            }
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    }

    // Clear days between last update and now
    if(month.lastMeterReadTime > now) {
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid future timestamp for month plot, resetting\n"));
        }
        month.activeImport = importCounter;
        month.activeExport = exportCounter;
        month.lastMeterReadTime = now;
    } else if(month.activeImport == 0 || now - month.lastMeterReadTime > 2682000) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf_P(PSTR("(AmsDataStorage) %lu == 0 || %lu - %lu > 2682000"), month.activeImport, now, month.lastMeterReadTime);
        }
        month.activeImport = importCounter;
        month.activeExport = exportCounter;
        month.lastMeterReadTime = now;
        if(debugger->isActive(RemoteDebug::WARNING)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Too long since last month update, clearing data\n"));
        }
        for(int i = 1; i<=31; i++) {
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    } else {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Last month update: %lu\n"), (int32_t) month.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(tz->toLocal(month.lastMeterReadTime), last);
        uint8_t endDay = ltz.Day;
        if(last.Day > ltz.Day) {
            for(int i = 1; i < ltz.Day; i++) {
                if(debugger->isActive(RemoteDebug::VERBOSE)) {
                    debugger->printf_P(PSTR("(AmsDataStorage) Clearing day: %d\n"), i);
                }
                setDayImport(i, 0);
                setDayExport(i, 0);
            }
            endDay = 31;
        }
        for(int i = last.Day; i < endDay; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Clearing day: %d\n"), i);
            }
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    }

    if(data->getListType() < 3) {
        debugger->printf_P(PSTR("(AmsDataStorage) Not enough data in list type: %d\n"), data->getListType());
        return false;
    }

    bool ret = false;

    // Update day plot
    if(!isDayHappy()) {
        if(day.activeImport > importCounter || day.activeExport > exportCounter) {
            day.activeImport = importCounter;
            day.activeExport = exportCounter;
            day.lastMeterReadTime = now;
            setHourImport(utcYesterday.Hour, 0);
            setHourExport(utcYesterday.Hour, 0);
        } else if(now - day.lastMeterReadTime < 4000) {
            uint32_t imp = importCounter - day.activeImport;
            uint32_t exp = exportCounter - day.activeExport;
            setHourImport(utcYesterday.Hour, imp);
            setHourExport(utcYesterday.Hour, exp);

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(AmsDataStorage) Usage for hour %d: %d - %d\n"), ltzYesterDay.Hour, imp, exp);
            day.activeImport = importCounter;
            day.activeExport = exportCounter;
            day.lastMeterReadTime = now;
        } else {
            float mins = (now - day.lastMeterReadTime) / 60.0;
            uint32_t im = importCounter - day.activeImport;
            uint32_t ex = exportCounter - day.activeExport;
            float ipm = im / mins;
            float epm = ex / mins;

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Since last day update, minutes: %.1f, import: %d (%.2f/min), export: %d (%.2f/min)\n"), mins, im, ipm, ex, epm);
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
                    debugger->printf_P(PSTR("(AmsDataStorage) Estimated usage for hour %u: %.1f - %.1f (%lu)\n"), last.Hour, imp, exp, (int32_t) cur);
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
        if(month.activeImport > importCounter || month.activeExport > exportCounter) {
            month.activeImport = importCounter;
            month.activeExport = exportCounter;
            month.lastMeterReadTime = now;
            setDayImport(ltzYesterDay.Day, 0);
            setDayExport(ltzYesterDay.Day, 0);
        } else if(now - month.lastMeterReadTime < 90100 && now - month.lastMeterReadTime > 82700) { // DST days are 23h (82800s) and 25h (90000)
            int32_t imp = importCounter - month.activeImport;
            int32_t exp = exportCounter - month.activeExport;

            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Usage for day %d: %d - %d\n"), ltzYesterDay.Day, imp, exp);
            }

            setDayImport(ltzYesterDay.Day, imp);
            setDayExport(ltzYesterDay.Day, exp);
            month.activeImport = importCounter;
            month.activeExport = exportCounter;
            month.lastMeterReadTime = now;
        } else {
            // Make sure last month read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(month.lastMeterReadTime), last);
            month.lastMeterReadTime = month.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;
            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Last month read after resetting to midnight: %lu\n"), (int32_t) month.lastMeterReadTime);
            }

            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint32_t im = importCounter - month.activeImport;
            uint32_t ex = exportCounter - month.activeExport;
            float iph = im / hrs;
            float eph = ex / hrs;

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Since last month update, hours: %.1f, import: %d (%.2f/hr), export: %d (%.2f/hr)\n"), hrs, im, iph, ex, eph);
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
                    debugger->printf_P(PSTR("(AmsDataStorage) Estimated usage for day %u: %.1f - %.1f (%lu)\n"), last.Day, imp, exp, (int32_t) cur);
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

void AmsDataStorage::setHourImport(uint8_t hour, uint32_t val) {
    if(hour < 0 || hour > 24) return;
    
    uint8_t accuracy = day.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) {
        accuracy++;
        update = val / pow(10, accuracy);
    }
   
    if(accuracy != day.accuracy) {
        setDayAccuracy(accuracy);
    }
    
    day.hImport[hour] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 24; i++) {
        if(day.hImport[i] > max)
            max = day.hImport[i];
        if(day.hExport[i] > max)
            max = day.hExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != day.accuracy) {
        setDayAccuracy(accuracy);
    }
}

uint32_t AmsDataStorage::getHourImport(uint8_t hour) {
    if(hour < 0 || hour > 24) return 0;
    return day.hImport[hour] * pow(10, day.accuracy);
}

void AmsDataStorage::setHourExport(uint8_t hour, uint32_t val) {
    if(hour < 0 || hour > 24) return;
    
    uint8_t accuracy = day.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != day.accuracy) {
        setDayAccuracy(accuracy);
    }

    day.hExport[hour] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 24; i++) {
        if(day.hImport[i] > max)
            max = day.hImport[i];
        if(day.hExport[i] > max)
            max = day.hExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != day.accuracy) {
        setDayAccuracy(accuracy);
    }
}

uint32_t AmsDataStorage::getHourExport(uint8_t hour) {
    if(hour < 0 || hour > 24) return 0;
    return day.hExport[hour] * pow(10, day.accuracy);
}

void AmsDataStorage::setDayImport(uint8_t day, uint32_t val) {
    if(day < 1 || day > 31) return;
    
    uint8_t accuracy = month.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != month.accuracy) {
        setMonthAccuracy(accuracy);
    }

    month.dImport[day-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 31; i++) {
        if(month.dImport[i] > max)
            max = month.dImport[i];
        if(month.dExport[i] > max)
            max = month.dExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != month.accuracy) {
        setMonthAccuracy(accuracy);
    }
}

uint32_t AmsDataStorage::getDayImport(uint8_t day) {
    if(day < 1 || day > 31) return 0;
    return (month.dImport[day-1] * pow(10, month.accuracy));
}

void AmsDataStorage::setDayExport(uint8_t day, uint32_t val) {
    if(day < 1 || day > 31) return;
    
    uint8_t accuracy = month.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != month.accuracy) {
        setMonthAccuracy(accuracy);
    }

    month.dExport[day-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 31; i++) {
        if(month.dImport[i] > max)
            max = month.dImport[i];
        if(month.dExport[i] > max)
            max = month.dExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != month.accuracy) {
        setMonthAccuracy(accuracy);
    }
}

uint32_t AmsDataStorage::getDayExport(uint8_t day) {
    if(day < 1 || day > 31) return 0;
    return (month.dExport[day-1] * pow(10, month.accuracy));
}

bool AmsDataStorage::load() {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Unable to load LittleFS\n"));
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

    return ret;
}

bool AmsDataStorage::save() {
    if(!LittleFS.begin()) {
        if(debugger->isActive(RemoteDebug::ERROR)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Unable to load LittleFS\n"));
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
    return true;
}

DayDataPoints AmsDataStorage::getDayData() {
    return day;
}

MonthDataPoints AmsDataStorage::getMonthData() {
    return month;
}

bool AmsDataStorage::setDayData(DayDataPoints& day) {
    if(day.version == 5) {
        this->day = day;
        return true;
    } else if(day.version == 4) {
        this->day = day;
        this->day.accuracy = 1;
        this->day.version = 5;
        return true;
    } else if(day.version == 3) {
        this->day = day;
        for(uint8_t i = 0; i < 24; i++) this->day.hExport[i] = 0;
        this->day.accuracy = 1;
        this->day.version = 5;
        return true;
    }
    return false;
}

bool AmsDataStorage::setMonthData(MonthDataPoints& month) {
    if(month.version == 6) {
        this->month = month;
        return true;
    } else if(month.version == 5) {
        this->month = month;
        this->month.accuracy = 1;
        this->month.version = 6;
        return true;
    } else if(month.version == 4) {
        this->month = month;
        for(uint8_t i = 0; i < 31; i++) this->month.dExport[i] = 0;
        this->month.accuracy = 1;
        this->month.version = 6;
        return true;
    }
    return false;
}

uint8_t AmsDataStorage::getDayAccuracy() {
    return day.accuracy;
}

void AmsDataStorage::setDayAccuracy(uint8_t accuracy) {
    if(day.accuracy != accuracy) {
        double multiplier = pow(10, day.accuracy)/pow(10, accuracy);
        for(uint8_t i = 0; i < 24; i++) {
            day.hImport[i] = day.hImport[i] * multiplier;
            day.hExport[i] = day.hExport[i] * multiplier;
        }
        day.accuracy = accuracy;
    }
}

uint8_t AmsDataStorage::getMonthAccuracy() {
    return month.accuracy;
}

void AmsDataStorage::setMonthAccuracy(uint8_t accuracy) {
    if(month.accuracy != accuracy) {
        double multiplier = pow(10, month.accuracy)/pow(10, accuracy);
        for(uint8_t i = 0; i < 31; i++) {
            month.dImport[i] = month.dImport[i] * multiplier;
            month.dExport[i] = month.dExport[i] * multiplier;
        }
        month.accuracy = accuracy;
    }
    month.accuracy = accuracy;
}

bool AmsDataStorage::isHappy() {
    return isDayHappy() && isMonthHappy();
}

bool AmsDataStorage::isDayHappy() {
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;

    if(now < day.lastMeterReadTime) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data timestamp %lu < %lu\n"), (int32_t) now, (int32_t) day.lastMeterReadTime);
        return false;
    }
    if(now-day.lastMeterReadTime > 3600) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data timestamp age %lu - %lu > 3600\n"), (int32_t) now, (int32_t) day.lastMeterReadTime);
        return false;
    }

    tmElements_t tm, last;
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(day.lastMeterReadTime), last);
    if(tm.Hour != last.Hour) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data hour of last timestamp %d > %d\n"), tm.Hour, last.Hour);
        return false;
    }

    return true;
}

bool AmsDataStorage::isMonthHappy() {
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;
    tmElements_t tm, last;
    
    if(now < month.lastMeterReadTime) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Month data timestamp %lu < %lu\n"), (int32_t) now, (int32_t) month.lastMeterReadTime);
        return false;
    }

    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(month.lastMeterReadTime), last);
    if(tm.Day != last.Day) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Month data day of last timestamp %d > %d\n"), tm.Day, last.Day);
        return false;
    }

    if(now-month.lastMeterReadTime > 90100) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Month %lu - %lu > 3600\n", (int32_t) now, (int32_t) month.lastMeterReadTime);
        return false;
    }

    return true;
}
