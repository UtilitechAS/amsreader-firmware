/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "PulseMeterCommunicator.h"
#include "Uptime.h"

#if defined(AMS_REMOTE_DEBUG)
PulseMeterCommunicator::PulseMeterCommunicator(RemoteDebug* debugger) {
#else
PulseMeterCommunicator::PulseMeterCommunicator(Stream* debugger) {
#endif
    this->debugger = debugger;
}

void PulseMeterCommunicator::configure(MeterConfig& meterConfig, Timezone* tz) {
    this->meterConfig = meterConfig;
    this->configChanged = false;
    this->tz = tz;
    setupGpio();
}

bool PulseMeterCommunicator::loop() {
    return updated || !initialized;
}

AmsData* PulseMeterCommunicator::getData(AmsData& meterState) {
    if(!initialized) {
        state.apply(meterState);
        initialized = true;
        return NULL;
    }
    updated = false;

    AmsData* ret = new AmsData();
    ret->apply(state);
    return ret;
}

int PulseMeterCommunicator::getLastError() {
    return 0;
}

bool PulseMeterCommunicator::isConfigChanged() {
    return this->configChanged;
}

void PulseMeterCommunicator::getCurrentConfig(MeterConfig& meterConfig) {
    meterConfig = this->meterConfig;
}

void PulseMeterCommunicator::setupGpio() {
    #if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::DEBUG))
#endif
debugger->printf_P(PSTR("Setting up Pulse Meter GPIO, rx: %d, tx: %d\n"), meterConfig.rxPin, meterConfig.txPin);
    if(meterConfig.rxPin != NOT_A_PIN) {
        pinMode(meterConfig.rxPin, meterConfig.rxPinPullup ? INPUT_PULLUP : INPUT);
    }
// Export counter?
//    if(meterConfig.txPin != NOT_A_PIN) {
//        pinMode(meterConfig.txPin, meterConfig.rxPinPullup ? INPUT_PULLUP : INPUT);
//    }
}

void PulseMeterCommunicator::onPulse(uint8_t pulses) {
    uint64_t now = millis64();
    if(initialized && pulses == 0) {
        if(now - lastUpdate > 10000) {
            ImpulseAmsData update(state, meterConfig.baud, pulses);
            state.apply(update);
            updated = true;
            lastUpdate = now;
        }
        return;
    }
    if(!initialized) {
        return;
    }

    ImpulseAmsData update(state, meterConfig.baud, pulses);
    state.apply(update);
    updated = true;
    lastUpdate = now;
}
