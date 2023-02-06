#include "EntsoeApi.h"
#include <EEPROM.h>
#include "Uptime.h"
#include "TimeLib.h"
#include "DnbCurrParser.h"
#include "version.h"

#include "GcmParser.h"

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

    tomorrowFetchMinute = 15 + random(45); // Random between 13:15 and 14:00
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

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);
    http.setTimeout(60000);
    http.setUserAgent("ams2mqtt/" + String(VERSION));
    http.useHTTP10(true);

    #if defined(AMS2MQTT_PRICE_KEY)
        key = new uint8_t[16] AMS2MQTT_PRICE_KEY;
        hub = true;
    #else
        hub = false;
    #endif
    #if defined(AMS2MQTT_PRICE_AUTHENTICATION)
        auth = new uint8_t[16] AMS2MQTT_PRICE_AUTHENTICATION;
        hub = hub && true;
    #else
        hub = false;
    #endif

}

char* EntsoeApi::getToken() {
    return this->config->token;
}

char* EntsoeApi::getCurrency() {
    return this->config->currency;
}

char* EntsoeApi::getArea() {
    return this->config->area;
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
        if(tomorrow->points[pos-24] == ENTSOE_NO_VALUE)
            return ENTSOE_NO_VALUE;
        value = tomorrow->points[pos-24] / 10000.0;
        if(strcmp(tomorrow->measurementUnit, "KWH") == 0) {
            // Multiplier is 1
        } else if(strcmp(tomorrow->measurementUnit, "MWH") == 0) {
            multiplier *= 0.001;
        } else {
            return ENTSOE_NO_VALUE;
        }
        float mult = getCurrencyMultiplier(tomorrow->currency, config->currency, cur);
        if(mult == 0) return ENTSOE_NO_VALUE;
        multiplier *= mult;
    } else if(pos >= 0) {
        if(today == NULL)
            return ENTSOE_NO_VALUE;
        if(today->points[pos] == ENTSOE_NO_VALUE)
            return ENTSOE_NO_VALUE;
        value = today->points[pos] / 10000.0;
        if(strcmp(today->measurementUnit, "KWH") == 0) {
            // Multiplier is 1
        } else if(strcmp(today->measurementUnit, "MWH") == 0) {
            multiplier *= 0.001;
        } else {
            return ENTSOE_NO_VALUE;
        }
        float mult = getCurrencyMultiplier(today->currency, config->currency, cur);
        if(mult == 0) return ENTSOE_NO_VALUE;
        multiplier *= mult;
    }
    return value * multiplier;
}

bool EntsoeApi::loop() {
    uint64_t now = millis64();
    if(now < 10000) return false; // Grace period

    time_t t = time(nullptr);
    if(t < BUILD_EPOCH) return false;

    #ifndef AMS2MQTT_PRICE_KEY
    if(strlen(getToken()) == 0) {
        return false;
    }
    #endif
    if(!config->enabled)
        return false;
    if(strlen(config->area) == 0)
        return false;
    if(strlen(config->currency) == 0)
        return false;

    tmElements_t tm;
    breakTime(tz->toLocal(t), tm);

    if(currentDay == 0) {
        currentDay = tm.Day;
        currentHour = tm.Hour;
    }
    
    if(currentDay != tm.Day) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Rotating price objects at %lu\n", t);
        if(today != NULL) delete today;
        if(tomorrow != NULL) {
            today = tomorrow;
            tomorrow = NULL;
        }
        currentDay = tm.Day;
        currentHour = tm.Hour;
        return today != NULL; // Only trigger MQTT publish if we have todays prices.
    } else if(currentHour != tm.Hour) {
        currentHour = tm.Hour;
        return today != NULL; // Only trigger MQTT publish if we have todays prices.
    }

    if(today == NULL && (lastTodayFetch == 0 || now - lastTodayFetch > 60000)) {
        try {
            lastTodayFetch = now;
            today = fetchPrices(t);
        } catch(const std::exception& e) {
            if(lastError == 0) lastError = 900;
            today = NULL;
        }
        return today != NULL; // Only trigger MQTT publish if we have todays prices.
    }

    // Prices for next day are published at 13:00 CE(S)T, but to avoid heavy server traffic at that time, we will 
    // fetch with one hour (with some random delay) and retry every 15 minutes
    if(tomorrow == NULL && (tm.Hour > 13 || (tm.Hour == 13 && tm.Minute >= tomorrowFetchMinute)) && (lastTomorrowFetch == 0 || now - lastTomorrowFetch > 900000)) {
        try {
            lastTomorrowFetch = now;
            tomorrow = fetchPrices(t+SECS_PER_DAY);
        } catch(const std::exception& e) {
            if(lastError == 0) lastError = 900;
            tomorrow = NULL;
        }
        return tomorrow != NULL;
    }

    return false;
}

