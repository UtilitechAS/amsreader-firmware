#include "AmsData.h"
#include "Kaifa.h"
#include "Aidon.h"
#include "Kamstrup.h"

AmsData::AmsData() {}

AmsData::AmsData(int meterType, HanReader& hanReader) {
    lastUpdateMillis = millis();
    packageTimestamp = hanReader.getPackageTime();

    int listSize = hanReader.getListSize();
    switch(meterType) {
        case METER_TYPE_KAIFA:
			extractFromKaifa(hanReader, listSize);
            break;
		case METER_TYPE_AIDON:
			extractFromAidon(hanReader, listSize);
            break;
		case METER_TYPE_KAMSTRUP:
			extractFromKamstrup(hanReader, listSize);
            break;

    }
}

void AmsData::extractFromKaifa(HanReader& hanReader, int listSize) {
    switch(listSize) {
        case (int)Kaifa::List1:
            listType = 1;
            break;
        case (int)Kaifa::List3PhaseShort:
            threePhase = true;
        case (int)Kaifa::List1PhaseShort:
            listType = 2;
            break;
        case (int)Kaifa::List3PhaseLong:
            threePhase = true;
        case (int)Kaifa::List1PhaseLong:
            listType = 3;
            break;
    }

    if(listSize == (int)Kaifa::List1) {
        activeImportPower = hanReader.getInt((int)Kaifa_List1::ActivePowerImported);
    } else {
        switch(listSize) {
            case (int)Kaifa::List3PhaseLong:
                meterTimestamp        = hanReader.getTime(         (int)Kaifa_List3Phase::MeterClock);
                activeImportCounter   = ((double) hanReader.getInt((int)Kaifa_List3Phase::CumulativeActiveImportEnergy)) / 1000;
                activeExportCounter   = ((double) hanReader.getInt((int)Kaifa_List3Phase::CumulativeActiveExportEnergy)) / 1000;
                reactiveImportCounter = ((double) hanReader.getInt((int)Kaifa_List3Phase::CumulativeReactiveImportEnergy)) / 1000;
                reactiveExportCounter = ((double) hanReader.getInt((int)Kaifa_List3Phase::CumulativeReactiveExportEnergy)) / 1000;
            case (int)Kaifa::List3PhaseShort:
                listId                = hanReader.getString(       (int)Kaifa_List3Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(       (int)Kaifa_List3Phase::MeterID);
                meterType             = hanReader.getString(       (int)Kaifa_List3Phase::MeterType);
                activeImportPower     = hanReader.getInt(          (int)Kaifa_List3Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getInt(          (int)Kaifa_List3Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getInt(          (int)Kaifa_List3Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getInt(          (int)Kaifa_List3Phase::ReactiveExportPower);
                l1current             = ((double) hanReader.getInt((int)Kaifa_List3Phase::CurrentL1)) / 1000;
                l2current             = ((double) hanReader.getInt((int)Kaifa_List3Phase::CurrentL2)) / 1000;
                l3current             = ((double) hanReader.getInt((int)Kaifa_List3Phase::CurrentL3)) / 1000;
                l1voltage             = ((double) hanReader.getInt((int)Kaifa_List3Phase::VoltageL1)) / 10;
                l2voltage             = ((double) hanReader.getInt((int)Kaifa_List3Phase::VoltageL2)) / 10;
                l3voltage             = ((double) hanReader.getInt((int)Kaifa_List3Phase::VoltageL3)) / 10;
                break;
            case (int)Kaifa::List1PhaseLong:
                meterTimestamp        = hanReader.getTime(         (int)Kaifa_List1Phase::MeterClock);
                activeImportCounter   = ((double) hanReader.getInt((int)Kaifa_List1Phase::CumulativeActiveImportEnergy));
                activeExportCounter   = ((double) hanReader.getInt((int)Kaifa_List1Phase::CumulativeActiveExportEnergy));
                reactiveImportCounter = ((double) hanReader.getInt((int)Kaifa_List1Phase::CumulativeReactiveImportEnergy));
                reactiveExportCounter = ((double) hanReader.getInt((int)Kaifa_List1Phase::CumulativeReactiveExportEnergy));
            case (int)Kaifa::List1PhaseShort:
                listId                = hanReader.getString(       (int)Kaifa_List1Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(       (int)Kaifa_List1Phase::MeterID);
                meterType             = hanReader.getString(       (int)Kaifa_List1Phase::MeterType);
                activeImportPower     = hanReader.getInt(          (int)Kaifa_List1Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getInt(          (int)Kaifa_List1Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getInt(          (int)Kaifa_List1Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getInt(          (int)Kaifa_List1Phase::ReactiveExportPower);
                l1current             = ((double) hanReader.getInt((int)Kaifa_List1Phase::CurrentL1)) / 1000;
                l1voltage             = ((double) hanReader.getInt((int)Kaifa_List1Phase::VoltageL1)) / 10;
                break;
        }
    }
}

void AmsData::extractFromAidon(HanReader& hanReader, int listSize) {
    switch(listSize) {
        case (int)Aidon::List1:
            listType = 1;
            break;
        case (int)Aidon::List3PhaseITShort:
        case (int)Aidon::List3PhaseShort:
            threePhase = true;
        case (int)Aidon::List1PhaseShort:
            listType = 2;
            break;
        case (int)Aidon::List3PhaseITLong:
        case (int)Aidon::List3PhaseLong:
            threePhase = true;
        case (int)Aidon::List1PhaseLong:
            listType = 3;
            break;
    }

    if(listSize == (int)Aidon::List1) {
        activeImportPower = hanReader.getInt((int)Aidon_List1::ActiveImportPower);
    } else {
        switch(listSize) {
            case (int)Aidon::List3PhaseLong:
                meterTimestamp        = hanReader.getTime(            (int)Aidon_List3Phase::Timestamp);
                activeImportCounter   = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CumulativeReactiveExportEnergy)) / 100;
            case (int)Aidon::List3PhaseShort:
                listId                = hanReader.getString(          (int)Aidon_List3Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(          (int)Aidon_List3Phase::MeterID);
                meterType             = hanReader.getString(          (int)Aidon_List3Phase::MeterType);
                activeImportPower     = hanReader.getInt(             (int)Aidon_List3Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getInt(             (int)Aidon_List3Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getInt(             (int)Aidon_List3Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getInt(             (int)Aidon_List3Phase::ReactiveExportPower);
                l1current             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CurrentL1)) / 10;
                l2current             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CurrentL2)) / 10;
                l3current             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::CurrentL3)) / 10;
                l1voltage             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::VoltageL1)) / 10;
                l2voltage             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::VoltageL2)) / 10;
                l3voltage             = ((double) hanReader.getInt(   (int)Aidon_List3Phase::VoltageL3)) / 10;
                break;
            case (int)Aidon::List1PhaseLong:
                meterTimestamp        = hanReader.getTime(            (int)Aidon_List1Phase::Timestamp);
                activeImportCounter   = ((double) hanReader.getInt(   (int)Aidon_List1Phase::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((double) hanReader.getInt(   (int)Aidon_List1Phase::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((double) hanReader.getInt(   (int)Aidon_List1Phase::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((double) hanReader.getInt(   (int)Aidon_List1Phase::CumulativeReactiveExportEnergy)) / 100;
            case (int)Aidon::List1PhaseShort:
                listId                = hanReader.getString(          (int)Aidon_List1Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(          (int)Aidon_List1Phase::MeterID);
                meterType             = hanReader.getString(          (int)Aidon_List1Phase::MeterType);
                activeImportPower     = hanReader.getInt(             (int)Aidon_List1Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getInt(             (int)Aidon_List1Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getInt(             (int)Aidon_List1Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getInt(             (int)Aidon_List1Phase::ReactiveExportPower);
                l1current             = ((double) hanReader.getInt(   (int)Aidon_List1Phase::CurrentL1)) / 10;
                l1voltage             = ((double) hanReader.getInt(   (int)Aidon_List1Phase::VoltageL1)) / 10;
                break;
            case (int)Aidon::List3PhaseITLong:
                meterTimestamp        = hanReader.getTime(            (int)Aidon_List3PhaseIT::Timestamp);
                activeImportCounter   = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CumulativeReactiveExportEnergy)) / 100;
            case (int)Aidon::List3PhaseITShort:
                listId                = hanReader.getString(          (int)Aidon_List3PhaseIT::ListVersionIdentifier);
                meterId               = hanReader.getString(          (int)Aidon_List3PhaseIT::MeterID);
                meterType             = hanReader.getString(          (int)Aidon_List3PhaseIT::MeterType);
                activeImportPower     = hanReader.getInt(             (int)Aidon_List3PhaseIT::ActiveImportPower);
                reactiveImportPower   = hanReader.getInt(             (int)Aidon_List3PhaseIT::ReactiveImportPower);
                activeExportPower     = hanReader.getInt(             (int)Aidon_List3PhaseIT::ActiveExportPower);
                reactiveExportPower   = hanReader.getInt(             (int)Aidon_List3PhaseIT::ReactiveExportPower);
                l1current             = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CurrentL1)) / 10;
                l2current             = 0;
                l3current             = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::CurrentL3)) / 10;
                l1voltage             = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::VoltageL1)) / 10;
                l2voltage             = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::VoltageL2)) / 10;
                l3voltage             = ((double) hanReader.getInt(   (int)Aidon_List3PhaseIT::VoltageL3)) / 10;
                //l2current             = ((activeImportPower * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
                break;
        }
    }
}

