#include "ams.h"
#include <string.h>
#include "lwip/def.h"
#include "Time.h"

time_t AMS_getTimestamp(uint8_t* obis, const char* ptr) {
    CosemData* item = AMS_findObis(obis, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeOctetString: {
                if(item->oct.length == 0x0C) {
                    AmsOctetTimestamp* ts = (AmsOctetTimestamp*) item;
                    tmElements_t tm;
                    tm.Year = ntohs(ts->year) - 1970;
                    tm.Month = ts->month;
                    tm.Day = ts->dayOfMonth;
                    tm.Hour = ts->hour;
                    tm.Minute = ts->minute;
                    tm.Second = ts->second;

                    time_t time = makeTime(tm);
                    int16_t deviation = ntohs(ts->deviation);
                    if(deviation >= -720 && deviation <= 720) {
                        time -= deviation * 60;
                    }
                    return time;
                }
            }
        }
    }
    return 0;
}

uint8_t AMS_getString(uint8_t* obis, const char* ptr, char* target) {
    CosemData* item = AMS_findObis(obis, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeString:
                memcpy(target, item->str.data, item->str.length);
                target[item->str.length] = 0;
                return item->str.length;
            case CosemTypeOctetString:
                memcpy(target, item->oct.data, item->oct.length);
                target[item->oct.length] = 0;
                return item->oct.length;
        }
    }
    return 0;
}

uint32_t AMS_getUnsignedNumber(uint8_t* obis, const char* ptr) {
    CosemData* item = AMS_findObis(obis, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeLongUnsigned:
                return ntohs(item->lu.data);
            case CosemTypeDLongUnsigned:
                return ntohl(item->dlu.data);
        }
    }
    return 0xFFFFFFFF;
}

int32_t AMS_getSignedNumber(uint8_t* obis, const char* ptr) {
    CosemData* item = AMS_findObis(obis, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeLongUnsigned:
                return ntohs(item->lu.data);
            case CosemTypeDLongUnsigned:
                return ntohl(item->dlu.data);
            case CosemTypeLongSigned:
                return ntohs(item->lu.data);
        }
    }
    return 0xFFFFFFFF;
}

CosemData* AMS_findObis(uint8_t* obis, const char* ptr) {
    CosemData* item = (CosemData*) ptr;
    int ret = 0;
    char* pos = (char*) ptr;
    do {
        item = (CosemData*) pos;
        if(ret == 1) return item;
        switch(item->base.type) {
            case CosemTypeArray:
            case CosemTypeStructure:
                pos += 2;
                break;
            case CosemTypeOctetString: {
                ret = 1;
                uint8_t* found = item->oct.data;
                int x = 6 - sizeof(&obis);
                for(int i = x; i < 6; i++) {
                    if(found[i] != obis[i-x]) ret = 0;
                }
            }
            case CosemTypeString: {
                pos += 2 + item->base.length;
                break;
            }
            case CosemTypeLongSigned:
                pos += 5;
                break;
            case CosemTypeLongUnsigned:
                pos += 3;
                break;
            case CosemTypeDLongUnsigned:
                pos += 5;
                break;
            case CosemTypeNull:
                return NULL;
            default:
                pos += 2;
        }
    } while(item->base.type != HDLC_FLAG);
    return NULL;
}