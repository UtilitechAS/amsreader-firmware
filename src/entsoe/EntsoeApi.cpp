#include "EntsoeApi.h"
#include <EEPROM.h>
#include "Uptime.h"
#include "TimeLib.h"
#include "DnbCurrParser.h"
#include "version.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

EntsoeApi::EntsoeApi(RemoteDebug* Debug) {
    this->buf = (char*) malloc(BufferSize);

    debugger = Debug;

    // Entso-E uses CET/CEST
    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
	TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
	tz = new Timezone(CEST, CET);

    tomorrowFetchMillis = 36000000 + (random(1800) * 1000); // Random between 13:30 and 14:00
}

void EntsoeApi::setup(EntsoeConfig& config) {
    if(this->config == NULL) {
        this->config = new EntsoeConfig();
    }
    memcpy(this->config, &config, sizeof(config));
    lastTodayFetch = lastTomorrowFetch = lastCurrencyFetch = 0;
    if(today != NULL) delete today;
    if(tomorrow != NULL) delete tomorrow;
    today = tomorrow = NULL;
}

char* EntsoeApi::getToken() {
    return this->config->token;
}

char* EntsoeApi::getCurrency() {
    return this->config->currency;
}

float EntsoeApi::getValueForHour(int8_t hour) {
    time_t cur = time(nullptr);
    return getValueForHour(cur, hour);
}

float EntsoeApi::getValueForHour(time_t cur, int8_t hour) {
    tmElements_t tm;
    if(tz != NULL)
        cur = tz->toLocal(cur);
    breakTime(cur, tm);
    int pos = tm.Hour + hour;
    if(pos >= 48)
        return ENTSOE_NO_VALUE;

    double value = ENTSOE_NO_VALUE;
    double multiplier = config->multiplier / 1000.0;
    if(pos > 23) {
        if(tomorrow == NULL)
            return ENTSOE_NO_VALUE;
        value = tomorrow->getPoint(pos-24);
        if(value != ENTSOE_NO_VALUE && strcmp(tomorrow->getMeasurementUnit(), "MWH") == 0) {
            multiplier *= 0.001;
        } else {
            return ENTSOE_NO_VALUE;
        }
        float mult = getCurrencyMultiplier(tomorrow->getCurrency(), config->currency);
        if(mult == 0) return ENTSOE_NO_VALUE;
        multiplier *= mult;
    } else if(pos >= 0) {
        if(today == NULL)
            return ENTSOE_NO_VALUE;
        value = today->getPoint(pos);
        if(value != ENTSOE_NO_VALUE && strcmp(today->getMeasurementUnit(), "MWH") == 0) {
            multiplier *= 0.001;
        } else {
            return ENTSOE_NO_VALUE;
        }
        float mult = getCurrencyMultiplier(today->getCurrency(), config->currency);
        if(mult == 0) return ENTSOE_NO_VALUE;
        multiplier *= mult;
    }
    return value * multiplier;
}

