/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * @brief Detects a WiFi connection that is up but no longer carries traffic
 *
 * @details The WiFi driver reports the connection as up as long as it keeps
 * hearing the AP, even if nothing we send gets through. The gateway is pinged
 * at a fixed interval, and when it stops answering the WiFi is restarted with
 * a growing backoff, and finally the device is rebooted.
 *
 * The watchdog is only armed once the gateway has answered, so a gateway that
 * does not answer ping, or a network that never came up after boot, never
 * leads to restarts or reboot loops.
 *
 * This class only holds the policy. Pinging and acting on the outcome is left
 * to the caller, which keeps it testable on native.
 */

#ifndef _NETWORKWATCHDOG_H
#define _NETWORKWATCHDOG_H

#include <stdint.h>
#include "AmsConfiguration.h"

#define NETWORK_WATCHDOG_PROBE_INTERVAL 60000
#define NETWORK_WATCHDOG_FIRST_RESTART 180000
#define NETWORK_WATCHDOG_MAX_BACKOFF 1440000
#define NETWORK_WATCHDOG_REBOOT_AFTER 3600000

#define NETWORK_WATCHDOG_ACTION_NONE 0
#define NETWORK_WATCHDOG_ACTION_RESTART_WIFI 1
#define NETWORK_WATCHDOG_ACTION_REBOOT 2

class NetworkWatchdog {
public:
    NetworkWatchdog(ResetDataContainer* rdc);

    bool isProbeDue(uint64_t now);
    void probeStarted(uint64_t now);
    void probeFinished(uint64_t now, bool replied);

    /** What the caller should do now, if anything */
    uint8_t check(uint64_t now);

    bool isArmed();
    uint8_t getFailedProbes();

private:
    ResetDataContainer* rdc;
    bool armed = false;
    uint8_t failedProbes = 0;
    uint64_t lastProbe = 0;
    uint64_t lastAlive = 0;
    uint64_t nextRestart = NETWORK_WATCHDOG_FIRST_RESTART;
    uint64_t backoff = NETWORK_WATCHDOG_FIRST_RESTART;

    void recordOutage(uint64_t now);
};

#endif
