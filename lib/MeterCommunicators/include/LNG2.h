/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _LNG2_H
#define _LNG2_H

#include "AmsData.h"
#include "AmsConfiguration.h"
#include "DataParser.h"
#include "Cosem.h"

struct Lng2Data_3p_0b {
    CosemBasic header;
    CosemLongUnsigned u1;
    CosemLongUnsigned u2;
    CosemLongUnsigned u3;
    CosemLongUnsigned i1;
    CosemLongUnsigned i2;
    CosemLongUnsigned i3;
    CosemDLongUnsigned activeImport;
    CosemDLongUnsigned activeExport;
    CosemDLongUnsigned acumulatedImport;
    CosemDLongUnsigned accumulatedExport;
    CosemString meterId;
} __attribute__((packed));

struct Lng2Data_3p_0e {
    CosemBasic header;
    CosemLongUnsigned u1;
    CosemLongUnsigned u2;
    CosemLongUnsigned u3;
    CosemLongUnsigned i1;
    CosemLongUnsigned i2;
    CosemLongUnsigned i3;
    CosemDLongUnsigned activeImport;
    CosemDLongUnsigned activeExport;
    CosemDLongUnsigned acumulatedImport;
    CosemDLongUnsigned accumulatedExport;
    CosemLongUnsigned x;
    CosemLongUnsigned y;
    CosemLongUnsigned z;
    CosemString meterId;
} __attribute__((packed));

class LNG2 : public AmsData {
public:
    LNG2(AmsData& meterState, const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx);

private:
    uint8_t getString(CosemData* item, char* target);

};

#endif
