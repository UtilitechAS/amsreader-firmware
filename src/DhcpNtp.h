/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * @brief Asks the DHCP server for its NTP server (option 42)
 */

#ifndef _DHCPNTP_H
#define _DHCPNTP_H

#if defined(ESP32)

#include <stdint.h>

#define DHCP_NTP_IDLE 0
#define DHCP_NTP_WAITING 1
#define DHCP_NTP_FOUND 2
#define DHCP_NTP_NONE 3     // The server answered without an NTP server
#define DHCP_NTP_TIMEOUT 4
#define DHCP_NTP_FAILED 5   // No DHCP client on the interface, or nothing could be sent

/**
 * Send a DHCPINFORM for option 42 from the interface holding localIp (network
 * byte order). Returns false if a request is already in flight.
 */
bool dhcpNtpRequest(uint32_t localIp);

/**
 * Poll the request. Returns DHCP_NTP_FOUND, DHCP_NTP_NONE, DHCP_NTP_TIMEOUT or
 * DHCP_NTP_FAILED once, then DHCP_NTP_IDLE. ntpServer (network byte order) is
 * set when the result is DHCP_NTP_FOUND.
 */
uint8_t dhcpNtpPoll(uint32_t& ntpServer);

#endif
#endif
