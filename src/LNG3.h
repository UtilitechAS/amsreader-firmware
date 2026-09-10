/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _LNG3_H
#define _LNG3_H

#include "AmsData.h"
#include "AmsConfiguration.h"
#include "DataParser.h"
#include "Cosem.h"

// Landis+Gyr E450 (IDIS 2DLM, Slovenia) pushes a bare structure: no capture
// object descriptor array (unlike LNG) and no OBIS codes, just the meter id as
// an octet string followed by naked integers. Two frame shapes alternate, both
// starting with the id, so the values below are what follows it.

struct Lng3Totals {          // 9-item frame
    CosemDLongUnsigned activeImportPower;     // 1.7.0 W
    CosemDLongUnsigned activeExportPower;     // 2.7.0 W
    CosemDLongUnsigned reactiveImportPower;   // 3.7.0 var
    CosemDLongUnsigned reactiveExportPower;   // 4.7.0 var
    CosemDLongUnsigned activeImportCounter;   // 1.8.0 Wh
    CosemDLongUnsigned activeExportCounter;   // 2.8.0 Wh
    CosemDLongUnsigned reactiveImportCounter; // 3.8.0 varh
    CosemDLongUnsigned reactiveExportCounter; // 4.8.0 varh
} __attribute__((packed));

struct Lng3Phases {          // 14-item frame
    CosemLongUnsigned frequency;              // 14.7.0 Hz (no AmsData field, unused)
    CosemLongUnsigned i1;                     // 31.7.0 cA
    CosemLongUnsigned i2;                     // 51.7.0 cA
    CosemLongUnsigned i3;                     // 71.7.0 cA
    CosemLongUnsigned u1;                     // 32.7.0 V, whole volts
    CosemLongUnsigned u2;                     // 52.7.0 V, whole volts
    CosemLongUnsigned u3;                     // 72.7.0 V, whole volts
    CosemDLongUnsigned p1;                    // 21.7.0 W
    CosemDLongUnsigned p2;                    // 41.7.0 W
    CosemDLongUnsigned p3;                    // 61.7.0 W
    CosemDLongUnsigned p1Export;              // 22.7.0 W
    CosemDLongUnsigned p2Export;              // 42.7.0 W
    CosemDLongUnsigned p3Export;              // 62.7.0 W
} __attribute__((packed));

class LNG3 : public AmsData {
public:
    // True if the payload is one of the two frame shapes above. The format
    // carries no identifier, so this checks the element count and the type of
    // every value before the parser casts the payload onto a struct.
    static bool matches(const char* payload, uint16_t length);

    LNG3(AmsData& meterState, const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx);
};

#endif
