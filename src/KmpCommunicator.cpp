#include "KmpCommunicator.h"

KmpCommunicator::KmpCommunicator(RemoteDebug* debugger) {}
void KmpCommunicator::configure(MeterConfig& config, Timezone* tz) {}
bool KmpCommunicator::loop() { return false; }
AmsData* KmpCommunicator::getData(AmsData& meterState) { return NULL; }
int KmpCommunicator::getLastError() { return 0; }
bool KmpCommunicator::isConfigChanged() { return false; }
void KmpCommunicator::getCurrentConfig(MeterConfig& meterConfig) {}

HardwareSerial* KmpCommunicator::getHwSerial() { return NULL; }
void KmpCommunicator::rxerr(int err) {}
