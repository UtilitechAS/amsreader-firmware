#include "AmsData.h"
#include "Kaifa.h"
#include "Aidon.h"
#include "Kamstrup.h"
#include "Omnipower.h"

AmsData::AmsData() {}

AmsData::AmsData(uint8_t meterType, bool substituteMissing, HanReader& hanReader) {
    lastUpdateMillis = millis();
    packageTimestamp = hanReader.getPackageTime(true, true);

    int listSize = hanReader.getListSize();
    switch(meterType) {
        case METER_TYPE_KAIFA:
			extractFromKaifa(hanReader, listSize);
            break;
		case METER_TYPE_AIDON:
        case METER_TYPE_RJ12:
        case METER_TYPE_RJ12_INV:
			extractFromAidon(hanReader, listSize, substituteMissing);
            break;
		case METER_TYPE_KAMSTRUP:
			extractFromKamstrup(hanReader, listSize, substituteMissing);
            break;
        case METER_TYPE_OMNIPOWER:
            extractFromOmnipower(hanReader, listSize, substituteMissing);
            break;
    }
}

void AmsData::extractFromKaifa(HanReader& hanReader, uint8_t listSize) {
    switch(listSize) {
        case (uint8_t)Kaifa::List1:
            listType = 1;
            break;
        case (uint8_t)Kaifa::List3PhaseShort:
            threePhase = true;
        case (uint8_t)Kaifa::List1PhaseShort:
            listType = 2;
            break;
        case (uint8_t)Kaifa::List3PhaseLong:
            threePhase = true;
        case (uint8_t)Kaifa::List1PhaseLong:
            listType = 3;
            break;
    }

    if(listSize == (uint8_t)Kaifa::List1) {
        activeImportPower = hanReader.getInt((int)Kaifa_List1::ActivePowerImported);
    } else {
        switch(listSize) {
            case (uint8_t)Kaifa::List3PhaseLong:
                meterTimestamp        = hanReader.getTime(          (int)Kaifa_List3Phase::MeterClock, false, false);
                activeImportCounter   = ((float) hanReader.getUint((int)Kaifa_List3Phase::CumulativeActiveImportEnergy)) / 1000;
                activeExportCounter   = ((float) hanReader.getUint((int)Kaifa_List3Phase::CumulativeActiveExportEnergy)) / 1000;
                reactiveImportCounter = ((float) hanReader.getUint((int)Kaifa_List3Phase::CumulativeReactiveImportEnergy)) / 1000;
                reactiveExportCounter = ((float) hanReader.getUint((int)Kaifa_List3Phase::CumulativeReactiveExportEnergy)) / 1000;
            case (uint8_t)Kaifa::List3PhaseShort:
                listId                = hanReader.getString(        (int)Kaifa_List3Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(        (int)Kaifa_List3Phase::MeterID);
                meterModel            = hanReader.getString(        (int)Kaifa_List3Phase::MeterType);
                activeImportPower     = hanReader.getUint(          (int)Kaifa_List3Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(          (int)Kaifa_List3Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(          (int)Kaifa_List3Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(          (int)Kaifa_List3Phase::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::CurrentL1)) / 1000;
                l2current             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::CurrentL2)) / 1000;
                l3current             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::CurrentL3)) / 1000;
                l1voltage             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::VoltageL1)) / 10;
                l2voltage             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::VoltageL2)) / 10;
                l3voltage             = ((float) hanReader.getInt( (int)Kaifa_List3Phase::VoltageL3)) / 10;
                break;
            case (uint8_t)Kaifa::List1PhaseLong:
                meterTimestamp        = hanReader.getTime(          (int)Kaifa_List1Phase::MeterClock, false, false);
                activeImportCounter   = ((float) hanReader.getUint((int)Kaifa_List1Phase::CumulativeActiveImportEnergy));
                activeExportCounter   = ((float) hanReader.getUint((int)Kaifa_List1Phase::CumulativeActiveExportEnergy));
                reactiveImportCounter = ((float) hanReader.getUint((int)Kaifa_List1Phase::CumulativeReactiveImportEnergy));
                reactiveExportCounter = ((float) hanReader.getUint((int)Kaifa_List1Phase::CumulativeReactiveExportEnergy));
            case (uint8_t)Kaifa::List1PhaseShort:
                listId                = hanReader.getString(        (int)Kaifa_List1Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(        (int)Kaifa_List1Phase::MeterID);
                meterModel            = hanReader.getString(        (int)Kaifa_List1Phase::MeterType);
                activeImportPower     = hanReader.getUint(          (int)Kaifa_List1Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(          (int)Kaifa_List1Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(          (int)Kaifa_List1Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(          (int)Kaifa_List1Phase::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt( (int)Kaifa_List1Phase::CurrentL1)) / 1000;
                l1voltage             = ((float) hanReader.getInt( (int)Kaifa_List1Phase::VoltageL1)) / 10;
                break;
        }
    }
}

void AmsData::extractFromAidon(HanReader& hanReader, uint8_t listSize, bool substituteMissing) {
    switch(listSize) {
        case (uint8_t)Aidon::List1:
            listType = 1;
            break;
        case (uint8_t)Aidon::List3PhaseITShort:
        case (uint8_t)Aidon::List3PhaseShort:
            threePhase = true;
        case (uint8_t)Aidon::List1PhaseShort:
            listType = 2;
            break;
        case (uint8_t)Aidon::List3PhaseITLong:
        case (uint8_t)Aidon::List3PhaseLong:
        case (uint8_t)Aidon::Swedish3p:
            threePhase = true;
        case (uint8_t)Aidon::List1PhaseLong:
        case (uint8_t)Aidon::Swedish1p:
            listType = 3;
            break;
    }

    if(listSize == (uint8_t)Aidon::List1) {
        activeImportPower = hanReader.getUint((uint8_t)Aidon_List1::ActiveImportPower);
    } else {
        switch(listSize) {
            case (uint8_t)Aidon::List3PhaseLong:
                meterTimestamp        = hanReader.getTime(            (uint8_t)Aidon_List3Phase::Timestamp, false, false);
                activeImportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List3Phase::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List3Phase::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List3Phase::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List3Phase::CumulativeReactiveExportEnergy)) / 100;
            case (uint8_t)Aidon::List3PhaseShort:
                listId                = hanReader.getString(          (uint8_t)Aidon_List3Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(          (uint8_t)Aidon_List3Phase::MeterID);
                meterModel            = hanReader.getString(          (uint8_t)Aidon_List3Phase::MeterType);
                activeImportPower     = hanReader.getUint(            (uint8_t)Aidon_List3Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(            (uint8_t)Aidon_List3Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(            (uint8_t)Aidon_List3Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(            (uint8_t)Aidon_List3Phase::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::CurrentL1)) / 10;
                l2current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::CurrentL2)) / 10;
                l3current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::CurrentL3)) / 10;
                l1voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::VoltageL1)) / 10;
                l2voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::VoltageL2)) / 10;
                l3voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3Phase::VoltageL3)) / 10;
                break;
            case (uint8_t)Aidon::List1PhaseLong:
                meterTimestamp        = hanReader.getTime(            (uint8_t)Aidon_List1Phase::Timestamp, false, false);
                activeImportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List1Phase::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List1Phase::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List1Phase::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List1Phase::CumulativeReactiveExportEnergy)) / 100;
            case (uint8_t)Aidon::List1PhaseShort:
                listId                = hanReader.getString(          (uint8_t)Aidon_List1Phase::ListVersionIdentifier);
                meterId               = hanReader.getString(          (uint8_t)Aidon_List1Phase::MeterID);
                meterModel            = hanReader.getString(          (uint8_t)Aidon_List1Phase::MeterType);
                activeImportPower     = hanReader.getUint(            (uint8_t)Aidon_List1Phase::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(            (uint8_t)Aidon_List1Phase::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(            (uint8_t)Aidon_List1Phase::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(            (uint8_t)Aidon_List1Phase::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List1Phase::CurrentL1)) / 10;
                l1voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List1Phase::VoltageL1)) / 10;
                break;
            case (uint8_t)Aidon::List3PhaseITLong:
                meterTimestamp        = hanReader.getTime(            (uint8_t)Aidon_List3PhaseIT::Timestamp, false, false);
                activeImportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List3PhaseIT::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_List3PhaseIT::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List3PhaseIT::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_List3PhaseIT::CumulativeReactiveExportEnergy)) / 100;
            case (uint8_t)Aidon::List3PhaseITShort:
                listId                = hanReader.getString(          (uint8_t)Aidon_List3PhaseIT::ListVersionIdentifier);
                meterId               = hanReader.getString(          (uint8_t)Aidon_List3PhaseIT::MeterID);
                meterModel            = hanReader.getString(          (uint8_t)Aidon_List3PhaseIT::MeterType);
                activeImportPower     = hanReader.getUint(            (uint8_t)Aidon_List3PhaseIT::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(            (uint8_t)Aidon_List3PhaseIT::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(            (uint8_t)Aidon_List3PhaseIT::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(            (uint8_t)Aidon_List3PhaseIT::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3PhaseIT::CurrentL1)) / 10;
                l2current             = 0;
                l3current             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3PhaseIT::CurrentL3)) / 10;
                l1voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3PhaseIT::VoltageL1)) / 10;
                l2voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3PhaseIT::VoltageL2)) / 10;
                l3voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_List3PhaseIT::VoltageL3)) / 10;
                if(substituteMissing) {
                    l2current         = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
                }
                break;
            case (uint8_t)Aidon::Swedish1p:
                meterTimestamp        = hanReader.getTime(            (uint8_t)Aidon_Swedish1p::Timestamp, false, false);
                activeImportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish1p::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish1p::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish1p::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish1p::CumulativeReactiveExportEnergy)) / 100;
                listId                = "AIDON_H0001";
                meterId               = "";
                meterModel            = "";
                activeImportPower     = hanReader.getUint(            (uint8_t)Aidon_Swedish1p::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(            (uint8_t)Aidon_Swedish1p::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(            (uint8_t)Aidon_Swedish1p::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(            (uint8_t)Aidon_Swedish1p::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish1p::CurrentL1)) / 10;
                l1voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish1p::VoltageL1)) / 10;
                break;
            case (uint8_t)Aidon::Swedish3p:
                meterTimestamp        = hanReader.getTime(            (uint8_t)Aidon_Swedish3p::Timestamp, false, false);
                activeImportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish3p::CumulativeActiveImportEnergy)) / 100;
                activeExportCounter   = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish3p::CumulativeActiveExportEnergy)) / 100;
                reactiveImportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish3p::CumulativeReactiveImportEnergy)) / 100;
                reactiveExportCounter = ((float) hanReader.getUint(  (uint8_t)Aidon_Swedish3p::CumulativeReactiveExportEnergy)) / 100;
                listId                = "AIDON_H0001";
                meterId               = "";
                meterModel            = "";
                activeImportPower     = hanReader.getUint(            (uint8_t)Aidon_Swedish3p::ActiveImportPower);
                reactiveImportPower   = hanReader.getUint(            (uint8_t)Aidon_Swedish3p::ReactiveImportPower);
                activeExportPower     = hanReader.getUint(            (uint8_t)Aidon_Swedish3p::ActiveExportPower);
                reactiveExportPower   = hanReader.getUint(            (uint8_t)Aidon_Swedish3p::ReactiveExportPower);
                l1current             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::CurrentL1)) / 10;
                l2current             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::CurrentL2)) / 10;
                l3current             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::CurrentL3)) / 10;
                l1voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::VoltageL1)) / 10;
                l2voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::VoltageL2)) / 10;
                l3voltage             = ((float) hanReader.getInt(   (uint8_t)Aidon_Swedish3p::VoltageL3)) / 10;
                break;
        }
    }
}

