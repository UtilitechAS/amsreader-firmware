#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#include "RemoteDebug.h"
#include "Timezone.h"

#define EPOCH_2021_01_01 1609459200

struct DayDataPoints {
    uint8_t version;
    int16_t h00;
    int16_t h01;
    int16_t h02;
    int16_t h03;
    int16_t h04;
    int16_t h05;
    int16_t h06;
    int16_t h07;
    int16_t h08;
    int16_t h09;
    int16_t h10;
    int16_t h11;
    int16_t h12;
    int16_t h13;
    int16_t h14;
    int16_t h15;
    int16_t h16;
    int16_t h17;
    int16_t h18;
    int16_t h19;
    int16_t h20;
    int16_t h21;
    int16_t h22;
    int16_t h23;
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
}; // 37 bytes

struct MonthDataPoints {
    uint8_t version;
    int16_t d01;
    int16_t d02;
    int16_t d03;
    int16_t d04;
    int16_t d05;
    int16_t d06;
    int16_t d07;
    int16_t d08;
    int16_t d09;
    int16_t d10;
    int16_t d11;
    int16_t d12;
    int16_t d13;
    int16_t d14;
    int16_t d15;
    int16_t d16;
    int16_t d17;
    int16_t d18;
    int16_t d19;
    int16_t d20;
    int16_t d21;
    int16_t d22;
    int16_t d23;
    int16_t d24;
    int16_t d25;
    int16_t d26;
    int16_t d27;
    int16_t d28;
    int16_t d29;
    int16_t d30;
    int16_t d31;
    time_t lastMeterReadTime;
    uint32_t activeImport;
    uint32_t activeExport;
}; // 75 bytes

class AmsDataStorage {
public:
    AmsDataStorage(RemoteDebug*);
    void setTimezone(Timezone*);
    bool update(AmsData*);
    DayDataPoints getDayDataPoints();
    MonthDataPoints getMonthDataPoints();
    bool load(AmsData*);
    bool save();

private:
    Timezone* tz;
    DayDataPoints day;
    MonthDataPoints month;
    RemoteDebug* debugger;

	void printD(String fmt, ...);
	void printI(String fmt, ...);
	void printW(String fmt, ...);
	void printE(String fmt, ...);
};

#endif
