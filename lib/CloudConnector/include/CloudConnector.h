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
#include "AmsMqttHandler.h"
#include "ConnectionHandler.h"

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

#define CC_BUF_SIZE 2048

static const char CC_JSON_POWER[] PROGMEM = ",\"%s\":{\"P\":%lu,\"Q\":%lu}";
static const char CC_JSON_POWER_LIST3[] PROGMEM = ",\"%s\":{\"P\":%lu,\"Q\":%lu,\"tP\":%.3f,\"tQ\":%.3f}";
static const char CC_JSON_PHASE[] PROGMEM = "%s\"%d\":{\"u\":%.2f,\"i\":%s}";
static const char CC_JSON_PHASE_LIST4[] PROGMEM = "%s\"%d\":{\"u\":%.2f,\"i\":%s,\"Pim\":%lu,\"Pex\":%lu,\"pf\":%.2f}";
static const char CC_JSON_STATUS[] PROGMEM = ",\"status\":{\"esp\":{\"state\":%d,\"error\":%d},\"han\":{\"state\":%d,\"error\":%d},\"wifi\":{\"state\":%d,\"error\":%d},\"mqtt\":{\"state\":%d,\"error\":%d}}";
static const char CC_JSON_INIT[] PROGMEM = ",\"init\":{\"mac\":\"%s\",\"apmac\":\"%s\",\"version\":\"%s\",\"boardType\":%d,\"bootReason\":%d,\"bootCause\":%d,\"tz\":\"%s\"},\"meter\":{\"manufacturerId\":%d,\"manufacturer\":\"%s\",\"model\":\"%s\",\"id\":\"%s\",\"system\":\"%s\",\"fuse\":%d,\"import\":%d,\"export\":%d},\"network\":{\"ip\":\"%s\",\"mask\":\"%s\",\"gw\":\"%s\",\"dns1\":\"%s\",\"dns2\":\"%s\"}";

struct CloudData {
    uint8_t type;
	int16_t data;
} __attribute__((packed));

class CloudConnector {
public:
    CloudConnector(RemoteDebug*);
    bool setup(CloudConfig& config, MeterConfig& meter, SystemConfig& system, NtpConfig& ntp, HwTools* hw, ResetDataContainer* rdc);
    void setMqttHandler(AmsMqttHandler* mqttHandler);
    void update(AmsData& data, EnergyAccounting& ea);
    void setPriceConfig(PriceServiceConfig&);
    void setEnergyAccountingConfig(EnergyAccountingConfig&);
    void forceUpdate();
    void setConnectionHandler(ConnectionHandler* ch);
    String generateSeed();

private:
    RemoteDebug* debugger = NULL;
    HwTools* hw = NULL;
    ConnectionHandler* ch = NULL;
    ResetDataContainer* rdc = NULL;
    AmsMqttHandler* mqttHandler = NULL;
    CloudConfig config;
    PriceServiceConfig priceConfig;
    unsigned long lastPriceConfig = 0;
    EnergyAccountingConfig eac;
    unsigned long lastEac = 0;
    HTTPClient http;
    WiFiUDP udp;
	int maxPwr = 0;
    uint8_t boardType = 0;
    char timezone[32];
	uint8_t distributionSystem = 0;
	uint16_t mainFuse = 0, productionCapacity = 0;

    String uuid;
    bool initialized = false;
    unsigned long lastUpdate = 0;
    char mac[18];
    char apmac[18];

    String seed = "";

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

    String distributionSystemStr(uint8_t ds) {
        switch(ds) {
            case 1: return F("IT");
            case 2: return F("TN");
        }
        return F("");
    }

};
#endif