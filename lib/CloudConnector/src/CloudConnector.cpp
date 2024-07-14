/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "CloudConnector.h"
#include "FirmwareVersion.h"
#include "crc.h"
#include "Uptime.h"
#include "hexutils.h"
#if defined(ESP32)
#include <ESPRandom.h>
#endif

#if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include "esp32/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/rtc.h"
#endif

#if defined(AMS_REMOTE_DEBUG)
CloudConnector::CloudConnector(RemoteDebug* debugger) {
#else
CloudConnector::CloudConnector(Stream* debugger) {
#endif
    this->debugger = debugger;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);
    http.setTimeout(60000);
    http.setUserAgent("ams2mqtt/" + String(FirmwareVersion::VersionString));
    http.useHTTP10(true);

    uint8_t mac[6];
    uint8_t apmac[6];

    #if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
    wifi_get_macaddr(SOFTAP_IF, apmac);
	#elif defined(ESP32)
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_AP, apmac);
	#endif
    sprintf_P(this->mac, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf_P(this->apmac, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), apmac[0], apmac[1], apmac[2], apmac[3], apmac[4], apmac[5]);
}

bool CloudConnector::setup(CloudConfig& config, MeterConfig& meter, SystemConfig& system, NtpConfig& ntp, HwTools* hw, ResetDataContainer* rdc) {
    bool ret = false;
    #if defined(ESP32)
    if(!ESPRandom::isValidV4Uuid(config.clientId)) {
        ESPRandom::uuid4(config.clientId);
        ret = true;
    }
    uuid = ESPRandom::uuidToString(config.clientId);
    #endif

    this->config = config;
    this->hw = hw;
    this->rdc = rdc;

    this->boardType = system.boardType;
    strcpy(this->timezone, ntp.timezone);

	this->maxPwr = 0;
	this->distributionSystem = meter.distributionSystem;
	this->mainFuse = meter.mainFuse;
	this->productionCapacity = meter.productionCapacity;

    this->initialized = false;

    return ret;
}

void CloudConnector::setMqttHandler(AmsMqttHandler* mqttHandler) {
    this->mqttHandler = mqttHandler;
}

bool CloudConnector::init() {
    if(config.enabled) {
        //if(config.port == 0) 
        config.port = 7443;
        //if(strlen(config.hostname) == 0) 
        strcpy_P(config.hostname, PSTR("cloud.amsleser.no"));

        snprintf_P(clearBuffer, CC_BUF_SIZE, PSTR("http://%s/hub/cloud/public.key"), config.hostname);
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("(CloudConnector) Downloading public key from %s\n"), clearBuffer);
        #if defined(ESP8266)
        WiFiClient client;
        client.setTimeout(5000);
        if(http.begin(client, clearBuffer)) {
        #elif defined(ESP32)
        if(http.begin(clearBuffer)) {
        #endif
            int status = http.GET();

            #if defined(ESP32)
                esp_task_wdt_reset();
            #elif defined(ESP8266)
                ESP.wdtFeed();
            #endif

            if(status == HTTP_CODE_OK) {
                String pub = http.getString();
                http.end();

                memset(clearBuffer, 0, CC_BUF_SIZE);
                snprintf(clearBuffer, CC_BUF_SIZE, pub.c_str());

                mbedtls_pk_context pk;
                mbedtls_pk_init(&pk);

                int error_code = 0;
                if((error_code =  mbedtls_pk_parse_public_key(&pk, (unsigned char*) clearBuffer, strlen((const char*) clearBuffer)+1)) == 0){
                    rsa = mbedtls_pk_rsa(pk);
                    mbedtls_ctr_drbg_init(&ctr_drbg);
                    mbedtls_entropy_init(&entropy);

                    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                                &entropy, (const unsigned char *) pers,
                                                strlen(pers));
                    if(ret != 0) {
                        #if defined(AMS_REMOTE_DEBUG)
                        if (debugger->isActive(RemoteDebug::ERROR))
                        #endif
                        debugger->printf_P(PSTR("mbedtls_ctr_drbg_seed return code: %d\n"), ret);
                    }
                    return ret == 0;
                } else {
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::ERROR))
                    #endif
                    debugger->printf("RSA public key read error: ");
                    mbedtls_strerror(error_code, clearBuffer, CC_BUF_SIZE);
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::ERROR))
                    #endif
                    debugger->printf("%s\n", clearBuffer);
                }
            } else {
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::ERROR))
                #endif
                debugger->printf_P(PSTR("(CloudConnector) Communication error, returned status: %d\n"), status);
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::ERROR))
                #endif
                debugger->printf(http.errorToString(status).c_str());
                debugger->println();
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                debugger->printf(http.getString().c_str());

                http.end();
            }
        }
    }
    return false;
}

