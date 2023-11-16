/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _CLOUDCONNECTOR_H
#define _CLOUDCONNECTOR_H

#include "RemoteDebug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/rsa.h"
#include "AmsConfiguration.h"
#include "AmsData.h"
#include "EnergyAccounting.h"
#include "HwTools.h"

#if defined(ESP8266)
	#include <ESP8266HTTPClient.h>
#elif defined(ESP32) // ARDUINO_ARCH_ESP32
	#include <HTTPClient.h>
    #include <esp_wifi.h>
    #include <esp_task_wdt.h>
    #include <WiFiUdp.h>
#else
	#warning "Unsupported board type"
#endif

#define CC_BUF_SIZE 1024

static const char CC_JSON_POWER[] PROGMEM = ",\"%s\":{\"P\":%lu,\"Q\":%lu}";
static const char CC_JSON_POWER_LIST3[] PROGMEM = ",\"%s\":{\"P\":%lu,\"Q\":%lu,\"tP\":%.3f,\"tQ\":%.3f}";
static const char CC_JSON_PHASE[] PROGMEM = "%s\"%d\":{\"u\":%.2f,\"i\":%.2f}";
static const char CC_JSON_PHASE_LIST4[] PROGMEM = "%s\"%d\":{\"u\":%.2f,\"i\":%.2f,\"Pim\":%lu,\"Pex\":%lu,\"pf\":%.2f}";

struct CloudData {
    uint8_t type;
	int16_t data;
} __attribute__((packed));

class CloudConnector {
public:
    CloudConnector(RemoteDebug*);
    void setup(CloudConfig& config, HwTools* hw);
    void update(AmsData& data, EnergyAccounting& ea);
    void forceUpdate();

private:
    RemoteDebug* debugger;
    HwTools* hw;
    CloudConfig config;
    HTTPClient http;
    WiFiUDP udp;
    bool initialized = false;
    unsigned long lastUpdate = 0;
    char mac[18];

    char clearBuffer[CC_BUF_SIZE];
    unsigned char encryptedBuffer[256];
    mbedtls_rsa_context* rsa = nullptr;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    char* pers = "amsreader";

    bool init();
    void debugPrint(byte *buffer, int start, int length);

    String meterManufacturer(uint8_t type) {
        switch(type) {
            case AmsTypeAidon: return F("Aidon");
            case AmsTypeKaifa: return F("Kaifa");
            case AmsTypeKamstrup: return F("Kamstrup");
            case AmsTypeIskra: return F("Iskra");
            case AmsTypeLandisGyr: return F("Landis+Gyr");
            case AmsTypeSagemcom: return F("Sagemcom");
        }
        return F("");
    }

};
#endif