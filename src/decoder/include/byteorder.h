/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Portable byte-order header.
 * On ESP8266/ESP32 delegates to lwIP; on native (Linux/macOS) uses POSIX.
 */

#ifndef _BYTEORDER_H
#define _BYTEORDER_H

#if defined(ESP8266) || defined(ESP32)
  #include "lwip/def.h"
#else
  #include <arpa/inet.h>
#endif

#endif
