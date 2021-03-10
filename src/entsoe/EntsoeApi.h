#ifndef _ENTSOEAPI_H
#define _ENTSOEAPI_H

#include "time.h"
#include "Timezone.h"
#include "RemoteDebug.h"
#include "EntsoeA44Parser.h"
#include "AmsConfiguration.h"

#define ENTSOE_NO_VALUE -127
#define ENTSOE_DEFAULT_MULTIPLIER 1.00
#define SSL_BUF_SIZE 512

class EntsoeApi {
public:
    EntsoeApi(RemoteDebug*);
    void setup(EntsoeConfig&);
    bool loop();

    char* getToken();
    float getValueForHour(uint8_t);
    float getValueForHour(time_t, uint8_t);

private:
    RemoteDebug* debugger;
    EntsoeConfig* config = NULL;

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
	void printI(String fmt, ...);
	void printW(String fmt, ...);
	void printE(String fmt, ...);
};
#endif
