/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "AmsData.h"

AmsData::AmsData() {}

void AmsData::apply(AmsData& other) {
    if(other.getListType() < 3) {
        unsigned long ms = this->lastUpdateMillis > other.getLastUpdateMillis() ? 0 : other.getLastUpdateMillis() - this->lastUpdateMillis;

        if(ms > 0) {
            if(other.getActiveImportPower() > 0) {
                uint32_t power = (activeImportPower + other.getActiveImportPower()) / 2;
                float add = power * (((float) ms) / 3600000.0);
                activeImportCounter += add / 1000.0;
                //Serial.printf("%dW, %dms, %.6fkWh added\n", other.getActiveImportPower(), ms, add);
            }

            if(other.getListType() > 1) {
                ms = this->lastList2 > other.getLastUpdateMillis() ? 0 : other.getLastUpdateMillis() - this->lastList2;
                if(other.getActiveExportPower() > 0) {
                    uint32_t power = (activeExportPower + other.getActiveExportPower()) / 2;
                    float add = power * (((float) ms) / 3600000.0);
                    activeExportCounter += add / 1000.0;
                }
                if(other.getReactiveImportPower() > 0) {
                    uint32_t power = (reactiveImportPower + other.getReactiveImportPower()) / 2;
                    float add = power * (((float) ms) / 3600000.0);
                    reactiveImportCounter += add / 1000.0;
                }
                if(other.getReactiveExportPower() > 0) {
                    uint32_t power = (reactiveExportPower + other.getReactiveExportPower()) / 2;
                    float add = power * (((float) ms) / 3600000.0);
                    reactiveExportCounter += add / 1000.0;
                }
            }
            counterEstimated = true;
        }
    }

    this->lastUpdateMillis = other.getLastUpdateMillis();
    if(other.getListType() > 1) {
        this->lastList2 = this->lastUpdateMillis;
    }
    this->packageTimestamp = other.getPackageTimestamp();
    if(other.getListType() > this->listType)
        this->listType = other.getListType();
    switch(other.getListType()) {
        case 4:
            this->powerFactor = other.getPowerFactor();
            this->l1PowerFactor = other.getL1PowerFactor();
            this->l2PowerFactor = other.getL2PowerFactor();
            this->l3PowerFactor = other.getL3PowerFactor();
            this->l1activeImportPower = other.getL1ActiveImportPower();
            this->l2activeImportPower = other.getL2ActiveImportPower();
            this->l3activeImportPower = other.getL3ActiveImportPower();
            this->l1activeExportPower = other.getL1ActiveExportPower();
            this->l2activeExportPower = other.getL2ActiveExportPower();
            this->l3activeExportPower = other.getL3ActiveExportPower();
            this->l1activeImportCounter = other.getL1ActiveImportCounter();
            this->l2activeImportCounter = other.getL2ActiveImportCounter();
            this->l3activeImportCounter = other.getL3ActiveImportCounter();
            this->l1activeExportCounter = other.getL1ActiveExportCounter();
            this->l2activeExportCounter = other.getL2ActiveExportCounter();
            this->l3activeExportCounter = other.getL3ActiveExportCounter();
        case 3:
            this->meterTimestamp = other.getMeterTimestamp();
            // Aidon tends to sometime send the same counter as last hour by accident
            if(meterType == AmsTypeAidon && counterEstimated && lastKnownCounter == other.getActiveImportCounter()-other.getActiveExportCounter()) {
                double diff = activeImportCounter - activeExportCounter - lastKnownCounter;
                if(diff < 1.0) { // In case a very low value have been calculated, use the new values
                    this->activeImportCounter = other.getActiveImportCounter();
                    this->activeExportCounter = other.getActiveExportCounter();
                    this->reactiveImportCounter = other.getReactiveImportCounter();
                    this->reactiveExportCounter = other.getReactiveExportCounter();
                    this->lastKnownCounter = activeImportCounter - activeExportCounter;
                }
            } else {
                this->activeImportCounter = other.getActiveImportCounter();
                this->activeExportCounter = other.getActiveExportCounter();
                this->reactiveImportCounter = other.getReactiveImportCounter();
                this->reactiveExportCounter = other.getReactiveExportCounter();
                this->lastKnownCounter = activeImportCounter - activeExportCounter;
            }
            this->counterEstimated = false;
        case 2:
            this->listId = other.getListId();
            this->meterId = other.getMeterId();
            this->meterType = other.getMeterType();
            this->meterModel = other.getMeterModel();
            this->reactiveImportPower = other.getReactiveImportPower();
            this->reactiveExportPower = other.getReactiveExportPower();
            this->l1current = other.getL1Current();
            this->l2current = other.getL2Current();
            this->l2currentMissing = other.isL2currentMissing();
            this->l3current = other.getL3Current();
            this->l1voltage = other.getL1Voltage();
            this->l2voltage = other.getL2Voltage();
            this->l3voltage = other.getL3Voltage();
            this->threePhase = other.isThreePhase();
            this->twoPhase = other.isTwoPhase();
    }

    // Moved outside switch to handle meters alternating between sending active and accumulated values
    if(other.getListType() == 1 || (other.getActiveImportPower() > 0 || other.getActiveExportPower() > 0))
        this->activeImportPower = other.getActiveImportPower();
    if(other.getListType() == 2 || (other.getActiveImportPower() > 0 || other.getActiveExportPower() > 0))
        this->activeExportPower = other.getActiveExportPower();
}

