/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#ifndef _AMSDATA_H
#define _AMSDATA_H

#include <WString.h>
#include "OBIScodes.h"

enum AmsType {
    AmsTypeAutodetect = 0x00,
    AmsTypeAidon = 0x01,
    AmsTypeKaifa = 0x02,
    AmsTypeKamstrup = 0x03,
    AmsTypeIskra = 0x08,
    AmsTypeLandisGyr = 0x09,
    AmsTypeSagemcom = 0x0A,
    AmsTypeCustom = 0x88,
    AmsTypeUnknown = 0xFF
};

class AmsData {
public:
    AmsData();

    void apply(AmsData& other);
    void apply(const OBIS_code_t obis, double value, uint64_t millis64);

    // True when other's List >= 3 accumulated registers are a verbatim repeat of
    // the last set we accepted: same meter clock AND same counters. Some meters
    // (confirmed on Aidon, #1119) publish the previous hour's register snapshot
    // again at the next whole hour, which would otherwise store a zero hour
    // followed by a double hour. Pure query - apply() does the bookkeeping.
    bool isStaleCounter(AmsData& other);

    uint64_t getLastUpdateMillis();

    time_t getPackageTimestamp();

    uint8_t getListType();

    String getListId();
    String getMeterId();
    uint8_t getMeterType();
    String getMeterModel();

    time_t getMeterTimestamp();

    uint32_t getActiveImportPower();
    uint32_t getReactiveImportPower();
    uint32_t getActiveExportPower();
    uint32_t getReactiveExportPower();

    float getL1Voltage();
    float getL2Voltage();
    float getL3Voltage();

    float getL1Current();
    float getL2Current();
    float getL3Current();

    float getPowerFactor();
    float getL1PowerFactor();
    float getL2PowerFactor();
    float getL3PowerFactor();

    uint32_t getL1ActiveImportPower();
    uint32_t getL2ActiveImportPower();
    uint32_t getL3ActiveImportPower();

    uint32_t getL1ActiveExportPower();
    uint32_t getL2ActiveExportPower();
    uint32_t getL3ActiveExportPower();

    double getL1ActiveImportCounter();
    double getL2ActiveImportCounter();
    double getL3ActiveImportCounter();

    double getL1ActiveExportCounter();
    double getL2ActiveExportCounter();
    double getL3ActiveExportCounter();

    double getActiveImportCounter();
    double getActiveImportCounterTariff1();
    double getActiveImportCounterTariff2();
    double getReactiveImportCounter();
    double getActiveExportCounter();
    double getActiveExportCounterTariff1();
    double getActiveExportCounterTariff2();
    double getReactiveExportCounter();

    // Whether the most recently applied packet was a stale whole-hour repeat.
    // Set from isStaleCounter() by the pipeline entry point, cleared again by the
    // next sub-hour packet, and consulted by the MQTT handlers to withhold the
    // repeated accumulated registers.
    bool isCounterStale();
    void setCounterStale(bool);

    bool isThreePhase();
    bool isTwoPhase();
    bool isCounterEstimated();
    bool isL2currentMissing();

    int8_t getLastError();
    void setLastError(int8_t);


protected:
    uint64_t lastUpdateMillis = 0;
    uint64_t lastList2 = 0;
    uint8_t listType = 0, meterType = AmsTypeUnknown;
    time_t packageTimestamp = 0;
    char listId[32] = {};
    char meterId[32] = {};
    char meterModel[65] = {};
    time_t meterTimestamp = 0;
    uint32_t activeImportPower = 0, reactiveImportPower = 0, activeExportPower = 0, reactiveExportPower = 0;
    float l1voltage = 0, l2voltage = 0, l3voltage = 0, l1current = 0, l2current = 0, l3current = 0;
    uint32_t l1activeImportPower = 0, l2activeImportPower = 0, l3activeImportPower = 0;
    uint32_t l1activeExportPower = 0, l2activeExportPower = 0, l3activeExportPower = 0;
    double l1activeImportCounter = 0, l2activeImportCounter = 0, l3activeImportCounter = 0;
    double l1activeExportCounter = 0, l2activeExportCounter = 0, l3activeExportCounter = 0;
    float powerFactor = 0, l1PowerFactor = 0, l2PowerFactor = 0, l3PowerFactor = 0;
    double activeImportCounter = 0, activeImportCounterTariff1 = 0, activeImportCounterTariff2 = 0, reactiveImportCounter = 0, activeExportCounter = 0, activeExportCounterTariff1 = 0, activeExportCounterTariff2 = 0, reactiveExportCounter = 0;
    double lastKnownCounter = 0;
    time_t lastAcceptedMeterTimestamp = 0;
    int32_t lastAcceptedMeterTimestampStep = 0;
    uint8_t staleCounterCount = 0;
    bool threePhase = false, twoPhase = false, counterEstimated = false, l2currentMissing = false;;
    bool counterStale = false;

    int8_t lastError = 0x00;
    uint8_t lastErrorCount = 0;
};

#endif
