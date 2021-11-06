#include "AmsData.h"
#include "ams/ams.h"

uint8_t AMS_OBIS_VERSION[6]                 = { 1, 1,  0, 2, 129, 255 };
uint8_t AMS_OBIS_METER_MODEL[6]             = { 0, 0, 96, 1, 7,   255 };
uint8_t AMS_OBIS_METER_ID[6]                = { 0, 0, 96, 1, 0,   255 };
uint8_t AMS_OBIS_METER_TIMESTAMP[6]         = { 0, 0,  1, 0, 0,   255 };
uint8_t AMS_OBIS_ACTIVE_IMPORT[6]           = {  1, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_IMPORT_L1[6]        = { 21, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_IMPORT_L2[6]        = { 41, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_IMPORT_L3[6]        = { 61, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_EXPORT[6]           = {  2, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_EXPORT_L1[6]        = { 22, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_EXPORT_L2[6]        = { 42, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_EXPORT_L3[6]        = { 62, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_IMPORT[6]         = {  3, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_IMPORT_L1[6]      = { 23, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_IMPORT_L2[6]      = { 43, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_IMPORT_L3[6]      = { 63, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_EXPORT[6]         = {  4, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_EXPORT_L1[6]      = { 24, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_EXPORT_L2[6]      = { 44, 7, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_EXPORT_L3[6]      = { 64, 7, 0, 255 };
uint8_t AMS_OBIS_CURRENT[6]                 = { 11, 7, 0, 255 };
uint8_t AMS_OBIS_CURRENT_L1[6]              = { 31, 7, 0, 255 };
uint8_t AMS_OBIS_CURRENT_L2[6]              = { 51, 7, 0, 255 };
uint8_t AMS_OBIS_CURRENT_L3[6]              = { 71, 7, 0, 255 };
uint8_t AMS_OBIS_VOLTAGE[6]                 = { 12, 7, 0, 255 };
uint8_t AMS_OBIS_VOLTAGE_L1[6]              = { 32, 7, 0, 255 };
uint8_t AMS_OBIS_VOLTAGE_L2[6]              = { 52, 7, 0, 255 };
uint8_t AMS_OBIS_VOLTAGE_L3[6]              = { 72, 7, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_IMPORT_COUNT[6]     = {  1, 8, 0, 255 };
uint8_t AMS_OBIS_ACTIVE_EXPORT_COUNT[6]     = {  2, 8, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_IMPORT_COUNT[6]   = {  3, 8, 0, 255 };
uint8_t AMS_OBIS_REACTIVE_EXPORT_COUNT[6]   = {  4, 8, 0, 255 };

AmsData::AmsData() {}

AmsData::AmsData(const char* d, bool substituteMissing) {
    uint32_t u32;
    int32_t s32;
    char str[64];

    u32 = AMS_getUnsignedNumber(AMS_OBIS_ACTIVE_IMPORT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 1;
        activeImportPower = u32;
    }

    int meterType = AmsTypeUnknown;
    CosemData* version = AMS_findObis(AMS_OBIS_VERSION, d);
    if(version != NULL && version->base.type == CosemTypeString) {
        if(memcmp(version->str.data, "AIDON", 5) == 0) {
            meterType = AmsTypeAidon;
        } else if(memcmp(version->str.data, "Kamstrup", 8) == 0) {
            meterType = AmsTypeKamstrup;
        }
    }

    u32 = AMS_getString(AMS_OBIS_VERSION, ((char *) (d)), str);
    if(u32 > 0) {
        listId = String(str);
    }

    u32 = AMS_getUnsignedNumber(AMS_OBIS_ACTIVE_EXPORT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        activeExportPower = u32;
    }

    u32 = AMS_getUnsignedNumber(AMS_OBIS_REACTIVE_IMPORT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        reactiveImportPower = u32;
    }

    u32 = AMS_getUnsignedNumber(AMS_OBIS_REACTIVE_EXPORT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        reactiveExportPower = u32;
    }

    u32 = AMS_getUnsignedNumber(AMS_OBIS_VOLTAGE_L1, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 2;
        l1voltage = u32;
    }
    u32 = AMS_getUnsignedNumber(AMS_OBIS_VOLTAGE_L2, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 2;
        l2voltage = u32;
    }
    u32 = AMS_getUnsignedNumber(AMS_OBIS_VOLTAGE_L3, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 2;
        l3voltage = u32;
    }

    s32 = AMS_getSignedNumber(AMS_OBIS_CURRENT_L1, ((char *) (d)));
    if(s32 != 0xFFFFFFFF) {
        listType = 2;
        l1current = s32;
    }
    s32 = AMS_getSignedNumber(AMS_OBIS_CURRENT_L2, ((char *) (d)));
    if(s32 != 0xFFFFFFFF) {
        listType = 2;
        l2current = s32;
    }
    s32 = AMS_getSignedNumber(AMS_OBIS_CURRENT_L3, ((char *) (d)));
    if(s32 != 0xFFFFFFFF) {
        listType = 2;
        l3current = s32;
    }

    int vdiv = 1;
    int voltage = l1voltage == 0 ? l2voltage == 0 ? l3voltage == 0 ? 0 : l3voltage : l2voltage : l1voltage;
    while(voltage > 1000) {
        vdiv *= 10;
        voltage /= 10;
    }

    l1voltage = l1voltage != 0 ? l1voltage / vdiv : 0;
    l2voltage = l2voltage != 0 ? l2voltage / vdiv : 0;
    l3voltage = l3voltage != 0 ? l3voltage / vdiv : 0;

    if(meterType == AmsTypeAidon) {
        l1current = l1current != 0 ? l1current / 10.0 : 0;
        l2current = l2current != 0 ? l2current / 10.0 : 0;
        l3current = l3current != 0 ? l3current / 10.0 : 0;
    } else if(meterType == AmsTypeKamstrup) {
        l1current = l1current != 0 ? l1current / 100.0 : 0;
        l2current = l2current != 0 ? l2current / 100.0 : 0;
        l3current = l3current != 0 ? l3current / 100.0 : 0;
    }

    u32 = AMS_getUnsignedNumber(AMS_OBIS_ACTIVE_IMPORT_COUNT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 3;
        activeImportCounter = u32 / 100.0;
    }
    u32 = AMS_getUnsignedNumber(AMS_OBIS_ACTIVE_EXPORT_COUNT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 3;
        activeExportCounter = u32 / 100.0;
    }
    u32 = AMS_getUnsignedNumber(AMS_OBIS_REACTIVE_IMPORT_COUNT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 3;
        reactiveImportCounter = u32 / 100.0;
    }
    u32 = AMS_getUnsignedNumber(AMS_OBIS_REACTIVE_EXPORT_COUNT, ((char *) (d)));
    if(u32 != 0xFFFFFFFF) {
        listType = 3;
        reactiveExportCounter = u32 / 100.0;
    }

    u32 = AMS_getString(AMS_OBIS_METER_MODEL, ((char *) (d)), str);
    if(u32 > 0) {
        meterModel = String(str);
    }

    u32 = AMS_getString(AMS_OBIS_METER_ID, ((char *) (d)), str);
    if(u32 > 0) {
        meterId = String(str);
    }

    time_t ts = AMS_getTimestamp(AMS_OBIS_METER_TIMESTAMP, ((char *) (d)));
    if(ts > 0) {
        meterTimestamp = ts;
    }

    threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
    twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);

    if(threePhase) {
        if(substituteMissing) {
            l2current         = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
        }
    }

/*
    time_t packageTimestamp = 0;
*/
/*    
    CosemData* model = AMS_findObis(AMS_OBIS_METER_MODEL, ((char *) (d)));
    if(model != NULL && model->base.type == CosemTypeString) {
        this->meterModel = String((const char*) model->str.data);
    }
*/
    lastUpdateMillis = millis();
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

bool AmsData::isTwoPhase() {
    return this->twoPhase;
}
