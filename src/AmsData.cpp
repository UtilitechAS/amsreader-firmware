#include "AmsData.h"

AmsData::AmsData() {}

void AmsData::apply(AmsData& other) {
    if(other.getListType() < 3) {
        unsigned long ms = this->lastUpdateMillis > other.getLastUpdateMillis() ? 0 : other.getLastUpdateMillis() - this->lastUpdateMillis;

        if(ms > 0) {
            if(other.getActiveImportPower() > 0)
                activeImportCounter += (((float) ms) * other.getActiveImportPower()) / 3600000000.0;

            if(other.getListType() > 1) {
                ms = this->lastUpdateMillis > other.getLastUpdateMillis() ? 0 : other.getLastUpdateMillis() - this->lastList2;
                if(other.getActiveExportPower() > 0)
                    activeExportCounter += (((float) ms) * other.getActiveExportPower()) / 3600000000.0;
                if(other.getReactiveImportPower() > 0)
                    reactiveImportCounter += (((float) ms) * other.getReactiveImportPower()) / 3600000000.0;
                if(other.getReactiveExportPower() > 0)
                    reactiveExportCounter += (((float) ms) * other.getReactiveExportPower()) / 3600000000.0;
            }
            counterEstimated = true;
        }
    } else {
        Serial.printf("\nDeviation: %.4f\n", other.getActiveImportCounter() - activeImportCounter);
    }

    this->lastUpdateMillis = other.getLastUpdateMillis();
    if(other.getListType() > 1) {
        this->lastList2 = this->lastUpdateMillis;
    }
    this->packageTimestamp = other.getPackageTimestamp();
    if(other.getListType() > this->listType)
        this->listType = other.getListType();
    switch(other.getListType()) {
        case 3:
            this->powerFactor = other.getPowerFactor();
            this->l1PowerFactor = other.getL1PowerFactor();
            this->l2PowerFactor = other.getL2PowerFactor();
            this->l3PowerFactor = other.getL3PowerFactor();
            this->meterTimestamp = other.getMeterTimestamp();
            this->activeImportCounter = other.getActiveImportCounter();
            this->activeExportCounter = other.getActiveExportCounter();
            this->reactiveImportCounter = other.getReactiveImportCounter();
            this->reactiveExportCounter = other.getReactiveExportCounter();
            this->counterEstimated = false;
        case 2:
            this->listId = other.getListId();
            this->meterId = other.getMeterId();
            this->meterType = other.getMeterType();
            this->meterModel = other.getMeterModel();
            this->reactiveImportPower = other.getReactiveImportPower();
            this->activeExportPower = other.getActiveExportPower();
            this->reactiveExportPower = other.getReactiveExportPower();
            this->l1current = other.getL1Current();
            this->l2current = other.getL2Current();
            this->l3current = other.getL3Current();
            this->l1voltage = other.getL1Voltage();
            this->l2voltage = other.getL2Voltage();
            this->l3voltage = other.getL3Voltage();
            this->threePhase = other.isThreePhase();
            this->twoPhase = other.isTwoPhase();
        case 1:
            this->activeImportPower = other.getActiveImportPower();
    }
}

unsigned long AmsData::getLastUpdateMillis() {
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

uint16_t AmsData::getActiveImportPower() {
    return this->activeImportPower;
}

uint16_t AmsData::getReactiveImportPower() {
    return this->reactiveImportPower;
}

uint16_t AmsData::getActiveExportPower() {
    return this->activeExportPower;
}

uint16_t AmsData::getReactiveExportPower() {
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

float AmsData::getActiveImportCounter() {
    return this->activeImportCounter;
}

float AmsData::getReactiveImportCounter() {
    return this->reactiveImportCounter;
}

float AmsData::getActiveExportCounter() {
    return this->activeExportCounter;
}

float AmsData::getReactiveExportCounter() {
    return this->reactiveExportCounter;
}

bool AmsData::isThreePhase() {
    return this->threePhase;
}

bool AmsData::isTwoPhase() {
    return this->twoPhase;
}
