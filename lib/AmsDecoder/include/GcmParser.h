/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _GCMPARSER_H
#define _GCMPARSER_H

#include "Arduino.h"
#include "DataParser.h"

#define GCM_TAG 0xDB
#define GCM_AUTH_FAILED -51
#define GCM_DECRYPT_FAILED -52
#define GCM_ENCRYPTION_KEY_FAILED -53

class GCMParser {
public:
    GCMParser(uint8_t *encryption_key, uint8_t *authentication_key);
    int8_t parse(uint8_t *buf, DataParserContext &ctx, bool hastag = true);
private:
    uint8_t encryption_key[16];
    uint8_t authentication_key[16];
};

#endif
