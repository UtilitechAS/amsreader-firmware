/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _BOUNDEDSEND_H
#define _BOUNDEDSEND_H

#include <stddef.h>
#include <stdint.h>

// Writes size bytes to a non-blocking-capable socket, giving up when timeoutMs
// has passed since the call started, whatever progress was made. Returns the
// number of bytes written. *failed is set when not everything was written,
// either because the deadline passed or because the socket reported an error;
// the stream is then unusable, since a partial packet may be on the wire.
//
// The ESP32 core's WiFiClient::write() waits up to 10 x 1 s for the socket to
// become writable and restarts that count whenever a few bytes get through, so
// on a link that trickles it blocks for minutes. lwmqtt hands every write the
// time left of its command timeout, but the Arduino glue ignores it. This is
// the write that honours it.
size_t boundedSend(int fd, const uint8_t* buf, size_t size, uint32_t timeoutMs, bool* failed);

#endif
