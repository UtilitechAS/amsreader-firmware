/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "KmpCommunicator.h"
#include "Uptime.h"
#include "crc.h"
#include "OBIScodes.h"

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
	uint64_t now = millis64();

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
            debugPrint(hanBuffer, 0, hanBufferSize);
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
    AmsData* data = new AmsData();
    data->apply(OBIS_ACTIVE_IMPORT_COUNT, kmpData.activeImportCounter);
    data->apply(OBIS_ACTIVE_EXPORT_COUNT, kmpData.activeExportCounter);
    data->apply(OBIS_REACTIVE_IMPORT_COUNT, kmpData.reactiveImportCounter);
    data->apply(OBIS_REACTIVE_EXPORT_COUNT, kmpData.reactiveExportCounter);
    data->apply(OBIS_ACTIVE_IMPORT, kmpData.activeImportPower);
    data->apply(OBIS_ACTIVE_EXPORT, kmpData.activeExportPower);
    data->apply(OBIS_REACTIVE_IMPORT, kmpData.reactiveImportPower);
    data->apply(OBIS_REACTIVE_EXPORT, kmpData.reactiveExportPower);
    data->apply(OBIS_VOLTAGE_L1, kmpData.l1voltage);
    data->apply(OBIS_VOLTAGE_L2, kmpData.l2voltage);
    data->apply(OBIS_VOLTAGE_L3, kmpData.l3voltage);
    data->apply(OBIS_CURRENT_L1, kmpData.l1current);
    data->apply(OBIS_CURRENT_L2, kmpData.l2current);
    data->apply(OBIS_CURRENT_L3, kmpData.l3current);
    data->apply(OBIS_POWER_FACTOR_L1, kmpData.l1PowerFactor);
    data->apply(OBIS_POWER_FACTOR_L2, kmpData.l2PowerFactor);
    data->apply(OBIS_POWER_FACTOR_L3, kmpData.l3PowerFactor);
    data->apply(OBIS_POWER_FACTOR, kmpData.powerFactor);
    data->apply(OBIS_ACTIVE_IMPORT_L1, kmpData.l1activeImportPower);
    data->apply(OBIS_ACTIVE_IMPORT_L2, kmpData.l2activeImportPower);
    data->apply(OBIS_ACTIVE_IMPORT_L3, kmpData.l3activeImportPower);
    data->apply(OBIS_ACTIVE_EXPORT_L1, kmpData.l1activeExportPower);
    data->apply(OBIS_ACTIVE_EXPORT_L2, kmpData.l2activeExportPower);
    data->apply(OBIS_ACTIVE_EXPORT_L3, kmpData.l3activeExportPower);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L1, kmpData.l1activeImportCounter);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L2, kmpData.l2activeImportCounter);
    data->apply(OBIS_ACTIVE_IMPORT_COUNT_L3, kmpData.l3activeImportCounter);
    data->apply(OBIS_METER_ID, kmpData.meterId);
    data->apply(OBIS_NULL, AmsTypeKamstrup);
    return data;
}
