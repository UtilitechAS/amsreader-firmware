/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#include "BoundedSend.h"

// ESP8266 has no lwIP socket API; its WiFiClient write is bounded by the client timeout
#if defined(NATIVE_TEST) || defined(ESP32)
#include <errno.h>

#if defined(NATIVE_TEST)
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>

static uint32_t nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t) (ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}
#else
#include <Arduino.h>
#include <lwip/sockets.h>

static uint32_t nowMs() {
    return millis();
}
#endif

#if defined(MSG_NOSIGNAL)
#define BOUNDED_SEND_FLAGS (MSG_DONTWAIT | MSG_NOSIGNAL)
#else
#define BOUNDED_SEND_FLAGS MSG_DONTWAIT
#endif

size_t boundedSend(int fd, const uint8_t* buf, size_t size, uint32_t timeoutMs, bool* failed) {
    *failed = false;
    if(fd < 0) {
        *failed = true;
        return 0;
    }

    uint32_t start = nowMs();
    size_t sent = 0;
    while(sent < size) {
        uint32_t elapsed = nowMs() - start;
        if(elapsed >= timeoutMs) break;
        uint32_t remaining = timeoutMs - elapsed;

        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        struct timeval tv;
        tv.tv_sec = remaining / 1000;
        tv.tv_usec = (remaining % 1000) * 1000;
        int ready = select(fd + 1, NULL, &set, NULL, &tv);
        if(ready < 0) {
            if(errno == EINTR) continue;
            break;
        }
        if(ready == 0) break; // Deadline passed while waiting

        int res = send(fd, buf + sent, size - sent, BOUNDED_SEND_FLAGS);
        if(res > 0) {
            sent += res;
        } else if(res < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            break;
        }
    }
    *failed = sent < size;
    return sent;
}

#endif