void AmsData::extractFromKamstrup(HanReader& hanReader, uint8_t listSize, bool substituteMissing) {
    switch(listSize) {
        case (uint8_t)Kamstrup::List3PhaseITShort:
        case (uint8_t)Kamstrup::List3PhaseShort:
            threePhase = true;
        case (uint8_t)Kamstrup::List1PhaseShort:
            listType = 2;
            break;
        case (uint8_t)Kamstrup::List3PhaseITLong:
        case (uint8_t)Kamstrup::List3PhaseLong:
            threePhase = true;
        case (uint8_t)Kamstrup::List1PhaseLong:
            listType = 3;
            break;
    }

    switch(listSize) {
        case (uint8_t)Kamstrup::List1PhaseLong:
            meterTimestamp        = hanReader.getTime(         (uint8_t)Kamstrup_List1Phase::MeterClock, true, true);
            activeImportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List1Phase::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List1Phase::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List1Phase::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List1Phase::CumulativeReactiveExportEnergy)) / 100;
        case (uint8_t)Kamstrup::List1PhaseShort:
            listId                = hanReader.getString(       (uint8_t)Kamstrup_List1Phase::ListVersionIdentifier);
            meterId               = hanReader.getString(       (uint8_t)Kamstrup_List1Phase::MeterID);
            meterModel            = hanReader.getString(       (uint8_t)Kamstrup_List1Phase::MeterType);
            activeImportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List1Phase::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List1Phase::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List1Phase::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List1Phase::ReactiveExportPower);
            l1current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List1Phase::CurrentL1)) / 100;
            l1voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List1Phase::VoltageL1);
            break;
        case (uint8_t)Kamstrup::List3PhaseLong:
            meterTimestamp        = hanReader.getTime(         (uint8_t)Kamstrup_List3Phase::MeterClock, true, true);
            activeImportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeReactiveExportEnergy)) / 100;
        case (uint8_t)Kamstrup::List3PhaseShort:
            listId                = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::ListVersionIdentifier);
            meterId               = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::MeterID);
            meterModel            = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::MeterType);
            activeImportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ReactiveExportPower);
            l1current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CurrentL1)) / 100;
            l2current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CurrentL2)) / 100;
            l3current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CurrentL3)) / 100;
            l1voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL1);
            l2voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL2);
            l3voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL3);
            break;
        case (uint8_t)Kamstrup::List3PhaseITLong:
            meterTimestamp        = hanReader.getTime(         (uint8_t)Kamstrup_List3Phase::MeterClock, true, true);
            activeImportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CumulativeReactiveExportEnergy)) / 100;
        case (uint8_t)Kamstrup::List3PhaseITShort:
            listId                = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::ListVersionIdentifier);
            meterId               = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::MeterID);
            meterModel            = hanReader.getString(       (uint8_t)Kamstrup_List3Phase::MeterType);
            activeImportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::ReactiveExportPower);
            l1current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CurrentL1)) / 100;
            l2current             = 0;
            l3current             = ((float) hanReader.getInt((uint8_t)Kamstrup_List3Phase::CurrentL3)) / 100;
            l1voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL1);
            l2voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL2);
            l3voltage             = hanReader.getInt(          (uint8_t)Kamstrup_List3Phase::VoltageL3);
            if(substituteMissing) {
                l2current         = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
            }
            break;
    }
}

