/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "LNG.h"
#include "lwip/def.h"
#include "ntohll.h"
#include "Uptime.h"

LNG::LNG(AmsData& meterState, const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx) {
    LngHeader* h = (LngHeader*) payload;
    if(h->tag == CosemTypeStructure && h->arrayTag == CosemTypeArray) {
        apply(meterState);
        meterType = AmsTypeLandisGyr;
        this->packageTimestamp = ctx.timestamp;

        uint8_t* ptr = (uint8_t*) &h[1];
        uint8_t* data = ptr + (18*h->arrayLength); // Skip descriptors

        uint64_t o170 = 0, o270 = 0;
        uint64_t o180 = 0, o280 = 0;
        uint64_t o181 = 0, o182 = 0;
        uint64_t o281 = 0, o282 = 0;
        uint64_t o380 = 0, o480 = 0;
        uint64_t o580 = 0, o680 = 0;
        uint64_t o780 = 0, o880 = 0;
        LngObisDescriptor* descriptor = (LngObisDescriptor*) ptr;
        for(uint8_t x = 0;  x < h->arrayLength-1; x++) {
            ptr = (uint8_t*) &descriptor[1];
            descriptor = (LngObisDescriptor*) ptr;

            CosemData* item = (CosemData*) data;
            if(descriptor->obis[3] == 7) {
                if(descriptor->obis[4] == 0) {
                    if(descriptor->obis[2] > 1) {
                        listType = listType >= 2 ? listType : 2;
                    } else {
                        listType = listType >= 1 ? listType : 1;
                    }
                    switch(descriptor->obis[2]) {
                        case 1:
                            o170 = getNumber(item);
                            break;
                        case 2:
                            o270 = getNumber(item);
                            break;
                        case 3:
                            reactiveImportPower = getNumber(item);
                            break;
                        case 4:
                            reactiveExportPower = getNumber(item);
                            break;
                        case 31:
                            l1current = getNumber(item) / 100.0;
                            break;
                        case 51:
                            l2current = getNumber(item) / 100.0;
                            break;
                        case 71:
                            l3current = getNumber(item) / 100.0;
                            break;
                        case 32:
                            l1voltage = getNumber(item) / 10.0;
                            break;
                        case 52:
                            l2voltage = getNumber(item) / 10.0;
                            break;
                        case 72:
                            l3voltage = getNumber(item) / 10.0;
                            break;
                    }
                }
            } else if(descriptor->obis[3] == 8) {
                listType = listType >= 3 ? listType : 3;
                if(descriptor->obis[4] == 0) {
                    switch(descriptor->obis[2]) {
                        case 1:
                            o180 = getNumber(item);
                            activeImportCounter = o180 / 1000.0;
                            break;
                        case 2:
                            o280 = getNumber(item);
                            activeExportCounter = o280 / 1000.0;
                            break;
                        case 3:
                            o380 = getNumber(item);
                            reactiveImportCounter = o380 / 1000.0;
                            break;
                        case 4:
                            o480 = getNumber(item);
                            reactiveExportCounter = o480 / 1000.0;
                            break;
                        case 5:
                            o580 = getNumber(item);
                            break;
                        case 6:
                            o680 = getNumber(item);
                            break;
                        case 7:
                            o780 = getNumber(item);
                            break;
                        case 8:
                            o880 = getNumber(item);
                            break;
                    }
                } else if(descriptor->obis[4] == 1) {
                    listType = listType >= 3 ? listType : 3;
                    switch(descriptor->obis[2]) {
                        case 1:
                            o181 = getNumber(item);
                            break;
                        case 2:
                            o281 = getNumber(item);
                            break;
                    }
                } else if(descriptor->obis[4] == 2) {
                    listType = listType >= 3 ? listType : 3;
                    switch(descriptor->obis[2]) {
                        case 1:
                            o182 = getNumber(item);
                            break;
                        case 2:
                            o282 = getNumber(item);
                            break;
                    }
                }
            } else if(descriptor->obis[2] == 96) {
                if(descriptor->obis[3] == 1) {
                    if(descriptor->obis[4] == 0) {
                        char str[item->oct.length+1];
                        memcpy(str, item->oct.data, item->oct.length);
                        str[item->oct.length] = '\0';
                        meterId = String(str);
                        listType = listType >= 2 ? listType : 2;
                    } else if(descriptor->obis[4] == 1) {
                        char str[item->oct.length+1];
                        memcpy(str, item->oct.data, item->oct.length);
                        str[item->oct.length] = '\0';
                        meterModel = String(str);
                        listType = listType >= 2 ? listType : 2;
                    }
                }
            }

            if(o170 > 0 || o270 > 0) {
                int32_t sum = o170-o270;
                if(sum > 0) {
                    activeImportPower = sum;
                    activeExportPower = 0;
                } else {
                    activeImportPower = 0;
                    activeExportPower = sum * -1;
                    listType = listType >= 2 ? listType : 2;
                }
            }

            if(o181 > 0 || o182 > 0) {
                activeImportCounter = (o181 + o182) / 1000.0;
            }
            if(o281 > 0 || o282 > 0) {
                activeExportCounter = (o281 + o282) / 1000.0;
            }

            if(o580 > 0 || o680 > 0) {
                reactiveImportCounter = (o580 + o680) / 1000.0;
            }
            if(o780 > 0 || o880 > 0) {
                reactiveExportCounter = (o780 + o880) / 1000.0;
            }

            if((*data) == 0x09) {
                data += (*(data+1))+2;
            } else if((*data) == 0x15) {
                data += 9;
            } else if((*data) == 0x06) {
                data += 5;
            } else if((*data) == 0x12) {
                data += 3;
            }

            lastUpdateMillis = millis64();
        }
        lastUpdateMillis = millis64();
        if(meterConfig->wattageMultiplier > 0) {
            activeImportPower = activeImportPower > 0 ? activeImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
            activeExportPower = activeExportPower > 0 ? activeExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
            reactiveImportPower = reactiveImportPower > 0 ? reactiveImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
            reactiveExportPower = reactiveExportPower > 0 ? reactiveExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        }
        if(meterConfig->voltageMultiplier > 0) {
            l1voltage = l1voltage > 0 ? l1voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
            l2voltage = l2voltage > 0 ? l2voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
            l3voltage = l3voltage > 0 ? l3voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
        }
        if(meterConfig->amperageMultiplier > 0) {
            l1current = l1current > 0 ? l1current * (meterConfig->amperageMultiplier / 1000.0) : 0;
            l2current = l2current > 0 ? l2current * (meterConfig->amperageMultiplier / 1000.0) : 0;
            l3current = l3current > 0 ? l3current * (meterConfig->amperageMultiplier / 1000.0) : 0;
        }
        if(meterConfig->accumulatedMultiplier > 0) {
            activeImportCounter = activeImportCounter > 0 ? activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
            activeExportCounter = activeExportCounter > 0 ? activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
            reactiveImportCounter = reactiveImportCounter > 0 ? reactiveImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
            reactiveExportCounter = reactiveExportCounter > 0 ? reactiveExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        }

        threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
        if(!threePhase)
            twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);
    }
}

uint64_t LNG::getNumber(CosemData* item) {
    if(item != NULL) {
        uint64_t ret = 0.0;
        switch(item->base.type) {
            case CosemTypeLongSigned: {
                int16_t i16 = ntohs(item->ls.data);
                return i16;
            }
            case CosemTypeLongUnsigned: {
                uint16_t u16 = ntohs(item->lu.data);
                return u16;
            }
            case CosemTypeDLongSigned: {
                int32_t i32 = ntohl(item->dlu.data);
                return i32;
            }
            case CosemTypeDLongUnsigned: {
                uint32_t u32 = ntohl(item->dlu.data);
                return u32;
            }
            case CosemTypeLong64Signed: {
                int64_t i64 = ntohll(item->l64s.data);
                return i64;
            }
            case CosemTypeLong64Unsigned: {
                uint64_t u64 = ntohll(item->l64u.data);
                return u64;
            }
        }
        return ret;
    }
    return 0.0;
}
