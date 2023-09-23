#include "LNG.h"
#include "lwip/def.h"
#include "ntohll.h"
#include "Uptime.h"

LNG::LNG(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, RemoteDebug* debugger) {
    LngHeader* h = (LngHeader*) payload;
    if(h->tag == CosemTypeStructure && h->arrayTag == CosemTypeArray) {
        meterType = AmsTypeLandisGyr;
        this->packageTimestamp = ctx.timestamp;

        uint8_t* ptr = (uint8_t*) &h[1];
        uint8_t* data = ptr + (18*h->arrayLength); // Skip descriptors

        uint64_t o170 = 0, o270 = 0;
        uint64_t o180 = 0, o280 = 0;
        uint64_t o181 = 0, o182 = 0;
        uint64_t o281 = 0, o282 = 0;
        LngObisDescriptor* descriptor = (LngObisDescriptor*) ptr;
        for(uint8_t x = 0;  x < h->arrayLength-1; x++) {
            ptr = (uint8_t*) &descriptor[1];
            descriptor = (LngObisDescriptor*) ptr;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(L&G) OBIS %d.%d.%d with type 0x%02X\n"), descriptor->obis[2], descriptor->obis[3], descriptor->obis[4], *data);

            CosemData* item = (CosemData*) data;
            if(descriptor->obis[2] == 1) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        o170 = getNumber(item);
                        listType = listType >= 1 ? listType : 1;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o170);
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        o180 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o180);
                        activeImportCounter = o180 / 1000.0;
                    } else if(descriptor->obis[4] == 1) {
                        o181 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o181);
                    } else if(descriptor->obis[4] == 2) {
                        o182 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o182);
                    }
                } 
            } else if(descriptor->obis[2] == 2) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        o270 = getNumber(item);
                        listType = listType >= 2 ? listType : 2;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o270);
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        o280 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o280);
                        activeExportCounter = o280 / 1000.0;
                    } else if(descriptor->obis[4] == 1) {
                        o281 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o281);
                    } else if(descriptor->obis[4] == 2) {
                        o282 = getNumber(item);
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %lu\n"), o282);
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
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %s (oct)\n"), str);
                    } else if(descriptor->obis[4] == 1) {
                        char str[item->oct.length+1];
                        memcpy(str, item->oct.data, item->oct.length);
                        str[item->oct.length] = '\0';
                        meterModel = String(str);
                        listType = listType >= 2 ? listType : 2;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR(" and value %s (oct)\n"), str);
                    }
                }
            }

            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("\n"));

            if(o170 > 0 || o270 > 0) {
                int32_t sum = o170-o270;
                if(sum > 0) {
                    activeImportPower = sum;
                } else {
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
