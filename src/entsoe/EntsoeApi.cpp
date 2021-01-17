#include "EntsoeApi.h"
#include <EEPROM.h>
#include "Uptime.h"
#include "Time.h"
#include "DnbCurrParser.h"

#if defined(ESP8266)
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <HTTPClient.h>
#else
	#warning "Unsupported board type"
#endif

EntsoeApi::EntsoeApi(RemoteDebug* Debug) {
    debugger = Debug;

    // Entso-E uses CET/CEST
    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
	TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
	tz = new Timezone(CEST, CET);
}

void EntsoeApi::setup(EntsoeConfig& config) {
    if(this->config == NULL) {
        this->config = new EntsoeConfig();
    }
    memcpy(this->config, &config, sizeof(config));
}

char* EntsoeApi::getToken() {
    return this->config->token;
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
        multiplier *= getCurrencyMultiplier(tomorrow->getCurrency(), config->currency);
    } else {
        if(today == NULL)
            return ENTSOE_NO_VALUE;
        value = today->getPoint(pos);
        if(strcmp(today->getMeasurementUnit(), "MWH") == 0) {
            multiplier *= 0.001;
        } else {
            return ENTSOE_NO_VALUE;
        }
        multiplier *= getCurrencyMultiplier(today->getCurrency(), config->currency);
    }
    return value * multiplier;
}

bool EntsoeApi::loop() {
    if(strlen(config->token) == 0)
        return false;
    bool ret = false;

    uint64_t now = millis64();

    if(midnightMillis == 0) {
        time_t epoch = tz->toLocal(time(nullptr));
        
        tmElements_t tm;
        breakTime(epoch, tm);
        if(tm.Year > 50) { // Make sure we are in 2021 or later (years after 1970)
            uint64_t curDeviceMillis = millis64();
            uint32_t curDayMillis = (((((tm.Hour * 60) + tm.Minute) * 60) + tm.Second) * 1000);

            midnightMillis = curDeviceMillis + (SECS_PER_DAY * 1000) - curDayMillis;
            printD("Setting midnight millis " + String((uint32_t) midnightMillis));
        }
    } else if(now > midnightMillis) {
        printD("Rotating price objects");
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
            "https://transparency.entsoe.eu/api", config->token, 
            d1.Year+1970, d1.Month, d1.Day, 23, 00,
            d2.Year+1970, d2.Month, d2.Day, 23, 00,
            config->area, config->area);

            printD("Fetching prices for today");
            printD(url);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(url, a44)) {
                today = a44;
                ret = true;
            } else {
                delete a44;
                today = NULL;
            }
        }

        if(tomorrow == NULL
            && midnightMillis - now < 39600000
            && (lastTomorrowFetch == 0 || now - lastTomorrowFetch > 60000)
        ) {
            lastTomorrowFetch = now;
            time_t e1 = time(nullptr);
            time_t e2 = e1 + SECS_PER_DAY;
            tmElements_t d1, d2;
            breakTime(e1, d1);
            breakTime(e2, d2);

            char url[256];
            snprintf(url, sizeof(url), "%s?securityToken=%s&documentType=A44&periodStart=%04d%02d%02d%02d%02d&periodEnd=%04d%02d%02d%02d%02d&in_Domain=%s&out_Domain=%s", 
            "https://transparency.entsoe.eu/api", config->token, 
            d1.Year+1970, d1.Month, d1.Day, 23, 00,
            d2.Year+1970, d2.Month, d2.Day, 23, 00,
            config->area, config->area);

            printD("Fetching prices for tomorrow");
            printD(url);
            EntsoeA44Parser* a44 = new EntsoeA44Parser();
            if(retrieve(url, a44)) {
                tomorrow = a44;
                ret = true;
            } else {
                delete a44;
                tomorrow = NULL;
            }
        }
    }
    return ret;
}

bool EntsoeApi::retrieve(const char* url, Stream* doc) {
    WiFiClientSecure client;
    #if defined(ESP8266)
    //client.setBufferSizes(4096, 512);
    client.setInsecure();
    #endif
    
    HTTPClient https;
    #if defined(ESP8266)
    https.setFollowRedirects(true);
    #endif

    if(https.begin(client, url)) {
        printD("Connection established");
        //ESP.wdtDisable();
        int status = https.GET();
        //ESP.wdtEnable(5000);
        if(status == HTTP_CODE_OK) {
            printD("Receiving data");
            https.writeToStream(doc);
            https.end();
            return true;
        } else {
            printE("Communication error: ");
            printE(https.errorToString(status));
            printI(url);
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
    if(lastCurrencyFetch == 0 || now - lastCurrencyFetch > (SECS_PER_HOUR * 1000)) {
        char url[256];
        snprintf(url, sizeof(url), "https://data.norges-bank.no/api/data/EXR/M.%s.%s.SP?lastNObservations=1", 
            from,
            to
        );

        DnbCurrParser p;
        if(retrieve(url, &p)) {
            currencyMultiplier = p.getValue();
        }
        lastCurrencyFetch = now;
    }
    return currencyMultiplier;
}

void EntsoeApi::printD(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void EntsoeApi::printI(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void EntsoeApi::printW(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void EntsoeApi::printE(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(String("(EntsoeApi)" + fmt + "\n").c_str(), args);
	va_end(args);
}
