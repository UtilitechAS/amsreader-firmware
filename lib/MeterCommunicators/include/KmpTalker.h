/**
 * @copyright Utilitech AS 2023
 * License: All rights reserved
 * 
 */

#pragma once

#include <Stream.h>

#define DATA_PARSE_OK 0
#define DATA_PARSE_FAIL -1
#define DATA_PARSE_INCOMPLETE -2
#define DATA_PARSE_FOOTER_CHECKSUM_ERROR -5

struct KmpParserContext {
    uint8_t type;
    uint16_t length;
};

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
    KmpTalker(Stream *hanSerial, uint8_t* hanBuffer, uint16_t hanBufferSize);
    bool loop();
    void getData(KmpDataHolder& data);
    int getLastError();

private:
    Stream *hanSerial;
    uint8_t *hanBuffer = NULL;
    uint16_t hanBufferSize = 0;

    bool dataAvailable = false;
    int len = 0;
    int pos = DATA_PARSE_INCOMPLETE;
    int lastError = DATA_PARSE_OK;
    bool serialInit = false;

    uint64_t lastUpdate = 0;
    uint8_t batch = 0;
    KmpParserContext ctx;

    KmpDataHolder state;
};
