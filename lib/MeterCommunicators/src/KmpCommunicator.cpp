/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "KmpCommunicator.h"
#include "Uptime.h"
#include "crc.h"
#include "OBIScodes.h"
#include "hexutils.h"

void KmpCommunicator::configure(MeterConfig& meterConfig) {
    this->meterConfig = meterConfig;
    this->configChanged = false;
    if(meterConfig.baud == 0) {
        this->configChanged = true;
        meterConfig.baud = 9600;
        meterConfig.parity = 7;
        meterConfig.invert = false;
    }
    setupHanPort(meterConfig.baud, meterConfig.parity, meterConfig.invert, false);
    talker = new KmpTalker(hanSerial, hanBuffer, hanBufferSize);
}

bool KmpCommunicator::loop() {
    bool ret = talker->loop();
    int lastError = getLastError();
    if(ret) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::VERBOSE))
        #endif
        debugger->printf_P(PSTR("Successful loop\n"));
        Serial.flush();
    } else if(lastError < 0 && lastError != DATA_PARSE_INCOMPLETE) {
		#if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
        debugger->printf_P(PSTR("Error code: %d\n"), getLastError());
		#if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::VERBOSE))
        #endif
        {
            debugger->printf_P(PSTR("  payload:\n"));
            debugPrint(hanBuffer, 0, hanBufferSize, debugger);
        }
    }
    return ret;
}

int KmpCommunicator::getLastError() {
    return talker == NULL ? DATA_PARSE_FAIL : talker->getLastError();
}

AmsData* KmpCommunicator::getData(AmsData& meterState) { 
    if(talker == NULL) return NULL;
    KmpDataHolder kmpData;
    talker->getData(kmpData);
	uint64_t now = millis64();
    AmsData* data = new AmsData();
    data->apply(OBIS_ACTIVE_IMPORT_COUNT, kmpData.activeImportCounter, now);
    data->apply(OBIS_ACTIVE_EXPORT_COUNT, kmpData.activeExportCounter, now);
    data->apply(OBIS_REACTIVE_IMPORT_COUNT, kmpData.reactiveImportCounter, now);
    data->apply(OBIS_REACTIVE_EXPORT_COUNT, kmpData.reactiveExportCounter, now);
    data->apply(OBIS_ACTIVE_IMPORT, kmpData.activeImportPower, now);
    data->apply(OBIS_ACTIVE_EXPORT, kmpData.activeExportPower, now);
    data->apply(OBIS_REACTIVE_IMPORT, kmpData.reactiveImportPower, now);
    data->apply(OBIS_REACTIVE_EXPORT, kmpData.reactiveExportPower, now);
    data->apply(OBIS_VOLTAGE_L1, kmpData.l1voltage, now);
    data->apply(OBIS_VOLTAGE_L2, kmpData.l2voltage, now);
    data->apply(OBIS_VOLTAGE_L3, kmpData.l3voltage, now);
    data->apply(OBIS_CURRENT_L1, kmpData.l1current, now);
    data->apply(OBIS_CURRENT_L2, kmpData.l2current, now);
    data->apply(OBIS_CURRENT_L3, kmpData.l3current, now);
    data->apply(OBIS_POWER_FACTOR_L1, kmpData.l1PowerFactor, now);
    data->apply(OBIS_POWER_FACTOR_L2, kmpData.l2PowerFactor, now);
    data->apply(OBIS_POWER_FACTOR_L3, kmpData.l3PowerFactor, now);
    data->apply(OBIS_POWER_FACTOR, kmpData.powerFactor, now);
    data->apply(OBIS_ACTIVE_IMPORT_L1, kmpData.l1activeImportPower, now);
    data->apply(OBIS_ACTIVE_IMPORT_L2, kmpData.l2activeImportPower, now);
    data->apply(OBIS_ACTIVE_IMPORT_L3, kmpData.l3activeImportPower, now);
    data->apply(OBIS_ACTIVE_EXPORT_L1, kmpData.l1activeExportPower, now);
    data->apply(OBIS_ACTIVE_EXPORT_L2, kmpData.l2activeExportPower, now);
    data->apply(OBIS_ACTIVE_EXPORT_L3, kmpData.l3activeExportPower, now);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L1, kmpData.l1activeImportCounter, now);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L2, kmpData.l2activeImportCounter, now);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L3, kmpData.l3activeImportCounter, now);
    data->apply(OBIS_METER_ID, kmpData.meterId, now);
    data->apply(OBIS_NULL, AmsTypeKamstrup, now);
    return data;
}