void AmsData::apply(OBIS_code_t obis, double value) {
    if(obis.sensor == 0 && obis.gr == 0 && obis.tariff == 0) {
        meterType = value;
    }
    if(obis.gr == 1) {
        if(obis.sensor == 96) {
            if(obis.tariff == 0) {
                meterId = String((long) value, 10);
                return;
            } else if(obis.tariff == 1) {
                return;
            }
        }
    }
    if(obis.tariff != 0) {
        Serial.println("Tariff not implemented");
        return;
    }
    if(obis.gr == 7) { // Instant values
        switch(obis.sensor) {
            case 1:
                activeImportPower = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 2:
                activeExportPower = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 3:
                reactiveImportPower = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 4:
                reactiveExportPower = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 13:
                powerFactor = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 21:
                l1activeImportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 22:
                l1activeExportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 31:
                l1current = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 32:
                l1voltage = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 33:
                l1PowerFactor = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 41:
                l2activeImportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 42:
                l2activeExportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 51:
                l2current = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 52:
                l2voltage = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 53:
                l2PowerFactor = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 61:
                l3activeImportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 62:
                l3activeExportPower = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 71:
                l3current = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 72:
                l3voltage = value;
                listType = max(listType, (uint8_t) 2);
                break;
            case 73:
                l3PowerFactor = value;
                listType = max(listType, (uint8_t) 4);
                break;
        }
    } else if(obis.gr == 8) { // Accumulated values
        switch(obis.sensor) {
            case 1:
                activeImportCounter = value;
                listType = max(listType, (uint8_t) 3);
                break;
            case 2:
                activeExportCounter = value;
                listType = max(listType, (uint8_t) 3);
                break;
            case 3:
                reactiveImportCounter = value;
                listType = max(listType, (uint8_t) 3);
                break;
            case 4:
                reactiveExportCounter = value;
                listType = max(listType, (uint8_t) 3);
                break;
            case 21:
                l1activeImportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 22:
                l1activeExportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 41:
                l2activeImportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 42:
                l2activeExportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 61:
                l3activeImportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
            case 62:
                l3activeExportCounter = value;
                listType = max(listType, (uint8_t) 4);
                break;
        }
    }
    if(listType > 0)
        lastUpdateMillis = millis();
}

uint64_t AmsData::getLastUpdateMillis() {
    return this->lastUpdateMillis;
}

time_t AmsData::getPackageTimestamp() {
    return this->packageTimestamp;
}

uint8_t AmsData::getListType() {
    return this->listType;
}

String AmsData::getListId() {
    return this->listId;
}

String AmsData::getMeterId() {
    return this->meterId;
}

uint8_t AmsData::getMeterType() {
    return this->meterType;
}

String AmsData::getMeterModel() {
    return this->meterModel;
}

time_t AmsData::getMeterTimestamp() {
    return this->meterTimestamp;
}

uint32_t AmsData::getActiveImportPower() {
    return this->activeImportPower;
}

uint32_t AmsData::getReactiveImportPower() {
    return this->reactiveImportPower;
}

uint32_t AmsData::getActiveExportPower() {
    return this->activeExportPower;
}

uint32_t AmsData::getReactiveExportPower() {
    return this->reactiveExportPower;
}

float AmsData::getL1Voltage() {
    return this->l1voltage;
}

float AmsData::getL2Voltage() {
    return this->l2voltage;
}

float AmsData::getL3Voltage() {
    return this->l3voltage;
}

float AmsData::getL1Current() {
    return this->l1current;
}

float AmsData::getL2Current() {
    return this->l2current;
}

float AmsData::getL3Current() {
    return this->l3current;
}

float AmsData::getPowerFactor() {
    return this->powerFactor;
}

float AmsData::getL1PowerFactor() {
    return this->l1PowerFactor;
}

float AmsData::getL2PowerFactor() {
    return this->l2PowerFactor;
}

float AmsData::getL3PowerFactor() {
    return this->l3PowerFactor;
}

uint32_t AmsData::getL1ActiveImportPower() {
    return this->l1activeImportPower;
}

uint32_t AmsData::getL2ActiveImportPower() {
    return this->l2activeImportPower;
}

uint32_t AmsData::getL3ActiveImportPower() {
    return this->l3activeImportPower;
}

uint32_t AmsData::getL1ActiveExportPower() {
    return this->l1activeExportPower;
}

uint32_t AmsData::getL2ActiveExportPower() {
    return this->l2activeExportPower;
}

uint32_t AmsData::getL3ActiveExportPower() {
    return this->l3activeExportPower;
}

double AmsData::getL1ActiveImportCounter() {
    return this->l1activeImportCounter;
}

double AmsData::getL2ActiveImportCounter() {
    return this->l2activeImportCounter;
}

double AmsData::getL3ActiveImportCounter() {
    return this->l3activeImportCounter;
}

double AmsData::getL1ActiveExportCounter() {
    return this->l1activeExportCounter;
}

double AmsData::getL2ActiveExportCounter() {
    return this->l2activeExportCounter;
}

double AmsData::getL3ActiveExportCounter() {
    return this->l3activeExportCounter;
}

double AmsData::getActiveImportCounter() {
    return this->activeImportCounter;
}

double AmsData::getReactiveImportCounter() {
    return this->reactiveImportCounter;
}

double AmsData::getActiveExportCounter() {
    return this->activeExportCounter;
}

double AmsData::getReactiveExportCounter() {
    return this->reactiveExportCounter;
}

bool AmsData::isThreePhase() {
    return this->threePhase;
}

bool AmsData::isTwoPhase() {
    return this->twoPhase;
}

bool AmsData::isCounterEstimated() {
    return this->counterEstimated;
}

bool AmsData::isL2currentMissing() {
    return this->l2currentMissing;
}

int8_t AmsData::getLastError() {
    return lastErrorCount > 2 ? lastError : 0;
}

void AmsData::setLastError(int8_t lastError) {
    this->lastError = lastError;
    if(lastError == 0) {
        lastErrorCount = 0;
    } else {
        lastErrorCount++;
    }
}