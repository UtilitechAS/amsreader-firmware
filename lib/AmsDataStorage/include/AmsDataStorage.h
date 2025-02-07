/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "Timezone.h"

struct DayDataPoints5 {
    uint8_t version;
    uint16_t hImport[24];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    uint16_t hExport[24];
    uint8_t accuracy;
};

struct MonthDataPoints6 {
    uint8_t version;
    uint16_t dImport[31];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    uint16_t dExport[31];
    uint8_t accuracy;
};

struct DayDataPoints {
    uint8_t version;
    uint16_t hImport[24];
    time_t lastMeterReadTime;
    uint64_t activeImport;
    uint64_t activeExport;
    uint16_t hExport[24];
    uint8_t accuracy;
};

struct MonthDataPoints {
    uint8_t version;
    uint16_t dImport[31];
    time_t lastMeterReadTime;
    uint64_t activeImport;
    uint64_t activeExport;
    uint16_t dExport[31];
    uint8_t accuracy;
};

class AmsDataStorage {
public:
    #if defined(AMS_REMOTE_DEBUG)
    AmsDataStorage(RemoteDebug*);
    #else
    AmsDataStorage(Stream*);
    #endif
    void setTimezone(Timezone*);
    bool update(AmsData* data, time_t now);
    uint32_t getHourImport(uint8_t);
    uint32_t getHourExport(uint8_t);
    uint32_t getDayImport(uint8_t);
    uint32_t getDayExport(uint8_t);
    bool load();
    bool save();

    DayDataPoints getDayData();
    bool setDayData(DayDataPoints&);
    MonthDataPoints getMonthData();
    bool setMonthData(MonthDataPoints&);

    uint8_t getDayAccuracy();
    void setDayAccuracy(uint8_t);
    uint8_t getMonthAccuracy();
    void setMonthAccuracy(uint8_t);

    bool isHappy(time_t now);
    bool isDayHappy(time_t now);
    bool isMonthHappy(time_t now);

    double getEstimatedImportCounter();

    void setHourImport(uint8_t, uint32_t);
    void setHourExport(uint8_t, uint32_t);
    void setDayImport(uint8_t, uint32_t);
    void setDayExport(uint8_t, uint32_t);

private:
    Timezone* tz;
    DayDataPoints day = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        10
    };
    MonthDataPoints month = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        10
    };
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
    #else
    Stream* debugger;
    #endif
};

#endif
