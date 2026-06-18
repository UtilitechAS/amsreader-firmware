/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#include "DsmrParser.h"
#include "crc.h"
#include "hexutils.h"
#include "byteorder.h"

// verified indicates that this data was encapsulated in something else, so we know this has the correct size etc
int8_t DSMRParser::parse(uint8_t *buf, DataParserContext &ctx, bool verified, Print* debugger) {
    uint16_t lenBefore = ctx.length;
    uint16_t crcPos = 0;
    bool reachedEnd = verified;
    for(uint16_t pos = 0; pos < ctx.length; pos++) {
        uint8_t b = *(buf+pos);
        if(pos == 0 && b != '/') return DATA_PARSE_BOUNDARY_FLAG_MISSING;
        if(pos > 0 && b == '!') crcPos = pos+1;
        // End of telegram is the first LF after the CRC line. Accept both
        // CRLF (\r\n, the P1 standard) and bare LF (\n) line endings.
        if(crcPos > 0 && b == 0x0A) {
            reachedEnd = true;
            ctx.length = pos;
            break;
        }
    }
    if(!reachedEnd) return DATA_PARSE_INCOMPLETE;
    buf[ctx.length+1] = '\0';

    // If we expect data to be encrypted and it was not previously verified, decrypt content
    if(gcmParser != NULL && !verified) {
        uint8_t* ptr = (uint8_t*) buf;
        while(*ptr != 0x0D && *ptr != 0x0A) ptr++;
        while(*ptr == 0x0D || *ptr == 0x0A) ptr++;
        uint16_t pos = ptr-buf;
        DataParserContext gcmCtx = {
            DATA_TAG_GCM,
            crcPos - pos - 1,
            ctx.timestamp
        };
        if(debugger != NULL) {
            debugger->printf("DSMR wants to decrypt at position %lu, length: %d, payload:\n", pos, gcmCtx.length);
            debugPrint(ptr, 0, gcmCtx.length, debugger);
        }
        int8_t gcmRet = gcmParser->parse(ptr, gcmCtx, false);
        if(gcmRet < 0) {
            if(debugger != NULL) {
                debugger->printf(" - Failed! (%d)\n", gcmRet);
            }
            return gcmRet;
        } else {
            if(debugger != NULL) {
                debugger->printf(" - Success! (%d)\n", gcmRet);
            }
            ptr += gcmRet;
            for(uint16_t i = 0; i < gcmCtx.length; i++) {
                buf[pos++] = ptr[i];
            }
            ptr = buf + crcPos - 1;
            crcPos = pos + 1;
            while(*ptr != '\0') {
                ctx.length = pos;
                buf[pos++] = *(ptr++);
            }
            while(pos < lenBefore) {
                buf[pos++] = '\0';
            }
        }
    } else if(crcPos > 0) {
	    crc_calc = crc16(buf, crcPos);
        crc = 0x0000;
        fromHex((uint8_t*) &crc, (const char*)(buf+crcPos), 2);
        crc = ntohs(crc);

        if(crc > 0 && crc != crc_calc) {
            if(debugger != NULL) {
                debugger->printf("CRC incorrrect, %04X != %04X at position %lu\n", crc, crc_calc, crcPos);
            }
            return DATA_PARSE_FOOTER_CHECKSUM_ERROR;
        }
    }
    return DATA_PARSE_OK;
}

uint16_t DSMRParser::getCrc() {
    return crc;
}
uint16_t DSMRParser::getCrcCalc() {
    return crc_calc;
}