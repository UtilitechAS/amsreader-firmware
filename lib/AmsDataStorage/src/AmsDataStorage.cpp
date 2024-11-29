/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "LittleFS.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"

#if defined(AMS_REMOTE_DEBUG)
AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
#else
AmsDataStorage::AmsDataStorage(Stream* debugger) {
#endif
    day.version = 6;
    day.accuracy = 1;
    month.version = 7;
    month.accuracy = 1;
    this->debugger = debugger;
}

void AmsDataStorage::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool AmsDataStorage::update(AmsData* data, time_t now) {
    if(isHappy(now)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Happy, not updating\n"));
        return false;
    }

    if(tz == NULL) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("No timezone, not updating\n"));
        return false;
    }
    if(now < FirmwareVersion::BuildEpoch) {
        if(data->getMeterTimestamp() > FirmwareVersion::BuildEpoch) {
            now = data->getMeterTimestamp();
        } else if(data->getPackageTimestamp() > FirmwareVersion::BuildEpoch) {
            now = data->getPackageTimestamp();
        }
    }
    if(now < FirmwareVersion::BuildEpoch) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Before build time, not updating\n"));
        return false;
    }

    tmElements_t utc, ltz, utcYesterday, ltzYesterDay;
    breakTime(now, utc);
    breakTime(tz->toLocal(now), ltz);
    breakTime(now-3600, utcYesterday);
    breakTime(tz->toLocal(now-3600), ltzYesterDay);

    uint64_t importCounter = data->getActiveImportCounter() * 1000;
    uint64_t exportCounter = data->getActiveExportCounter() * 1000;

    // Clear hours between last update and now
    if(!isDayHappy(now) && day.lastMeterReadTime > now) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Day was updated in the future, resetting\n"));
        day.activeImport = importCounter;
        day.activeExport = exportCounter;
        day.lastMeterReadTime = now;
    } else if(importCounter > 0 && day.activeImport == 0) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Initializing day data\n"));
        day.activeImport = importCounter;
        day.activeExport = exportCounter;
        day.lastMeterReadTime = now;
        for(int i = 0; i<24; i++) {
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    } else if(now - day.lastMeterReadTime > 86400) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Day was updated to long ago, clearing\n"));
        day.activeImport = importCounter;
        day.activeExport = exportCounter;
        day.lastMeterReadTime = now;
        for(int i = 0; i<24; i++) {
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    } else {
        tmElements_t last;
        breakTime(day.lastMeterReadTime, last);
        uint8_t endHour = utc.Hour;
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Clearing hours from %d to %d\n"), last.Hour, endHour);
        if(last.Hour > utc.Hour){
            for(int i = 0; i < utc.Hour; i++) {
                setHourImport(i, 0);
                setHourExport(i, 0);
            }
            endHour = 24;
        }
        for(int i = last.Hour; i < endHour; i++) {
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    }

    // Clear days between last update and now
    if(!isMonthHappy(now) && month.lastMeterReadTime > now) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Month was updated in the future, resetting\n"));
        month.activeImport = importCounter;
        month.activeExport = exportCounter;
        month.lastMeterReadTime = now;
    } else if(importCounter > 0 && month.activeImport == 0) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Initializing month data\n"));
        month.activeImport = importCounter;
        month.activeExport = exportCounter;
        month.lastMeterReadTime = now;
        for(int i = 1; i<=31; i++) {
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    } else if(now - month.lastMeterReadTime > 2682000) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Month was updated to long ago, clearing\n"));
        month.activeImport = importCounter;
        month.activeExport = exportCounter;
        month.lastMeterReadTime = now;
        for(int i = 1; i<=31; i++) {
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    } else {
        tmElements_t last;
        breakTime(tz->toLocal(month.lastMeterReadTime), last);
        uint8_t endDay = ltz.Day;
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Clearing days from %d to %d\n"), last.Day, endDay);
        if(last.Day > ltz.Day) {
            for(int i = 1; i < ltz.Day; i++) {
                setDayImport(i, 0);
                setDayExport(i, 0);
            }
            endDay = 31;
        }
        for(int i = last.Day; i < endDay; i++) {
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    }

    if(data->getListType() < 3) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Type %d, not updating\n"), data->getListType());
        return false;
    }

    bool ret = false;

    // Update day plot
    if(!isDayHappy(now)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Day is not happy\n"));
        if(day.activeImport > importCounter || day.activeExport > exportCounter) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - reset\n"));
            day.activeImport = importCounter;
            day.activeExport = exportCounter;
            day.lastMeterReadTime = now;
            setHourImport(utcYesterday.Hour, 0);
            setHourExport(utcYesterday.Hour, 0);
        } else if(now - day.lastMeterReadTime < 4000) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - normal\n"));
            uint32_t imp = importCounter - day.activeImport;
            uint32_t exp = exportCounter - day.activeExport;
            setHourImport(utcYesterday.Hour, imp);
            setHourExport(utcYesterday.Hour, exp);

            day.activeImport = importCounter;
            day.activeExport = exportCounter;
            day.lastMeterReadTime = now;
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - average\n"));
            float mins = (now - day.lastMeterReadTime) / 60.0;
            uint32_t im = importCounter - day.activeImport;
            uint32_t ex = exportCounter - day.activeExport;
            float ipm = im / mins;
            float epm = ex / mins;

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

                day.activeImport += imp;
                day.activeExport += exp;
                day.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }

    // Update month plot
    if(ltz.Hour == 0 && !isMonthHappy(now)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
            debugger->printf_P(PSTR("Month is not happy\n"));
        if(month.activeImport > importCounter || month.activeExport > exportCounter) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - reset\n"));
            month.activeImport = importCounter;
            month.activeExport = exportCounter;
            month.lastMeterReadTime = now;
            setDayImport(ltzYesterDay.Day, 0);
            setDayExport(ltzYesterDay.Day, 0);
        } else if(now - month.lastMeterReadTime < 90100 && now - month.lastMeterReadTime > 82700) { // DST days are 23h (82800s) and 25h (90000)
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - normal\n"));
            uint32_t imp = importCounter - month.activeImport;
            uint32_t exp = exportCounter - month.activeExport;

            setDayImport(ltzYesterDay.Day, imp);
            setDayExport(ltzYesterDay.Day, exp);
            month.activeImport = importCounter;
            month.activeExport = exportCounter;
            month.lastMeterReadTime = now;
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
                debugger->printf_P(PSTR(" - average\n"));
            // Make sure last month read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(month.lastMeterReadTime), last);
            month.lastMeterReadTime = month.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;

            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint32_t im = importCounter - month.activeImport;
            uint32_t ex = exportCounter - month.activeExport;
            float iph = im / hrs;
            float eph = ex / hrs;

            time_t stopAt = now - (ltz.Hour * 3600) - (ltz.Minute * 60) - ltz.Second;
            while(month.lastMeterReadTime < stopAt) {
                time_t cur = min(month.lastMeterReadTime + 86400, stopAt);
                uint8_t hours = round((cur - month.lastMeterReadTime) / 3600.0);

                breakTime(tz->toLocal(month.lastMeterReadTime), last);
                float imp = (iph * hours);
                float exp = (eph * hours);
                setDayImport(last.Day, imp);
                setDayExport(last.Day, exp);

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
        return false;
    }

    bool ret = false;
    if(LittleFS.exists(FILE_DAYPLOT)) {
        File file = LittleFS.open(FILE_DAYPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        if(buf[0] > 5) {
            DayDataPoints* day = (DayDataPoints*) buf;
            ret = setDayData(*day);
        } else {
            DayDataPoints5* old = (DayDataPoints5*) buf;
            DayDataPoints day = { old->version };
            day.lastMeterReadTime = old->lastMeterReadTime;
            day.activeImport = old->activeImport;
            day.activeExport = old->activeExport;
            day.accuracy = old->accuracy;
            for(uint8_t i = 0; i < 24; i++) {
                day.hImport[i] = old->hImport[i];
                day.hExport[i] = old->hExport[i];
            }

            ret = setDayData(day);
        }
        file.close();
    }

    if(LittleFS.exists(FILE_MONTHPLOT)) {
        File file = LittleFS.open(FILE_MONTHPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        if(buf[0] > 6) {
            MonthDataPoints* month = (MonthDataPoints*) buf;
            ret &= setMonthData(*month);
        } else {
            MonthDataPoints6* old = (MonthDataPoints6*) buf;
            MonthDataPoints month = { old->version };
            month.lastMeterReadTime = old->lastMeterReadTime;
            month.activeImport = old->activeImport;
            month.activeExport = old->activeExport;
            month.accuracy = old->accuracy;
            for(uint8_t i = 0; i < 31; i++) {
                month.dImport[i] = old->dImport[i];
                month.dExport[i] = old->dExport[i];
            }

            ret &= setMonthData(month);
        }
        file.close();
    }

    return ret;
}

bool AmsDataStorage::save() {
    if(!LittleFS.begin()) {
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
    if(day.version == 5 || day.version == 6) {
        this->day = day;
        this->day.version = 6;
        return true;
    } else if(day.version == 4) {
        this->day = day;
        this->day.accuracy = 1;
        this->day.version = 6;
        return true;
    } else if(day.version == 3) {
        this->day = day;
        for(uint8_t i = 0; i < 24; i++) this->day.hExport[i] = 0;
        this->day.accuracy = 1;
        this->day.version = 6;
        return true;
    }
    return false;
}

bool AmsDataStorage::setMonthData(MonthDataPoints& month) {
    if(month.version == 6 || month.version == 7) {
        this->month = month;
        this->month.version = 7;
        return true;
    } else if(month.version == 5) {
        this->month = month;
        this->month.accuracy = 1;
        this->month.version = 7;
        return true;
    } else if(month.version == 4) {
        this->month = month;
        for(uint8_t i = 0; i < 31; i++) this->month.dExport[i] = 0;
        this->month.accuracy = 1;
        this->month.version = 7;
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

bool AmsDataStorage::isHappy(time_t now) {
    return isDayHappy(now) && isMonthHappy(now);
}

bool AmsDataStorage::isDayHappy(time_t now) {
    if(tz == NULL) {
        return false;
    }

    if(now < FirmwareVersion::BuildEpoch) return false;

    if(now < day.lastMeterReadTime) {
        return false;
    }
    // There are cases where the meter reports before the hour. The update method will then receive the meter timestamp as reference, thus there will not be 3600s between. 
    // Leaving a 100s buffer for these cases
    if(now-day.lastMeterReadTime > 3500) {
        return false;
    }

    tmElements_t tm, last;
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(day.lastMeterReadTime), last);
    if(tm.Hour != last.Hour) {
        return false;
    }

    return true;
}

bool AmsDataStorage::isMonthHappy(time_t now) {
    if(tz == NULL) {
        return false;
    }

    if(now < FirmwareVersion::BuildEpoch) return false;
    
    if(now < month.lastMeterReadTime) {
        return false;
    }

    // 25 hours, because of DST
    if(now-month.lastMeterReadTime > 90000) {
        return false;
    }

    tmElements_t tm, last;
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(month.lastMeterReadTime), last);
    if(tm.Day != last.Day) {
        return false;
    }

    return true;
}

double AmsDataStorage::getEstimatedImportCounter() {
    if(day.lastMeterReadTime == 0) return 0;

    time_t now = time(nullptr);
    double hours = (now - day.lastMeterReadTime) / 3600.0;
    uint64_t total = 0;
    for(uint8_t i = 0; i < 24; i++) {
        total += getHourImport(i);
    }
    double perHour = total / 24.0;
    return (day.activeImport + (perHour * hours)) / 1000.0;
}