bool EntsoeApi::loop() {
    if(strlen(getToken()) == 0)
        return false;

    uint64_t now = millis64();
    if(now < 10000) return false; // Grace period

    time_t t = time(nullptr);
    if(t < BUILD_EPOCH) return false;

    bool ret = false;
    tmElements_t tm;
    breakTime(tz->toLocal(t), tm);
    if(currentHour != tm.Hour) {
        currentHour = tm.Hour;
        ret = today != NULL; // Only trigger MQTT publish if we have todays prices.
    }

    if(midnightMillis == 0) {
        uint32_t curDayMillis = (((((tm.Hour * 60) + tm.Minute) * 60) + tm.Second) * 1000);
        midnightMillis = now + (SECS_PER_DAY * 1000) - curDayMillis;
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Setting midnight millis %llu\n", midnightMillis);
        currentDay = tm.Day;
        return false;
    } else if(now > midnightMillis && currentDay != tm.Day) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Rotating price objects at %lld\n", t);
        if(today != NULL) delete today;
        if(tomorrow != NULL) {
            today = tomorrow;
            tomorrow = NULL;
        }
        currentDay = tm.Day;
        midnightMillis = 0; // Force new midnight millis calculation
        return true;
    } else {
        breakTime(t, tm); // Break UTC to find UTC midnight
        if(today == NULL && (lastTodayFetch == 0 || now - lastTodayFetch > 60000)) {
            lastTodayFetch = now;
            time_t e1 = t - (tm.Hour * 3600) - (tm.Minute * 60) - tm.Second; // UTC midnight
            time_t e2 = e1 + SECS_PER_DAY;
            tmElements_t d1, d2;
            breakTime(tz->toUTC(e1), d1); // To get day and hour for CET/CEST at UTC midnight
            breakTime(tz->toUTC(e2), d2);

            snprintf(buf, BufferSize, "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
            "https://transparency.entsoe.eu/api", getToken(), 
            d1.Year+1970, d1.Month, d1.Day, d1.Hour, 00,
            d2.Year+1970, d2.Month, d2.Day, d2.Hour, 00,
            config->area, config->area);

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif


            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Fetching prices for today\n");
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", buf);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(buf, a44) && a44->getPoint(0) != ENTSOE_NO_VALUE) {
                today = a44;
                return true;
            } else if(a44 != NULL) {
                delete a44;
                today = NULL;
                return false;
            }
        }

        // Prices for next day are published at 13:00 CE(S)T, but to avoid heavy server traffic at that time, we will 
        // fetch 1 hr after that (with some random delay) and retry every 15 minutes
        if(tomorrow == NULL
            && midnightMillis - now < tomorrowFetchMillis
            && (lastTomorrowFetch == 0 || now - lastTomorrowFetch > 900000)
        ) {
            lastTomorrowFetch = now;
            time_t e1 = t - (tm.Hour * 3600) - (tm.Minute * 60) - tm.Second + (SECS_PER_DAY);
            time_t e2 = e1 + SECS_PER_DAY;
            tmElements_t d1, d2;
            breakTime(tz->toUTC(e1), d1);
            breakTime(tz->toUTC(e2), d2);

            snprintf(buf, BufferSize, "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
            "https://transparency.entsoe.eu/api", getToken(), 
            d1.Year+1970, d1.Month, d1.Day, d1.Hour, 00,
            d2.Year+1970, d2.Month, d2.Day, d2.Hour, 00,
            config->area, config->area);

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Fetching prices for tomorrow\n");
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", buf);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(buf, a44) && a44->getPoint(0) != ENTSOE_NO_VALUE) {
                tomorrow = a44;
                return true;
            } else if(a44 != NULL) {
                delete a44;
                tomorrow = NULL;
                return false;
            }
        }
    }
    return ret;
}

bool EntsoeApi::retrieve(const char* url, Stream* doc) {
    HTTPClient https;
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    https.setReuse(false);
    https.setTimeout(50000);
    https.setUserAgent("ams2mqtt/" + String(VERSION));
    #if defined(ESP32)
        if(https.begin(url)) {
            printD("Connection established");

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            int status = https.GET();

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(status == HTTP_CODE_OK) {
                printD("Receiving data");
                https.writeToStream(doc);
                https.end();
                return true;
            } else {
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("(EntsoeApi) Communication error, returned status: %d\n", status);
                printE(https.errorToString(status));
                printD(https.getString());

                https.end();
                return false;
            }
        } else {
            return false;
        }
    #endif
    return false;
}

float EntsoeApi::getCurrencyMultiplier(const char* from, const char* to) {
    if(strcmp(from, to) == 0)
        return 1.00;

    uint64_t now = millis64();
    if(now > lastCurrencyFetch && (lastCurrencyFetch == 0 || (now - lastCurrencyFetch) > 60000)) {
        lastCurrencyFetch = now;
        
        DnbCurrParser p;

        #if defined(ESP32)
            esp_task_wdt_reset();
        #elif defined(ESP8266)
            ESP.wdtFeed();
        #endif

        snprintf(buf, BufferSize, "https://data.norges-bank.no/api/data/EXR/M.%s.NOK.SP?lastNObservations=1", from);
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Retrieving %s to NOK conversion\n", from);
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", buf);
        if(retrieve(buf, &p)) {
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  got exchange rate %.4f\n", p.getValue());
            currencyMultiplier = p.getValue();
            if(strncmp(to, "NOK", 3) != 0) {
                snprintf(buf, BufferSize, "https://data.norges-bank.no/api/data/EXR/M.%s.NOK.SP?lastNObservations=1", to);
                if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Retrieving %s to NOK conversion\n", to);
                if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", buf);
                if(retrieve(buf, &p)) {
                    if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  got exchange rate %.4f\n", p.getValue());
                    currencyMultiplier /= p.getValue();
                } else {
                    return 0;
                }
            }
        } else {
            return 0;
        }
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi) Resulting currency multiplier: %.4f\n", currencyMultiplier);
        lastCurrencyFetch = midnightMillis;
    }
    return currencyMultiplier;
}

void EntsoeApi::printD(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void EntsoeApi::printE(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}
