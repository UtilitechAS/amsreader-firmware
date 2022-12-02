#ifndef _IEC62056_7_5_H
#define _IEC62056_7_5_H

#include "AmsData.h"
#include "AmsConfiguration.h"
#include "DataParser.h"
#include "Cosem.h"

#define NOVALUE 0xFFFFFFFF

struct AmsOctetTimestamp {
    uint8_t type;
    CosemDateTime dt;
} __attribute__((packed));

class IEC6205675 : public AmsData {
public:
    IEC6205675(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx);

private:
    CosemData* getCosemDataAt(uint8_t index, const char* ptr);
    CosemData* findObis(uint8_t* obis, int matchlength, const char* ptr);
    uint8_t getString(uint8_t* obis, int matchlength, const char* ptr, char* target);
    double getNumber(uint8_t* obis, int matchlength, const char* ptr);
    double getNumber(CosemData*);
    time_t getTimestamp(uint8_t* obis, int matchlength, const char* ptr);

    uint8_t AMS_OBIS_VERSION[4]                 = {  0, 2, 129, 255 };
    uint8_t AMS_OBIS_METER_MODEL[4]             = { 96, 1, 1, 255 };
    uint8_t AMS_OBIS_METER_MODEL_2[4]           = { 96, 1, 7, 255 };
    uint8_t AMS_OBIS_METER_ID[4]                = { 96, 1, 0, 255 };
    uint8_t AMS_OBIS_METER_ID_2[4]              = {  0, 0, 5, 255 };
    uint8_t AMS_OBIS_METER_TIMESTAMP[4]         = {  1, 0, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_IMPORT[4]           = {  1, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_EXPORT[4]           = {  2, 7, 0, 255 };
    uint8_t AMS_OBIS_REACTIVE_IMPORT[4]         = {  3, 7, 0, 255 };
    uint8_t AMS_OBIS_REACTIVE_EXPORT[4]         = {  4, 7, 0, 255 };
    uint8_t AMS_OBIS_CURRENT_L1[4]              = { 31, 7, 0, 255 };
    uint8_t AMS_OBIS_CURRENT_L2[4]              = { 51, 7, 0, 255 };
    uint8_t AMS_OBIS_CURRENT_L3[4]              = { 71, 7, 0, 255 };
    uint8_t AMS_OBIS_VOLTAGE_L1[4]              = { 32, 7, 0, 255 };
    uint8_t AMS_OBIS_VOLTAGE_L2[4]              = { 52, 7, 0, 255 };
    uint8_t AMS_OBIS_VOLTAGE_L3[4]              = { 72, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_IMPORT_COUNT[4]     = {  1, 8, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_EXPORT_COUNT[4]     = {  2, 8, 0, 255 };
    uint8_t AMS_OBIS_REACTIVE_IMPORT_COUNT[4]   = {  3, 8, 0, 255 };
    uint8_t AMS_OBIS_REACTIVE_EXPORT_COUNT[4]   = {  4, 8, 0, 255 };
    uint8_t AMS_OBIS_POWER_FACTOR[4]            = { 13, 7, 0, 255 };
    uint8_t AMS_OBIS_POWER_FACTOR_L1[4]         = { 33, 7, 0, 255 };
    uint8_t AMS_OBIS_POWER_FACTOR_L2[4]         = { 53, 7, 0, 255 };
    uint8_t AMS_OBIS_POWER_FACTOR_L3[4]         = { 73, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_IMPORT_L1[4]        = { 21, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_IMPORT_L2[4]        = { 41, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_IMPORT_L3[4]        = { 61, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_EXPORT_L1[4]        = { 22, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_EXPORT_L2[4]        = { 42, 7, 0, 255 };
    uint8_t AMS_OBIS_ACTIVE_EXPORT_L3[4]        = { 62, 7, 0, 255 };

};
#endif
