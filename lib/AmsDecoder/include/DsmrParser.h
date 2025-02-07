/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _DSMRPARSER_H
#define _DSMRPARSER_H

#include "Arduino.h"
#include "DataParser.h"
#include "GcmParser.h"

class DSMRParser {
public:
    DSMRParser(GCMParser* gcmParser) { this->gcmParser = gcmParser; };
    int8_t parse(uint8_t *buf, DataParserContext &ctx, bool verified, Print* debugger);
    uint16_t getCrc();
    uint16_t getCrcCalc();
private:
    uint16_t crc;
    uint16_t crc_calc;

    GCMParser* gcmParser;
};

#endif