void AmsData::extractFromOmnipower(HanReader& hanReader, uint8_t listSize, bool substituteMissing) {
    switch(listSize) {
        case (uint8_t)Kamstrup::List3PhaseITShort:
        case (uint8_t)Kamstrup::List3PhaseShort:
        case (uint8_t)Kamstrup::List1PhaseShort:
        case (uint8_t)Kamstrup::List3PhaseITLong:
        case (uint8_t)Kamstrup::List3PhaseLong:
        case (uint8_t)Kamstrup::List1PhaseLong:
            extractFromKamstrup(hanReader, listSize, substituteMissing);
            break;
        case (uint8_t)Omnipower::DLMS:
            meterTimestamp        = hanReader.getTime(         (uint8_t)Omnipower_DLMS::MeterClock, true, true);
            activeImportCounter   = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CumulativeActiveImportEnergy)) / 100;
            activeExportCounter   = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CumulativeActiveExportEnergy)) / 100;
            reactiveImportCounter = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CumulativeReactiveImportEnergy)) / 100;
            reactiveExportCounter = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CumulativeReactiveExportEnergy)) / 100;
            listId                = hanReader.getString(       (uint8_t)Omnipower_DLMS::ListVersionIdentifier);
            activeImportPower     = hanReader.getInt(          (uint8_t)Omnipower_DLMS::ActiveImportPower);
            reactiveImportPower   = hanReader.getInt(          (uint8_t)Omnipower_DLMS::ReactiveImportPower);
            activeExportPower     = hanReader.getInt(          (uint8_t)Omnipower_DLMS::ActiveExportPower);
            reactiveExportPower   = hanReader.getInt(          (uint8_t)Omnipower_DLMS::ReactiveExportPower);
            l1current             = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CurrentL1)) / 100;
            l2current             = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CurrentL2)) / 100;
            l3current             = ((float) hanReader.getInt((uint8_t)Omnipower_DLMS::CurrentL3)) / 100;
            l1voltage             = hanReader.getInt(          (uint8_t)Omnipower_DLMS::VoltageL1);
            l2voltage             = hanReader.getInt(          (uint8_t)Omnipower_DLMS::VoltageL2);
            l3voltage             = hanReader.getInt(          (uint8_t)Omnipower_DLMS::VoltageL3);
            listType = 3;
            break;
    }
    threePhase = l3voltage != 0;
}

