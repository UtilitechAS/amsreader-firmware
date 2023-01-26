#ifndef _HEXUTILS_H
#define _HEXUTILS_H

#include <stdint.h>
#include "Arduino.h"

String toHex(uint8_t* in);
String toHex(uint8_t* in, uint16_t size);
void fromHex(uint8_t *out, String in, uint16_t size);
void stripNonAscii(uint8_t* in, uint16_t size);

#endif