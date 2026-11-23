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

// NTP server handed out by DHCP and in use, network byte order, 0 when the
// configured server is used. Only set on ESP32, see DhcpNtp.cpp.
void ntpSetDhcpServer(uint32_t server);
uint32_t ntpDhcpServer();

#endif
