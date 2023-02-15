#include "Uptime.h"

uint64_t millis64() {
    uint32_t new_low32 = millis();
    if (new_low32 < _uptime_last_value) _uptime_rollovers++;
    _uptime_last_value = new_low32;
    return (uint64_t) _uptime_rollovers << 32 | _uptime_last_value;
}
