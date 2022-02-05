#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#include "RemoteDebug.h"
#include "Timezone.h"

#define EPOCH_2021_01_01 1609459200

struct DayDataPoints {
    uint8_t version;
    int16_t points[24];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
}; // 37 bytes

struct MonthDataPoints {
    uint8_t version;
    int16_t points[31];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
}; // 75 bytes

class AmsDataStorage {
public:
    AmsDataStorage(RemoteDebug*);
    void setTimezone(Timezone*);
    bool update(AmsData*);
    int32_t getHour(uint8_t);
    int32_t getDay(uint8_t);
    bool load();
    bool save();

    DayDataPoints getDayData();
    bool setDayData(DayDataPoints&);
    MonthDataPoints getMonthData();
    bool setMonthData(MonthDataPoints&);

private:
    Timezone* tz;
    DayDataPoints day = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    MonthDataPoints month = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    RemoteDebug* debugger;
    void setHour(uint8_t, int32_t);
    void setDay(uint8_t, int32_t);
};

#endif
