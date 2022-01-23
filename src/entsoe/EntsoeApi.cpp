#include "EntsoeApi.h"
#include <EEPROM.h>
#include "Uptime.h"
#include "TimeLib.h"
#include "DnbCurrParser.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#endif

EntsoeApi::EntsoeApi(RemoteDebug* Debug) {
    debugger = Debug;

    client.setInsecure();
    https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    https.setTimeout(5000);

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
    lastCurrencyFetch = 0;
}

char* EntsoeApi::getToken() {
    return this->config->token;
}

char* EntsoeApi::getCurrency() {
    return this->config->currency;
}

float EntsoeApi::getValueForHour(uint8_t hour) {
    time_t cur = time(nullptr);
    return getValueForHour(cur, hour);
}

float EntsoeApi::getValueForHour(time_t cur, uint8_t hour) {
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
        if(strcmp(tomorrow->getMeasurementUnit(), "MWH") == 0) {
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
        if(strcmp(today->getMeasurementUnit(), "MWH") == 0) {
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

    if(midnightMillis == 0) {
        time_t t = time(nullptr);
        if(t <= 0) return false; // NTP not ready

        time_t epoch = tz->toLocal(t);
        
        tmElements_t tm;
        breakTime(epoch, tm);
        if(tm.Year > 50) { // Make sure we are in 2021 or later (years after 1970)
            uint32_t curDayMillis = (((((tm.Hour * 60) + tm.Minute) * 60) + tm.Second) * 1000);

            midnightMillis = now + (SECS_PER_DAY * 1000) - curDayMillis + 1000; // Adding 1s to ensure we have passed midnight
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Setting midnight millis %lu\n", midnightMillis);
        }
    } else if(now > midnightMillis) {
        time_t t = time(nullptr);
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Rotating price objects at %lu\n", t);
        delete today;
        today = tomorrow;
        tomorrow = NULL;
        midnightMillis = 0; // Force new midnight millis calculation
    } else {
        if(today == NULL && (lastTodayFetch == 0 || now - lastTodayFetch > 60000)) {
            lastTodayFetch = now;
            time_t e1 = time(nullptr) - (SECS_PER_DAY * 1);
            time_t e2 = e1 + SECS_PER_DAY;
            tmElements_t d1, d2;
            breakTime(e1, d1);
            breakTime(e2, d2);

            char url[256];
            snprintf(url, sizeof(url), "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
            "https://transparency.entsoe.eu/api", getToken(), 
            d1.Year+1970, d1.Month, d1.Day, 23, 00,
            d2.Year+1970, d2.Month, d2.Day, 23, 00,
            config->area, config->area);

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif


            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Fetching prices for today\n");
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", url);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(url, a44) && a44->getPoint(0) != ENTSOE_NO_VALUE) {
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
            time_t e1 = time(nullptr);
            time_t e2 = e1 + SECS_PER_DAY;
            tmElements_t d1, d2;
            breakTime(e1, d1);
            breakTime(e2, d2);

            char url[256];
            snprintf(url, sizeof(url), "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
            "https://transparency.entsoe.eu/api", getToken(), 
            d1.Year+1970, d1.Month, d1.Day, 23, 00,
            d2.Year+1970, d2.Month, d2.Day, 23, 00,
            config->area, config->area);

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Fetching prices for tomorrow\n");
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", url);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(url, a44) && a44->getPoint(0) != ENTSOE_NO_VALUE) {
                tomorrow = a44;
                return true;
            } else if(a44 != NULL) {
                delete a44;
                tomorrow = NULL;
                return false;
            }
        }
    }
    return false;
}

bool EntsoeApi::retrieve(const char* url, Stream* doc) {
    #if defined(ESP8266)
        // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html#mfln-or-maximum-fragment-length-negotiation-saving-ram
        /* Rumor has it that a client cannot request a lower max_fragment_length, so I guess thats why the following does not work.
           And there is currently not enough heap space to go around in this project to do a full HTTPS request on ESP8266

        int bufSize = 512;
        while(!client.probeMaxFragmentLength("transparency.entsoe.eu", 443, bufSize) && bufSize <= 4096) {
            bufSize += 512;
        }
        if(client.probeMaxFragmentLength("transparency.entsoe.eu", 443, bufSize)) {
            printD("Negotiated MFLN size");
            printD(String(bufSize));
            client.setBufferSizes(bufSize, bufSize);
        }
        */
    #endif

    if(https.begin(client, url)) {
        printD("Connection established");

        #if defined(ESP32)
            esp_task_wdt_reset();
        #elif defined(ESP8266)
            ESP.wdtFeed();
        #endif

        /*
        #if defined(ESP8266)
            if(!client.getMFLNStatus()) {
                printE("Negotiated MFLN was not respected");
                https.end();
                client.stop();
                return false;
            }
        #endif
        */

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
            printE("Communication error: ");
            printE(https.errorToString(status));
            printD(https.getString());

            #if defined(ESP8266)
            char buf[64];
            client.getLastSSLError(buf,64);
            printE(buf);
            #endif

            https.end();
            return false;
        }
    } else {
        #if defined(ESP8266)
        char buf[64];
        client.getLastSSLError(buf,64);
        printE(buf);
        #endif
        return false;
    }
    client.stop();
}

float EntsoeApi::getCurrencyMultiplier(const char* from, const char* to) {
    if(strcmp(from, to) == 0)
        return 1.00;

    uint64_t now = millis64();
    if(lastCurrencyFetch < midnightMillis) {
        char url[256];
        DnbCurrParser p;

        #if defined(ESP32)
            esp_task_wdt_reset();
        #elif defined(ESP8266)
            ESP.wdtFeed();
        #endif

        snprintf(url, sizeof(url), "https://data.norges-bank.no/api/data/EXR/M.%s.NOK.SP?lastNObservations=1", from);
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Retrieving %s to NOK conversion\n", from);
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", url);
        if(retrieve(url, &p)) {
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  got exchange rate %.4f\n", p.getValue());
            currencyMultiplier = p.getValue();
            if(strncmp(to, "NOK", 3) != 0) {
                snprintf(url, sizeof(url), "https://data.norges-bank.no/api/data/EXR/M.%s.NOK.SP?lastNObservations=1", to);
                if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Retrieving %s to NOK conversion\n", to);
                if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", url);
                if(retrieve(url, &p)) {
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
