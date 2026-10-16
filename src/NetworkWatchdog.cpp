/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#include "NetworkWatchdog.h"

NetworkWatchdog::NetworkWatchdog(ResetDataContainer* rdc) {
    this->rdc = rdc;
}

bool NetworkWatchdog::isProbeDue(uint64_t now) {
    return lastProbe == 0 || now - lastProbe >= NETWORK_WATCHDOG_PROBE_INTERVAL;
}

void NetworkWatchdog::probeStarted(uint64_t now) {
    lastProbe = now;
}

void NetworkWatchdog::probeFinished(uint64_t now, bool replied) {
    if(replied) {
        // No probes run while WiFi is down, so a restart also marks an outage
        if(armed && (failedProbes > 0 || nextRestart > NETWORK_WATCHDOG_FIRST_RESTART)) {
            recordOutage(now);
        }
        armed = true;
        lastAlive = now;
        failedProbes = 0;
        nextRestart = NETWORK_WATCHDOG_FIRST_RESTART;
        backoff = NETWORK_WATCHDOG_FIRST_RESTART;
    } else if(armed && failedProbes < 255) {
        failedProbes++;
    }
}

uint8_t NetworkWatchdog::check(uint64_t now) {
    if(!armed) return NETWORK_WATCHDOG_ACTION_NONE;

    uint64_t outage = now - lastAlive;
    if(outage >= NETWORK_WATCHDOG_REBOOT_AFTER) {
        recordOutage(now);
        return NETWORK_WATCHDOG_ACTION_REBOOT;
    }
    if(outage >= nextRestart) {
        recordOutage(now);
        if(rdc->wdRestarts < 0xFFFF) rdc->wdRestarts++;
        nextRestart = outage + backoff;
        backoff = backoff * 2 > NETWORK_WATCHDOG_MAX_BACKOFF ? NETWORK_WATCHDOG_MAX_BACKOFF : backoff * 2;
        return NETWORK_WATCHDOG_ACTION_RESTART_WIFI;
    }
    return NETWORK_WATCHDOG_ACTION_NONE;
}

bool NetworkWatchdog::isArmed() {
    return armed;
}

uint8_t NetworkWatchdog::getFailedProbes() {
    return failedProbes;
}

void NetworkWatchdog::recordOutage(uint64_t now) {
    uint32_t seconds = (now - lastAlive) / 1000;
    if(seconds > rdc->wdLongestOutage) rdc->wdLongestOutage = seconds;
}
