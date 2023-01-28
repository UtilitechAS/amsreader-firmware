#ifndef _LNG_H
#define _LNG_H

#include "AmsData.h"
#include "AmsConfiguration.h"
#include "DataParser.h"
#include "Cosem.h"
#include "RemoteDebug.h"

struct LngHeader {
    uint8_t tag;
    uint8_t values;
    uint8_t arrayTag;
    uint8_t arrayLength;
} __attribute__((packed));

struct LngObisDescriptor {
    uint8_t ignore1[5];
    uint8_t octetTag;
    uint8_t octetLength;
    uint8_t obis[6];
    uint8_t ignore2[5];
} __attribute__((packed));


class LNG : public AmsData {
public:
    LNG(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, RemoteDebug* debugger);
    uint64_t getNumber(CosemData* item);
};

#endif
