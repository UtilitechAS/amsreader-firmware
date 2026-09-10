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
// object descriptor array (unlike LNG) and no OBIS codes, just a serial number
// as an octet string followed by naked integers. Two frame shapes alternate,
// both led by a serial, so the values below are what follows it. Registers are
// per the push object lists supplied by the DSO; note that the two frames lead
// with *different* id registers.

struct Lng3Totals {          // 9-item frame, led by 0-0:96.1.0 (manufacturer serial)
    CosemDLongUnsigned activeImportPower;     // 1-0:1.7.0  active power import +P, W
    CosemDLongUnsigned activeExportPower;     // 1-0:2.7.0  active power export -P, W
    CosemDLongUnsigned reactiveImportPower;   // 1-0:3.7.0  reactive power import +Q, var
    CosemDLongUnsigned reactiveExportPower;   // 1-0:4.7.0  reactive power export -Q, var
    CosemDLongUnsigned activeImportCounter;   // 1-0:1.8.0  active energy +A (QI+QIV), Wh
    CosemDLongUnsigned activeExportCounter;   // 1-0:2.8.0  active energy -A (QII+QIII), Wh
    CosemDLongUnsigned reactiveImportCounter; // 1-0:3.8.0  reactive energy +R (QI+QII), varh
    CosemDLongUnsigned reactiveExportCounter; // 1-0:4.8.0  reactive energy -R (QIII+QIV), varh
} __attribute__((packed));

struct Lng3Phases {          // 14-item frame, led by 0-0:96.1.1 (utility serial 2)
    CosemLongUnsigned frequency;              // 1-0:14.7.0 net frequency, any phase (no AmsData field, unused)
    CosemLongUnsigned i1;                     // 1-0:31.7.0 current L1, cA
    CosemLongUnsigned i2;                     // 1-0:51.7.0 current L2, cA
    CosemLongUnsigned i3;                     // 1-0:71.7.0 current L3, cA
    CosemLongUnsigned u1;                     // 1-0:32.7.0 voltage L1, whole volts
    CosemLongUnsigned u2;                     // 1-0:52.7.0 voltage L2, whole volts
    CosemLongUnsigned u3;                     // 1-0:72.7.0 voltage L3, whole volts
    CosemDLongUnsigned p1;                    // 1-0:21.7.0 active power import +P L1, W
    CosemDLongUnsigned p2;                    // 1-0:41.7.0 active power import +P L2, W
    CosemDLongUnsigned p3;                    // 1-0:61.7.0 active power import +P L3, W
    CosemDLongUnsigned p1Export;              // 1-0:22.7.0 active power export -P L1, W
    CosemDLongUnsigned p2Export;              // 1-0:42.7.0 active power export -P L2, W
    CosemDLongUnsigned p3Export;              // 1-0:62.7.0 active power export -P L3, W
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
