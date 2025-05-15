/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PRICESERVICE_H
#define _PRICESERVICE_H

#include <vector>

#include "TimeLib.h"
#include "Timezone.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "AmsConfiguration.h"
#include "EntsoeA44Parser.h"

#if defined(ESP8266)
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <HTTPClient.h>
#else
	#warning "Unsupported board type"
#endif

#define SSL_BUF_SIZE 512

#define PRICE_DIRECTION_IMPORT 0x01
#define PRICE_DIRECTION_EXPORT 0x02
#define PRICE_DIRECTION_BOTH 0x03

#define PRICE_DAY_MO 0x01
#define PRICE_DAY_TU 0x02
#define PRICE_DAY_WE 0x04
#define PRICE_DAY_TH 0x08
#define PRICE_DAY_FR 0x10
#define PRICE_DAY_SA 0x12
#define PRICE_DAY_SU 0x14

#define PRICE_TYPE_FIXED 0x00
#define PRICE_TYPE_ADD 0x01
#define PRICE_TYPE_PCT 0x02
#define PRICE_TYPE_SUBTRACT 0x03

struct PriceConfig {
    char name[32];
    uint8_t direction;
    uint8_t days;
    uint32_t hours;
    uint8_t type;
    uint32_t value;
    uint8_t start_month;
    uint8_t start_dayofmonth;
    uint8_t end_month;
    uint8_t end_dayofmonth;
};

struct PricePart {
    char name[32];
    char description[32];
    uint32_t value;
};

class PriceService {
public:
    #if defined(AMS_REMOTE_DEBUG)
    PriceService(RemoteDebug*);
    #else
    PriceService(Stream*);
    #endif
    void setup(PriceServiceConfig&);
    void setTimezone(Timezone* tz);
    bool loop();

    char* getToken();
    char* getCurrency();
    char* getArea();
    char* getSource();
    float getValueForHour(uint8_t direction, int8_t hour);
    float getValueForHour(uint8_t direction, time_t ts, int8_t hour);

    float getEnergyPriceForHour(uint8_t direction, time_t ts, int8_t hour);

    std::vector<PriceConfig>& getPriceConfig();
    void setPriceConfig(uint8_t index, PriceConfig &priceConfig);
    void cropPriceConfig(uint8_t size);

    PricePart getPricePart(uint8_t index);

    int16_t getLastError();

    bool load();
    bool save();

private:
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
    #else
    Stream* debugger;
    #endif
    PriceServiceConfig* config = NULL;
    HTTPClient* http = NULL;

    uint8_t currentDay = 0, currentHour = 0;
    uint8_t tomorrowFetchMinute = 15; // How many minutes over 13:00 should it fetch prices
    uint8_t nextFetchDelayMinutes = 15;
    uint64_t lastTodayFetch = 0;
    uint64_t lastTomorrowFetch = 0;
    uint64_t lastCurrencyFetch = 0;
    PricesContainer* today = NULL;
    PricesContainer* tomorrow = NULL;

    std::vector<PriceConfig> priceConfig;

    Timezone* tz = NULL;
    Timezone* entsoeTz = NULL;

    static const uint16_t BufferSize = 256;
    char* buf;

    bool hub = false;
    uint8_t* key = NULL;
    uint8_t* auth = NULL;

    float currencyMultiplier = 0;

    int16_t lastError = 0;

    PricesContainer* fetchPrices(time_t);
    bool retrieve(const char* url, Stream* doc);
    float getCurrencyMultiplier(const char* from, const char* to, time_t t);
};
#endif
