/* Minimal Timezone stub for native builds.
 * IEC6205675 takes a Timezone* parameter; tests pass nullptr. */
#pragma once
#include <time.h>

class Timezone {
public:
    time_t toUTC(time_t t) { return t; }
    time_t toLocal(time_t t) { return t; }
};
