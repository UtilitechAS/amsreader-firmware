#ifndef _UPTIME_H
#define _UPTIME_H

#include "Arduino.h"

static uint32_t _uptime_last_value = 0;
static uint32_t _uptime_rollovers = 0;
uint64_t millis64();

#endif
