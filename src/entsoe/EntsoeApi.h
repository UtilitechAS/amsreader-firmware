#ifndef _ENTSOEAPI_H
#define _ENTSOEAPI_H

#include "TimeLib.h"
#include "Timezone.h"
#include "RemoteDebug.h"
#include "EntsoeA44Parser.h"
#include "AmsConfiguration.h"

#if defined(ESP8266)
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <HTTPClient.h>
#else
	#warning "Unsupported board type"
#endif

#define ENTSOE_DEFAULT_MULTIPLIER 1.00
#define SSL_BUF_SIZE 512

class EntsoeApi {
public:
    EntsoeApi(RemoteDebug*);
    void setup(EntsoeConfig&);
    bool loop();

    char* getToken();
    char* getCurrency();
    float getValueForHour(uint8_t);
    float getValueForHour(time_t, uint8_t);

private:
    RemoteDebug* debugger;
    EntsoeConfig* config = NULL;
    WiFiClientSecure client;
    HTTPClient https;

    uint32_t tomorrowFetchMillis = 3600000; // Number of ms before midnight. Default fetch 10hrs before midnight (14:00 CE(S)T)
    uint64_t midnightMillis = 0;
    uint64_t lastTodayFetch = 0;
    uint64_t lastTomorrowFetch = 0;
    uint64_t lastCurrencyFetch = 0;
    EntsoeA44Parser* today = NULL;
    EntsoeA44Parser* tomorrow = NULL;

    Timezone* tz = NULL;

    float currencyMultiplier = ENTSOE_DEFAULT_MULTIPLIER;

    bool retrieve(const char* url, Stream* doc);
    float getCurrencyMultiplier(const char* from, const char* to);

	void printD(String fmt, ...);
	void printE(String fmt, ...);
};
#endif
