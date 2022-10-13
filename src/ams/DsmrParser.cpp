#include "DsmrParser.h"

int8_t DSMRParser::parse(uint8_t *buf, DataParserContext &ctx, bool verified) {
    uint16_t crcPos = 0;
    bool reachedEnd = verified;
    uint8_t lastByte = 0x00;
    for(int pos = 0; pos < ctx.length; pos++) {
        uint8_t b = *(buf+pos);
        if(pos == 0 && b != '/') return DATA_PARSE_BOUNDRY_FLAG_MISSING;
        if(pos > 0 && b == '!' && lastByte == '\n') crcPos = pos+1;
        if(crcPos > 0 && b == '\n') reachedEnd = true;
        lastByte = b;
    }
    if(!reachedEnd) return DATA_PARSE_INCOMPLETE;
    buf[ctx.length+1] = '\0';
    if(crcPos > 0) {
        // TODO: CRC
        Serial.printf("CRC: %s\n", buf+crcPos);
    }
    return DATA_PARSE_OK;
}