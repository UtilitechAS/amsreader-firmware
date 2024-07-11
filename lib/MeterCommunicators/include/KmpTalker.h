/**
 * @copyright Utilitech AS 2023
 * License: All rights reserved
 * 
 */

#pragma once

#include <Stream.h>

struct KmpDataHolder {
    uint32_t activeImportPower = 0, reactiveImportPower = 0, activeExportPower = 0, reactiveExportPower = 0;
    float l1voltage = 0, l2voltage = 0, l3voltage = 0, l1current = 0, l2current = 0, l3current = 0;
    uint32_t l1activeImportPower = 0, l2activeImportPower = 0, l3activeImportPower = 0;
    uint32_t l1activeExportPower = 0, l2activeExportPower = 0, l3activeExportPower = 0;
    double l1activeImportCounter = 0, l2activeImportCounter = 0, l3activeImportCounter = 0;
    double l1activeExportCounter = 0, l2activeExportCounter = 0, l3activeExportCounter = 0;
    float powerFactor = 0, l1PowerFactor = 0, l2PowerFactor = 0, l3PowerFactor = 0;
    double activeImportCounter = 0, reactiveImportCounter = 0, activeExportCounter = 0, reactiveExportCounter = 0;
    uint16_t meterId;
};

class KmpTalker {
public:
    KmpTalker(Stream *hanSerial);
    bool loop();
    void getData(KmpDataHolder& data);
};
