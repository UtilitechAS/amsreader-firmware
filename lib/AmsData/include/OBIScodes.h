/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _OBISCODES_H
#define _OBISCODES_H

#include "lwip/def.h"

#define OBIS_MEDIUM_ABSTRACT 0
#define OBIS_MEDIUM_ELECTRICITY 1

#define OBIS_CHAN_0 0
#define OBIS_CHAN_1 1

#define OBIS_RANGE_NA 0xFF

struct OBIS_head_t {
    uint8_t medium;
    uint8_t channel;
} __attribute__((packed));

struct OBIS_code_t {
    uint8_t sensor;
    uint8_t gr;
    uint8_t tariff;
} __attribute__((packed));

struct OBIS_t {
    OBIS_head_t head;
    OBIS_code_t code;
    uint8_t range;
} __attribute__((packed));


const OBIS_code_t OBIS_NULL PROGMEM =                  {   0,   0,   0 };

const OBIS_code_t OBIS_VERSION PROGMEM =               {   0,   2, 129 };
const OBIS_code_t OBIS_METER_MODEL PROGMEM =           {  96,   1,   1 };
const OBIS_code_t OBIS_METER_MODEL_2 PROGMEM =         {  96,   1,   7 };
const OBIS_code_t OBIS_METER_ID PROGMEM =              {  96,   1,   0 };
const OBIS_code_t OBIS_METER_ID_2 PROGMEM =            {   0,   0,   5 };
const OBIS_code_t OBIS_METER_TIMESTAMP PROGMEM =       {   1,   0,   0 };

const OBIS_code_t OBIS_ACTIVE_IMPORT PROGMEM =         {   1,   7,   0 };
const OBIS_code_t OBIS_ACTIVE_IMPORT_COUNT PROGMEM =   {   1,   8,   0 };
const OBIS_code_t OBIS_ACTIVE_EXPORT PROGMEM =         {   2,   7,   0 };
const OBIS_code_t OBIS_ACTIVE_EXPORT_COUNT PROGMEM =   {   2,   8,   0 };
const OBIS_code_t OBIS_REACTIVE_IMPORT PROGMEM =       {   3,   7,   0 };
const OBIS_code_t OBIS_REACTIVE_IMPORT_COUNT PROGMEM = {   3,   8,   0 };
const OBIS_code_t OBIS_REACTIVE_EXPORT PROGMEM =       {   4,   7,   0 };
const OBIS_code_t OBIS_REACTIVE_EXPORT_COUNT PROGMEM = {   4,   8,   0 };

const OBIS_code_t OBIS_POWER_FACTOR PROGMEM =          {  13,   7,   0 };

const OBIS_code_t OBIS_ACTIVE_IMPORT_L1 PROGMEM =      {  21,   7,   0 };
const OBIS_code_t OBIS_ACTIVE_EXPORT_L1 PROGMEM =      {  22,   7,   0 };

const OBIS_code_t OBIS_CURRENT_L1 PROGMEM =            {  31,   7,   0 };
const OBIS_code_t OBIS_VOLTAGE_L1 PROGMEM =            {  32,   7,   0 };
const OBIS_code_t OBIS_POWER_FACTOR_L1 PROGMEM =       {  33,   7,   0 };

const OBIS_code_t OBIS_ACTIVE_IMPORT_L2 PROGMEM =      {  41,   7,   0 };
const OBIS_code_t OBIS_ACTIVE_EXPORT_L2 PROGMEM =      {  42,   7,   0 };

const OBIS_code_t OBIS_CURRENT_L2 PROGMEM =            {  51,   7,   0 };
const OBIS_code_t OBIS_VOLTAGE_L2 PROGMEM =            {  52,   7,   0 };
const OBIS_code_t OBIS_POWER_FACTOR_L2 PROGMEM =       {  53,   7,   0 };

const OBIS_code_t OBIS_ACTIVE_IMPORT_L3 PROGMEM =      {  61,   7,   0 };
const OBIS_code_t OBIS_ACTIVE_EXPORT_L3 PROGMEM =      {  62,   7,   0 };

const OBIS_code_t OBIS_CURRENT_L3 PROGMEM =            {  71,   7,   0 };
const OBIS_code_t OBIS_VOLTAGE_L3 PROGMEM =            {  72,   7,   0 };
const OBIS_code_t OBIS_POWER_FACTOR_L3 PROGMEM =       {  73,   7,   0 };

#endif
