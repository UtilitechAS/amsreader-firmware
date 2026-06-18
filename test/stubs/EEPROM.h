/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Native shim for <EEPROM.h>. AmsConfiguration.h includes it, but the native
 * decode path never touches persistent storage (AmsConfiguration.cpp is not
 * compiled for the native env), so this only needs to exist.
 */
#ifndef _NATIVE_EEPROM_H
#define _NATIVE_EEPROM_H
#endif