void CloudConnector::update(AmsData& data, EnergyAccounting& ea) {
    if(!config.enabled) return;
    unsigned long now = millis();
    if(now-lastUpdate < config.interval*1000) return;
    if(!ESPRandom::isValidV4Uuid(config.clientId)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::WARNING))
        #endif
        debugger->printf_P(PSTR("(CloudConnector) Client ID is not valid\n"));
        return;
    }
    if(data.getListType() < 2) return;

    if(!initialized && !init()) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::WARNING))
        #endif
        debugger->printf_P(PSTR("Unable to initialize cloud connector\n"));
        lastUpdate = now;
        config.enabled = false;
        return;
    }
    initialized = true;

    memset(clearBuffer, 0, CC_BUF_SIZE);

    int pos = 0;

    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("{\"id\":\"%s\""), uuid.c_str());
    if(!seed.isEmpty()) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"seed\":\"%s\""), seed.c_str());
    }

    if(lastUpdate == 0) {
        seed.clear();
        if(mainFuse > 0 && distributionSystem > 0) {
            int voltage = distributionSystem == 2 ? 400 : 230;
            if(data.isThreePhase()) {
                maxPwr = mainFuse * sqrt(3) * voltage;
            } else if(data.isTwoPhase()) {
                maxPwr = mainFuse * voltage;
            } else {
                maxPwr = mainFuse * 230;
            }
        }

        IPAddress localIp;
        IPAddress subnet;
        IPAddress gateway;
        IPAddress dns1;
        IPAddress dns2;

        if(ch == NULL) {
            localIp = WiFi.localIP();
            subnet = IPAddress(255,255,255,0);
            gateway = WiFi.subnetMask();
            dns1 = WiFi.dnsIP(0);
            dns2 = WiFi.dnsIP(1);
        } else {
            localIp = ch->getIP();
            subnet = ch->getSubnetMask();
            gateway = ch->getGateway();
            dns1 = ch->getDns(0);
            dns2 = ch->getDns(1);
        }

        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_INIT,
            mac, 
            apmac, 
            FirmwareVersion::VersionString,
            boardType,
            rtc_get_reset_reason(0),
            rdc == NULL ? 0 : rdc->last_cause,
            timezone,
            data.getMeterType(),
            meterManufacturer(data.getMeterType()).c_str(),
            data.getMeterModel().c_str(),
            data.getMeterId().c_str(),
            distributionSystemStr(distributionSystem).c_str(),
            mainFuse,
            maxPwr,
            productionCapacity,
            localIp.toString().c_str(),
            subnet.toString().c_str(),
            gateway.toString().c_str(),
            dns1.toString().c_str(),
            dns2.toString().c_str()
        );
    } else if(lastPriceConfig == 0) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"price\":{\"area\":\"%s\",\"currency\":\"%s\"}"), priceConfig.area, priceConfig.currency);
        lastPriceConfig = now;
    } else if(lastEac == 0) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"accounting\":{\"hours\":%d,\"thresholds\":[%d,%d,%d,%d,%d,%d,%d,%d,%d]}"), 
            eac.hours,
            eac.thresholds[0],
            eac.thresholds[1],
            eac.thresholds[2],
            eac.thresholds[3],
            eac.thresholds[4],
            eac.thresholds[5],
            eac.thresholds[6],
            eac.thresholds[7],
            eac.thresholds[8]
        );
        lastEac = now;
    }

    float vcc = 0.0;
    int rssi = 0;
    float temperature = -127;
    if(hw != NULL) {
        vcc = hw->getVcc();
        rssi = hw->getWifiRssi();
        temperature = hw->getTemperature();
    }

    uint8_t espStatus;
    #if defined(ESP8266)
    if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
        espStatus = 1;
    } else if(vcc > 2.8 && vcc < 3.5) {
        espStatus = 1;
    } else if(vcc > 2.7 && vcc < 3.6) {
        espStatus = 2;
    } else {
        espStatus = 3;
    }
    #elif defined(ESP32)
    if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
        espStatus = 1;
    } else if(vcc > 3.1 && vcc < 3.5) {
        espStatus = 1;
    } else if(vcc > 3.0 && vcc < 3.6) {
        espStatus = 2;
    } else {
        espStatus = 3;
    }
    #endif

    uint8_t hanStatus;
    if(data.getLastError() != 0) {
        hanStatus = 3;
    } else if(data.getLastUpdateMillis() == 0 && now < 30000) {
        hanStatus = 0;
    } else if(now - data.getLastUpdateMillis() < 15000) {
        hanStatus = 1;
    } else if(now - data.getLastUpdateMillis() < 30000) {
        hanStatus = 2;
    } else {
        hanStatus = 3;
    }

    uint8_t wifiStatus;
    if(rssi > -75) {
        wifiStatus = 1;
    } else if(rssi > -95) {
        wifiStatus = 2;
    } else {
        wifiStatus = 3;
    }

    uint8_t mqttStatus;
    if(mqttHandler == NULL) {
        mqttStatus = 0;
    } else if(mqttHandler->connected()) {
        mqttStatus = 1;
    } else if(mqttHandler->lastError() == 0) {
        mqttStatus = 2;
    } else {
        mqttStatus = 3;
    }

    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"data\":{\"clock\":%lu,\"up\":%lu,\"lastUpdate\":%lu,\"est\":%s"), 
        (uint32_t) time(nullptr),
        (uint32_t) (millis64()/1000),
        (uint32_t) (data.getLastUpdateMillis()/1000),
        data.isCounterEstimated() ? "true" : "false"
    );
    if(data.getListType() > 2) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_POWER_LIST3, "import", data.getActiveImportPower(), data.getReactiveImportPower(), data.getActiveImportCounter(), data.getReactiveImportCounter());
    } else {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_POWER, "import", data.getActiveImportPower(), data.getReactiveImportPower());
    }
    if(data.getListType() > 2) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_POWER_LIST3, "export", data.getActiveExportPower(), data.getReactiveExportPower(), data.getActiveExportCounter(), data.getReactiveExportCounter());
    } else {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_POWER, "export", data.getActiveExportPower(), data.getReactiveExportPower());
    }
    
    if(data.getListType() > 1) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"phases\":{"));
        bool first = true;
        if(data.getL1Voltage() > 0.0) {
            if(data.getListType() > 3) {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 1, data.getL1Voltage(), String(data.getL1Current(), 2).c_str(), data.getL1ActiveImportPower(), data.getL1ActiveExportPower(), data.getL1PowerFactor());
            } else {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 1, data.getL1Voltage(), String(data.getL1Current(), 2).c_str());
            }
            first = false;
        }
        if(data.getL2Voltage() > 0.0) {
            if(data.getListType() > 3) {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 2, data.getL2Voltage(), String(data.getL2Current(), 2).c_str(), data.getL2ActiveImportPower(), data.getL2ActiveExportPower(), data.getL2PowerFactor());
            } else {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 2, data.getL2Voltage(), data.isL2currentMissing() ? "null" : String(data.getL2Current(), 2).c_str());
            }
            first = false;
        }
        if(data.getL3Voltage() > 0.0) {
            if(data.getListType() > 3) {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 3, data.getL3Voltage(), String(data.getL3Current(), 2).c_str(), data.getL3ActiveImportPower(), data.getL3ActiveExportPower(), data.getL3PowerFactor());
            } else {
                pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 3, data.getL3Voltage(), String(data.getL3Current(), 2).c_str());
            }
            first = false;
        }
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("}"));
    }
    if(data.getListType() > 3) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"pf\":%.2f"), data.getPowerFactor());
    }

    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"realtime\":{\"import\":%.3f,\"export\":%.3f}"), ea.getUseThisHour(), ea.getProducedThisHour());
    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"vcc\":%.2f,\"temp\":%.2f,\"rssi\":%d,\"free\":%d"), vcc, temperature, rssi, ESP.getFreeHeap());
    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_STATUS, 
        espStatus, 0,
        hanStatus, data.getLastError(),
        wifiStatus, 0,
        mqttStatus, mqttHandler == NULL ? 0 : mqttHandler->lastError()
    );

    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("}"));

    uint16_t crc = crc16((uint8_t*) clearBuffer, pos);
    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"crc\":\"%04X\"}"), crc);

    if(rsa == nullptr) return;
    int ret = mbedtls_rsa_check_pubkey(rsa);
    if(ret != 0) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("mbedtls_rsa_pkcs1_encrypt return code: %d\n"), ret);
        mbedtls_strerror(ret, clearBuffer, CC_BUF_SIZE);
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("%s\n"), clearBuffer);
        return;
    }
    memset(encryptedBuffer, 0, rsa->len);

    int maxlen = rsa->len - 11; // 11 should be the correct padding size for PKCS1
    udp.beginPacket(config.hostname,7443);
    for(int i = 0; i < pos; i += maxlen) {
        int ret = mbedtls_rsa_pkcs1_encrypt(rsa, mbedtls_ctr_drbg_random, &ctr_drbg, MBEDTLS_RSA_PUBLIC, maxlen, (unsigned char*) (clearBuffer+i), encryptedBuffer);
        if(ret == 0) {
            udp.write(encryptedBuffer, rsa->len);
            delay(1);
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("mbedtls_rsa_pkcs1_encrypt return code: %d\n"), ret);
            mbedtls_strerror(ret, clearBuffer, CC_BUF_SIZE);
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("%s\n"), clearBuffer);
        }
    }
    udp.endPacket();

    lastUpdate = now;
}

void CloudConnector::forceUpdate() {
    lastUpdate = 0;
    lastPriceConfig = 0;
    lastEac = 0;
}

void CloudConnector::setConnectionHandler(ConnectionHandler* ch) {
	this->ch = ch;
}

void CloudConnector::setPriceConfig(PriceServiceConfig& priceConfig) {
    this->priceConfig = priceConfig;
    this->lastPriceConfig = 0;
}

void CloudConnector::setEnergyAccountingConfig(EnergyAccountingConfig& eac) {
    this->eac = eac;
    this->lastEac = 0;
}

void CloudConnector::debugPrint(byte *buffer, int start, int length) {
	for (int i = start; i < start + length; i++) {
		if (buffer[i] < 0x10)
			debugger->print(F("0"));
		debugger->print(buffer[i], HEX);
		debugger->print(F(" "));
		if ((i - start + 1) % 16 == 0)
			debugger->println(F(""));
		else if ((i - start + 1) % 4 == 0)
			debugger->print(F(" "));

		yield(); // Let other get some resources too
	}
	debugger->println(F(""));
}

String CloudConnector::generateSeed() {
    uint8_t key[16];
    ESPRandom::uuid4(key);
    seed = ESPRandom::uuidToString(key);
    return seed;
}
