#include "LNG.h"
#include "lwip/def.h"
#include "ams/Cosem.h"

LNG::LNG(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, RemoteDebug* debugger) {
    LngHeader* h = (LngHeader*) payload;
    if(h->tag == CosemTypeStructure && h->arrayTag == CosemTypeArray) {
        meterType = AmsTypeLng;
        this->packageTimestamp = ctx.timestamp;

        uint8_t* ptr = (uint8_t*) &h[1];
        uint8_t* data = ptr + (18*h->arrayLength); // Skip descriptors

        uint16_t o170 = 0, o270 = 0;
        uint16_t o181 = 0, o182 = 0;
        uint16_t o281 = 0, o282 = 0;
        LngObisDescriptor* descriptor = (LngObisDescriptor*) ptr;
        for(uint8_t x = 0;  x < h->arrayLength-1; x++) {
            ptr = (uint8_t*) &descriptor[1];
            descriptor = (LngObisDescriptor*) ptr;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(L&G) OBIS %d.%d.%d with type 0x%02X", descriptor->obis[2], descriptor->obis[3], descriptor->obis[4], *data);

            CosemData* item = (CosemData*) data;
            if(descriptor->obis[2] == 1) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        o170 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        activeImportCounter = ntohl(item->dlu.data) / 1000.0;
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    } else if(descriptor->obis[4] == 1) {
                        o181 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    } else if(descriptor->obis[4] == 2) {
                        o182 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    }
                } 
            } else if(descriptor->obis[2] == 2) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        o270 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        activeExportCounter = ntohl(item->dlu.data) / 1000.0;
                        listType = listType >= 3 ? listType : 3;
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    } else if(descriptor->obis[4] == 1) {
                        o281 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    } else if(descriptor->obis[4] == 2) {
                        o282 = ntohl(item->dlu.data);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %lu (dlu)", ntohl(item->dlu.data));
                    }
                } 
            } else if(descriptor->obis[2] == 96) {
                if(descriptor->obis[3] == 1) {
                    if(descriptor->obis[4] == 0) {
                        char str[item->oct.length+1];
                        memcpy(str, item->oct.data, item->oct.length);
                        str[item->oct.length] = '\0';
                        meterId = String(str);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %s (oct)", str);
                    } else if(descriptor->obis[4] == 1) {
                        char str[item->oct.length+1];
                        memcpy(str, item->oct.data, item->oct.length);
                        str[item->oct.length] = '\0';
                        meterModel = String(str);
                        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf(" and value %s (oct)", str);
                    }
                }
            }

            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("\n");

            if(o170 > 0 || o270 > 0) {
                int32_t sum = o170-o270;
                if(sum > 0) {
                    listType = listType >= 1 ? listType : 1;
                    activeImportPower = sum;
                } else {
                    listType = listType >= 2 ? listType : 2;
                    activeExportPower = sum * -1;
                }
            }

            if(o181 > 0 || o182 > 0) {
                activeImportCounter = (o181 + o182) / 1000.0;
                listType = listType >= 3 ? listType : 3;
            }
            if(o281 > 0 || o282 > 0) {
                activeExportCounter = (o281 + o282) / 1000.0;
                listType = listType >= 3 ? listType : 3;
            }

            if((*data) == 0x09) {
                data += (*(data+1))+2;
            } else {
                data += 5;
            }

            lastUpdateMillis = millis();
        }
    }
}