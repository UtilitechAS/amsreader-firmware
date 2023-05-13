#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#include "RemoteDebug.h"
#include "Timezone.h"

struct DayDataPoints {
    uint8_t version;
    uint16_t hImport[24];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    uint16_t hExport[24];
    uint8_t accuracy;
}; // 113 bytes

struct MonthDataPoints {
    uint8_t version;
    uint16_t dImport[31];
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
    uint16_t dExport[31];
    uint8_t accuracy;
}; // 142 bytes

//EHorvat new hour plot.
struct HourDataPoints {
    uint8_t version;
    int32_t mWatt[180];
}; // 723 bytes
//EHorvat new hour plot END

class AmsDataStorage {
public:
    AmsDataStorage(RemoteDebug*);
    void setTimezone(Timezone*);
    bool update(AmsData*);
    int32_t getHourWatt(uint8_t);        //EHorvat new hour plot.
    void updateHour(int32_t);            //EHorvat new hour plot.
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

    bool isHappy();
    bool isDayHappy();
    bool isMonthHappy();

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
    
    //EHorvat new 1 hour plot start
    HourDataPoints hour = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    //EHorvat new 1 hour plot end

          RemoteDebug* debugger;
    void setHourImport(uint8_t, uint32_t);
    void setHourExport(uint8_t, uint32_t);
    void setDayImport(uint8_t, uint32_t);
    void setDayExport(uint8_t, uint32_t);
};

#endif
