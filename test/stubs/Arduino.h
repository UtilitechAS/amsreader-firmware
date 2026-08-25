/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Minimal native shim for <Arduino.h>, used only by native unit tests.
 * Provides the handful of Arduino types/macros the decoder headers expect.
 */
#ifndef _NATIVE_ARDUINO_H
#define _NATIVE_ARDUINO_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <math.h>         /* Arduino.h exposes pow/sin/cos/sqrt in global ns */
#include "WString.h"
#include "DebugPrint.h"   /* provides Print/Stream on native */

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#ifndef F
#define F(x) (x)
#endif
#ifndef PSTR
#define PSTR(x) (x)
#endif
#ifndef strcpy_P
#define strcpy_P strcpy
#endif
#ifndef strncmp_P
#define strncmp_P strncmp
#endif
#ifndef snprintf_P
#define snprintf_P snprintf
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif

typedef uint8_t byte;
typedef bool boolean;

inline void delay(unsigned long) {}
inline void pinMode(uint8_t, uint8_t) {}

class NativeESPClass {
public:
    uint32_t getChipId() const { return 0x123456; }
};

inline NativeESPClass ESP;

#endif
