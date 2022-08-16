#include "LNG.h"
#include "lwip/def.h"
#include "ams/Cosem.h"

LNG::LNG(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx) {
    LngHeader* h = (LngHeader*) payload;
    if(h->tag == CosemTypeStructure && h->arrayTag == CosemTypeArray) {
        meterType = AmsTypeLng;
        this->packageTimestamp = ctx.timestamp;

        uint8_t* ptr = (uint8_t*) &h[1];
        uint8_t* data = ptr + (18*h->arrayLength); // Skip descriptors

        for(uint8_t i = 0; i < h->arrayLength; i++) {
            LngObisDescriptor* descriptor = (LngObisDescriptor*) ptr;
            if(descriptor->obis[2] == 1) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        CosemDLongUnsigned* item = (CosemDLongUnsigned*) data;
                        activeImportPower = ntohl(item->data);
                        listType = listType >= 1 ? listType : 1;
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        CosemDLongUnsigned* item = (CosemDLongUnsigned*) data;
                        activeImportCounter = ntohl(item->data);
                        listType = listType >= 3 ? listType : 3;
                    }
                } 
            } else if(descriptor->obis[2] == 2) {
                if(descriptor->obis[3] == 7) {
                    if(descriptor->obis[4] == 0) {
                        CosemDLongUnsigned* item = (CosemDLongUnsigned*) data;
                        activeExportPower = ntohl(item->data);
                        listType = listType >= 2 ? listType : 2;
                    }
                } else if(descriptor->obis[3] == 8) {
                    if(descriptor->obis[4] == 0) {
                        CosemDLongUnsigned* item = (CosemDLongUnsigned*) data;
                        activeExportCounter = ntohl(item->data);
                        listType = listType >= 3 ? listType : 3;
                    }
                } 
            } else if(descriptor->obis[2] == 96) {
                if(descriptor->obis[3] == 1) {
                    if(descriptor->obis[4] == 0) {
                        CosemString* item = (CosemString*) data;
                        char str[item->length+1];
                        memcpy(str, item->data, item->length);
                        str[item->length] = '\0';
                        meterId = String(str);
                    }
                }
            }

            ptr = (uint8_t*) &descriptor[1];

            if((*data) == 0x09) {
                data += (*data+1)+2;
            } else {
                data += 5;
            }
        }
    }
}