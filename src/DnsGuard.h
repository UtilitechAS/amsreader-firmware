/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 * @brief Keeps the IPv4 DNS servers from being evicted by IPv6 ones
 */

#ifndef _DNSGUARD_H
#define _DNSGUARD_H

#if defined(ESP32)

#include <stdint.h>

/**
 * Whether an IPv6 DNS server is allowed to stay in the table at all. When IPv6
 * is disabled we have no global IPv6 address, so such a server is unreachable.
 */
void dnsGuardSetIpv6Allowed(bool allowed);

/**
 * Reconcile the lwIP DNS table with the last known good IPv4 servers. Cheap
 * enough to call from the main loop, safe to call from an event handler.
 */
void dnsGuardEnforce();

/** Number of times the table had to be repaired since boot */
uint32_t dnsGuardRepairs();

#endif
#endif
