/* Portable lwip/def.h stub for native (non-embedded) builds.
 * On Linux/macOS, byte-order functions come from POSIX arpa/inet.h. */
#pragma once
#include <arpa/inet.h>

/* PROGMEM is an AVR program-memory attribute; a no-op on native. OBIScodes.h
 * (which includes this header) annotates its OBIS tables with it. */
#ifndef PROGMEM
#define PROGMEM
#endif
