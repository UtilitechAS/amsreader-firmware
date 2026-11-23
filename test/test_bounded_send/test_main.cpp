/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 */

#include <unity.h>

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "BoundedSend.h"

static int fds[2];

static uint32_t nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t) (ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}

void setUp(void) {
    TEST_ASSERT_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    // A small send buffer, so a stalled reader fills it quickly
    int size = 4096;
    setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    setsockopt(fds[1], SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

void tearDown(void) {
    if(fds[0] >= 0) close(fds[0]);
    if(fds[1] >= 0) close(fds[1]);
}

// A packet that fits is written in full, without waiting
static void test_writes_everything_when_peer_reads(void) {
    uint8_t buf[512];
    memset(buf, 0xA5, sizeof(buf));
    bool failed = true;
    uint32_t start = nowMs();
    size_t sent = boundedSend(fds[0], buf, sizeof(buf), 1000, &failed);
    TEST_ASSERT_EQUAL(sizeof(buf), sent);
    TEST_ASSERT_FALSE(failed);
    TEST_ASSERT_LESS_THAN(100, nowMs() - start);

    uint8_t rx[512];
    TEST_ASSERT_EQUAL(sizeof(rx), recv(fds[1], rx, sizeof(rx), MSG_WAITALL));
    TEST_ASSERT_EQUAL_MEMORY(buf, rx, sizeof(rx));
}

// A peer that never reads: give up at the deadline, not after 10 x 1 s
static void test_gives_up_at_deadline_when_peer_stalls(void) {
    std::vector<uint8_t> buf(1024 * 1024, 0x5A);
    bool failed = false;
    uint32_t start = nowMs();
    size_t sent = boundedSend(fds[0], buf.data(), buf.size(), 300, &failed);
    uint32_t elapsed = nowMs() - start;
    TEST_ASSERT_TRUE(failed);
    TEST_ASSERT_LESS_THAN(buf.size(), sent);
    TEST_ASSERT_GREATER_OR_EQUAL(290, elapsed);
    TEST_ASSERT_LESS_THAN(600, elapsed);
}

// A link that trickles keeps making progress. The ESP32 core restarts its
// wait on every bit of progress and would block until it all got through;
// the deadline must hold regardless.
static void test_gives_up_at_deadline_when_peer_trickles(void) {
    std::atomic<bool> stop(false);
    std::thread reader([&]() {
        uint8_t rx[256];
        while(!stop) {
            recv(fds[1], rx, sizeof(rx), MSG_DONTWAIT);
            usleep(20 * 1000);
        }
    });
    std::vector<uint8_t> buf(1024 * 1024, 0x3C);
    bool failed = false;
    uint32_t start = nowMs();
    size_t sent = boundedSend(fds[0], buf.data(), buf.size(), 500, &failed);
    uint32_t elapsed = nowMs() - start;
    stop = true;
    reader.join();
    TEST_ASSERT_TRUE(failed);
    TEST_ASSERT_GREATER_THAN(0, sent);
    TEST_ASSERT_LESS_THAN(buf.size(), sent);
    TEST_ASSERT_GREATER_OR_EQUAL(490, elapsed);
    TEST_ASSERT_LESS_THAN(800, elapsed);
}

// A closed connection fails at once
static void test_fails_at_once_when_peer_closed(void) {
    close(fds[1]);
    fds[1] = -1;
    uint8_t buf[64] = {0};
    bool failed = false;
    uint32_t start = nowMs();
    size_t sent = boundedSend(fds[0], buf, sizeof(buf), 1000, &failed);
    TEST_ASSERT_TRUE(failed);
    TEST_ASSERT_EQUAL(0, sent);
    TEST_ASSERT_LESS_THAN(100, nowMs() - start);
}

static void test_fails_on_invalid_socket(void) {
    uint8_t buf[8] = {0};
    bool failed = false;
    TEST_ASSERT_EQUAL(0, boundedSend(-1, buf, sizeof(buf), 1000, &failed));
    TEST_ASSERT_TRUE(failed);
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    UNITY_BEGIN();
    RUN_TEST(test_writes_everything_when_peer_reads);
    RUN_TEST(test_gives_up_at_deadline_when_peer_stalls);
    RUN_TEST(test_gives_up_at_deadline_when_peer_trickles);
    RUN_TEST(test_fails_at_once_when_peer_closed);
    RUN_TEST(test_fails_on_invalid_socket);
    return UNITY_END();
}