bool EntsoeApi::retrieve(const char* url, Stream* doc) {
    #if defined(ESP32)
        if(http.begin(url)) {
            printD("Connection established");

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            int status = http.GET();

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(status == HTTP_CODE_OK) {
                printD("Receiving data");
                http.writeToStream(doc);
                http.end();
                lastError = 0;
                return true;
            } else {
                lastError = status;
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("(EntsoeApi) Communication error, returned status: %d\n", status);
                printE(http.errorToString(status));
                printD(http.getString());

                http.end();
                return false;
            }
        } else {
            return false;
        }
    #endif
    return false;
}

float EntsoeApi::getCurrencyMultiplier(const char* from, const char* to, time_t t) {
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
        tmElements_t tm;
        breakTime(t, tm);
        lastCurrencyFetch = now + (SECS_PER_DAY * 1000) - (((((tm.Hour * 60) + tm.Minute) * 60) + tm.Second) * 1000);
    }
    return currencyMultiplier;
}

PricesContainer* EntsoeApi::fetchPrices(time_t t) {
    tmElements_t tm;
    breakTime(t, tm);
    if(strlen(getToken()) > 0) {
        time_t e1 = t - (tm.Hour * 3600) - (tm.Minute * 60) - tm.Second; // UTC midnight
        time_t e2 = e1 + SECS_PER_DAY;
        tmElements_t d1, d2;
        breakTime(tz->toUTC(e1), d1); // To get day and hour for CET/CEST at UTC midnight
        breakTime(tz->toUTC(e2), d2);

        snprintf(buf, BufferSize, "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
        "https://web-api.tp.entsoe.eu/api", getToken(), 
        d1.Year+1970, d1.Month, d1.Day, d1.Hour, 00,
        d2.Year+1970, d2.Month, d2.Day, d2.Hour, 00,
        config->area, config->area);

        #if defined(ESP32)
            esp_task_wdt_reset();
        #elif defined(ESP8266)
            ESP.wdtFeed();
        #endif

        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(EntsoeApi) Fetching prices for %d.%d.%d\n", tm.Day, tm.Month, tm.Year+1970);
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi)  url: %s\n", buf);
        EntsoeA44Parser a44;
        if(retrieve(buf, &a44) && a44.getPoint(0) != ENTSOE_NO_VALUE) {
            PricesContainer* ret = new PricesContainer();
            a44.get(ret);
            return ret;
        } else {
            return NULL;
        }
    } else if(hub) {
        String data;
        snprintf(buf, BufferSize, "%s/%s/%d/%d/%d?currency=%s",
            "http://hub.amsleser.no/hub/price",
            config->area,
            tm.Year+1970,
            tm.Month,
            tm.Day,
            config->currency
        );
        #if defined(ESP8266)
        WiFiClient client;
        client.setTimeout(5000);
        if(http.begin(client, buf)) {
        #elif defined(ESP32)
        if(http.begin(buf)) {
        #endif
            int status = http.GET();

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(status == HTTP_CODE_OK) {
                printD("Receiving data");
                data = http.getString();
                http.end();
                lastError = 0;
            } else {
                lastError = status;
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("(EntsoeApi) Communication error, returned status: %d\n", status);
                printE(http.errorToString(status));
                printD(http.getString());

                http.end();
            }
        }
        uint8_t* content = (uint8_t*) (data.c_str());
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            printD("Received content for prices:");
            debugPrint(content, 0, data.length());
        }

        DataParserContext ctx;
        ctx.length = data.length();
        GCMParser gcm(key, auth);
        int8_t gcmRet = gcm.parse(content, ctx);
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            printD("Decrypted content for prices:");
            debugPrint(content, 0, data.length());
        }
        if(gcmRet > 0) {
            if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EntsoeApi) Price data starting at: %d\n", gcmRet);
            PricesContainer* ret = new PricesContainer();
            memcpy(ret, content+gcmRet, sizeof(*ret));
            for(uint8_t i = 0; i < 24; i++) {
                ret->points[i] = ntohl(ret->points[i]);
            }
            lastError = 0;
            return ret;
        } else {
            lastError = gcmRet;
            if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("(EntsoeApi) Error code while decrypting prices: %d\n", gcmRet);
        }
    }
    return NULL;
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

void EntsoeApi::debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			debugger->print("0");
		debugger->print(buffer[i], HEX);
		debugger->print(" ");
		if ((i - start + 1) % 16 == 0)
			debugger->println("");
		else if ((i - start + 1) % 4 == 0)
			debugger->print(" ");

		yield(); // Let other get some resources too
	}
	debugger->println("");
}

int16_t EntsoeApi::getLastError() {
    return lastError;
}