#ifndef _ENTSOEAPI_H
#define _ENTSOEAPI_H

#include "time.h"
#include "Timezone.h"
#include "RemoteDebug.h"
#include "EntsoeA44Parser.h"

#define ENTSOE_NO_VALUE -127
#define ENTSOE_DEFAULT_MULTIPLIER 1.00

class EntsoeApi {
public:
    EntsoeApi(RemoteDebug* Debug);
    bool loop();

    double getValueForHour(int hour);
    double getValueForHour(time_t now, int hour);
    char* getCurrency();

    void setToken(const char* token);
    void setArea(const char* area);
    void setCurrency(const char* currency);
    void setMultiplier(double multiplier);

private:
    RemoteDebug* debugger;
    char token[37]; // UUID + null terminator

    uint64_t midnightMillis = 0;
    uint64_t lastTomorrowFetch = 0;
    uint64_t lastCurrencyFetch = 0;
    EntsoeA44Parser* today = NULL;
    EntsoeA44Parser* tomorrow = NULL;

    Timezone* tz = NULL;

    char area[32];
    char currency[4];
    double multiplier = ENTSOE_DEFAULT_MULTIPLIER;
    double currencyMultiplier = ENTSOE_DEFAULT_MULTIPLIER;

    bool retrieve(const char* url, Stream* doc);
    double getCurrencyMultiplier(const char* from, const char* to);

	void printD(String fmt, ...);
	void printI(String fmt, ...);
	void printW(String fmt, ...);
	void printE(String fmt, ...);
};
#endif
