/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _NTPSTATUS_H
#define _NTPSTATUS_H

#include <stdint.h>

// Registers the platform SNTP sync-notification callback. Call once during setup.
void ntpRegisterSyncCallback();

// millis64() value captured at the last successful SNTP sync, or 0 if no SNTP
// sync has happened since boot. Used to detect a clock that is set but stale
// (NTP stopped resyncing), which silently corrupts day-boundary accounting.
uint64_t ntpLastSyncMillis();

#endif
