#ifndef _DSMRPARSER_H
#define _DSMRPARSER_H

#include "Arduino.h"
#include "DataParser.h"

class DSMRParser {
public:
    int8_t parse(uint8_t *buf, DataParserContext &ctx, bool verified);
    uint16_t getCrc();
    uint16_t getCrcCalc();
private:
    uint16_t crc;
    uint16_t crc_calc;
};

#endif