void AmsData::apply(AmsData& other) {
    if(other.getListType() < 3) {
        unsigned long ms = this->lastUpdateMillis > other.getLastUpdateMillis() ? 0 : other.getLastUpdateMillis() - this->lastUpdateMillis;

        if(ms > 0) {
            if(other.getActiveImportPower() > 0)
                activeImportCounter += (((float) ms) * other.getActiveImportPower()) / 3600000000;

            if(other.getListType() > 1) {
                if(other.getActiveExportPower() > 0)
                    activeExportCounter += (((float) ms*2) * other.getActiveExportPower()) / 3600000000;
                if(other.getReactiveImportPower() > 0)
                    reactiveImportCounter += (((float) ms*2) * other.getReactiveImportPower()) / 3600000000;
                if(other.getReactiveExportPower() > 0)
                    reactiveExportCounter += (((float) ms*2) * other.getReactiveExportPower()) / 3600000000;
            }
            counterEstimated = true;
        }
    }

    this->lastUpdateMillis = other.getLastUpdateMillis();
    this->packageTimestamp = other.getPackageTimestamp();
    if(other.getListType() > this->listType)
        this->listType = other.getListType();
    switch(other.getListType()) {
        case 3:
            this->meterTimestamp = other.getMeterTimestamp();
            this->activeImportCounter = other.getActiveImportCounter();
            this->activeExportCounter = other.getActiveExportCounter();
            this->reactiveImportCounter = other.getReactiveImportCounter();
            this->reactiveExportCounter = other.getReactiveExportCounter();
            this->counterEstimated = false;
        case 2:
            this->listId = other.getListId();
            this->meterId = other.getMeterId();
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
