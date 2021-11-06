#ifndef _AMS_H
#define _AMS_H

#include "Arduino.h"
#include "hdlc.h"

struct AmsOctetTimestamp {
    uint16_t year;
    uint8_t month;
    uint8_t dayOfMonth;
    uint8_t dayOfWeek;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t hundredths;
    int16_t deviation;
    uint8_t status;
} __attribute__((packed));


CosemData* AMS_findObis(uint8_t* obis, int matchlength, const char* ptr);
uint32_t AMS_getUnsignedNumber(uint8_t* obis, int matchlength, const char* ptr);
int32_t AMS_getSignedNumber(uint8_t* obis, int matchlength, const char* ptr);
uint8_t AMS_getString(uint8_t* obis, int matchlength, const char* ptr, char* target);
time_t AMS_getTimestamp(uint8_t* obis, int matchlength, const char* ptr);

#endif
