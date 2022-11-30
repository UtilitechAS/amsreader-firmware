#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#include "RemoteDebug.h"
#include "Timezone.h"

struct DayDataPoints {
    uint8_t version;
    int16_t hImport[24];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    int16_t hExport[24];
}; // 112 bytes

struct MonthDataPoints {
    uint8_t version;
    int16_t dImport[31];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    int16_t dExport[31];
}; // 141 bytes

//EHorvat new 5 Minute plot
struct MinuteDataPoints {
    uint8_t version;
    int16_t mImport[60];
    int16_t mExport[60];
}; // 241 bytes

class AmsDataStorage {
public:
    AmsDataStorage(RemoteDebug*);
    void setTimezone(Timezone*);
    bool update(AmsData*);
    int32_t getHourImport(uint8_t);
    int32_t getHourExport(uint8_t);
    int32_t getDayImport(uint8_t);
    int32_t getDayExport(uint8_t);
    int32_t get5MinuteImp(uint8_t);                 //EHorvat new 5 Minute plot
    int32_t get5MinuteExp(uint8_t);                 //EHorvat new 5 Minute plot
    void update5Minute(uint16_t, uint16_t);           //EHorvat new 5 Minute plot    
    bool load();
    bool save();

    DayDataPoints getDayData();
    bool setDayData(DayDataPoints&);
    MonthDataPoints getMonthData();
    bool setMonthData(MonthDataPoints&);

    bool isHappy();
    bool isDayHappy();
    bool isMonthHappy();

private:
    Timezone* tz;
    DayDataPoints day = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    MonthDataPoints month = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
        //EHorvat new 5 Minute plot
    MinuteDataPoints Min5 = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    RemoteDebug* debugger;
    void setHourImport(uint8_t, int32_t);
    void setHourExport(uint8_t, int32_t);
    void setDayImport(uint8_t, int32_t);
    void setDayExport(uint8_t, int32_t);
};

#endif