void AmsData::extractFromKamstrup(HanReader& hanReader, int listSize) {
    switch(listSize) {
        case (int)Kamstrup::List3PhaseShort:
            threePhase = true;
        case (int)Kamstrup::List1PhaseShort:
            listType = 2;
            break;
        case (int)Kamstrup::List3PhaseLong:
            threePhase = true;
        case (int)Kamstrup::List1PhaseLong:
            listType = 3;
            break;
    }

    switch(listSize) {
        case (int)Kamstrup::List3PhaseLong:
            meterTimestamp        = hanReader.getTime(         (int)Kamstrup_List3Phase::MeterClock);
            activeImportCounter   = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CumulativeReactiveExportEnergy)) / 100;
        case (int)Kamstrup::List3PhaseShort:
            listId                = hanReader.getString(       (int)Kamstrup_List3Phase::ListVersionIdentifier);
            meterId               = hanReader.getString(       (int)Kamstrup_List3Phase::MeterID);
            meterType             = hanReader.getString(       (int)Kamstrup_List3Phase::MeterType);
            activeImportPower     = hanReader.getInt(          (int)Kamstrup_List3Phase::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (int)Kamstrup_List3Phase::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (int)Kamstrup_List3Phase::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (int)Kamstrup_List3Phase::ReactiveExportPower);
            l1current             = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CurrentL1)) / 100;
            l2current             = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CurrentL2)) / 100;
            l3current             = ((double) hanReader.getInt((int)Kamstrup_List3Phase::CurrentL3)) / 100;
            l1voltage             = hanReader.getInt(          (int)Kamstrup_List3Phase::VoltageL1);
            l2voltage             = hanReader.getInt(          (int)Kamstrup_List3Phase::VoltageL2);
            l3voltage             = hanReader.getInt(          (int)Kamstrup_List3Phase::VoltageL3);
            break;
        case (int)Kamstrup::List1PhaseLong:
            meterTimestamp        = hanReader.getTime(         (int)Kamstrup_List1Phase::MeterClock);
            activeImportCounter   = ((double) hanReader.getInt((int)Kamstrup_List1Phase::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((double) hanReader.getInt((int)Kamstrup_List1Phase::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((double) hanReader.getInt((int)Kamstrup_List1Phase::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((double) hanReader.getInt((int)Kamstrup_List1Phase::CumulativeReactiveExportEnergy)) / 100;
        case (int)Kamstrup::List1PhaseShort:
            listId                = hanReader.getString(       (int)Kamstrup_List1Phase::ListVersionIdentifier);
            meterId               = hanReader.getString(       (int)Kamstrup_List1Phase::MeterID);
            meterType             = hanReader.getString(       (int)Kamstrup_List1Phase::MeterType);
            activeImportPower     = hanReader.getInt(          (int)Kamstrup_List1Phase::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (int)Kamstrup_List1Phase::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (int)Kamstrup_List1Phase::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (int)Kamstrup_List1Phase::ReactiveExportPower);
            l1current             = ((double) hanReader.getInt((int)Kamstrup_List1Phase::CurrentL1)) / 100;
            l1voltage             = hanReader.getInt(          (int)Kamstrup_List1Phase::VoltageL1);
            break;
    }
}

void AmsData::apply(AmsData& other) {
    this->lastUpdateMillis = other.getLastUpdateMillis();
    this->packageTimestamp = other.getPackageTimestamp();
    this->listType = max(this->listType, other.getListType());
    switch(other.getListType()) {
        case 3:
            this->meterTimestamp = other.getMeterTimestamp();
            this->activeImportCounter = other.getActiveImportCounter();
            this->activeExportCounter = other.getActiveExportCounter();
            this->reactiveImportCounter = other.getReactiveImportCounter();
            this->reactiveExportCounter = other.getReactiveExportCounter();
        case 2:
            this->listId = other.getListId();
            this->meterId = other.getMeterId();
            this->meterType = other.getMeterType();
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
        case 1:
            this->activeImportPower = other.getActiveImportPower();
    }
}

unsigned long AmsData::getLastUpdateMillis() {
    return this->lastUpdateMillis;
}

unsigned long AmsData::getPackageTimestamp() {
    return this->packageTimestamp;
}

int AmsData::getListType() {
    return this->listType;
}

String AmsData::getListId() {
    return this->listId;
}

String AmsData::getMeterId() {
    return this->meterId;
}

String AmsData::getMeterType() {
    return this->meterType;
}

unsigned long AmsData::getMeterTimestamp() {
    return this->meterTimestamp;
}

int AmsData::getActiveImportPower() {
    return this->activeImportPower;
}

int AmsData::getReactiveImportPower() {
    return this->reactiveImportPower;
}

int AmsData::getActiveExportPower() {
    return this->activeExportPower;
}

int AmsData::getReactiveExportPower() {
    return this->reactiveExportPower;
}

double AmsData::getL1Voltage() {
    return this->l1voltage;
}

double AmsData::getL2Voltage() {
    return this->l2voltage;
}

double AmsData::getL3Voltage() {
    return this->l3voltage;
}

double AmsData::getL1Current() {
    return this->l1current;
}

double AmsData::getL2Current() {
    return this->l2current;
}

double AmsData::getL3Current() {
    return this->l3current;
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
