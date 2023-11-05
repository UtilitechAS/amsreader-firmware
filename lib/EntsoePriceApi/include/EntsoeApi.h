#ifndef _ENTSOEAPI_H
#define _ENTSOEAPI_H

#include "TimeLib.h"
#include "Timezone.h"
#include "RemoteDebug.h"
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

class EntsoeApi {
public:
    EntsoeApi(RemoteDebug*);
    void setup(EntsoeConfig&);
    bool loop();

    char* getToken();
    char* getCurrency();
    char* getArea();
    char* getSource();
    float getValueForHour(int8_t);
    float getValueForHour(time_t, int8_t);

    int16_t getLastError();

private:
    RemoteDebug* debugger;
    EntsoeConfig* config = NULL;
    HTTPClient* http = NULL;

    uint8_t currentDay = 0, currentHour = 0;
    uint8_t tomorrowFetchMinute = 15; // How many minutes over 13:00 should it fetch prices
    uint8_t nextFetchDelayMinutes = 15;
    uint64_t lastTodayFetch = 0;
    uint64_t lastTomorrowFetch = 0;
    uint64_t lastCurrencyFetch = 0;
    PricesContainer* today = NULL;
    PricesContainer* tomorrow = NULL;

    Timezone* tz = NULL;

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

    void debugPrint(byte *buffer, int start, int length);
};
#endif
