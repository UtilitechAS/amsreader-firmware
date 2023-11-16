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

CloudConnector::CloudConnector(RemoteDebug* debugger) {
    this->debugger = debugger;

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);
    http.setTimeout(60000);
    http.setUserAgent("ams2mqtt/" + String(FirmwareVersion::VersionString));
    http.useHTTP10(true);

    uint8_t mac[6];

    #if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
	#elif defined(ESP32)
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
	#endif
    sprintf(this->mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void CloudConnector::setup(CloudConfig& config, HwTools* hw) {
    this->config = config;
    this->hw = hw;
}

bool CloudConnector::init() {
    if(config.enabled && strlen(config.hostname) > 0 && config.port > 0) {
        snprintf_P(clearBuffer, CC_BUF_SIZE, PSTR("http://%s/hub/cloud/public.key"), config.hostname);
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf("(CloudConnector) Downloading public key from %s\n", clearBuffer);
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

                if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("Cloud public key:\n%s\n", clearBuffer);

                mbedtls_pk_context pk;
                mbedtls_pk_init(&pk);

                int error_code = 0;
                if((error_code =  mbedtls_pk_parse_public_key(&pk, (unsigned char*) clearBuffer, strlen((const char*) clearBuffer)+1)) == 0){
                    if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("RSA public key OK\n");
                    rsa = mbedtls_pk_rsa(pk);
                    mbedtls_ctr_drbg_init(&ctr_drbg);
                    mbedtls_entropy_init(&entropy);

                    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                                &entropy, (const unsigned char *) pers,
                                                strlen(pers));
                    if(ret != 0) {
                        if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("mbedtls_ctr_drbg_seed return code: %d\n", ret);
                    }
                    return ret == 0;
                } else {
                    if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("RSA public key read error: ");
                    mbedtls_strerror(error_code, clearBuffer, CC_BUF_SIZE);
                    if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("%s\n", clearBuffer);
                }
            } else {
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("(CloudConnector) Communication error, returned status: %d\n"), status);
                if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(http.errorToString(status).c_str());
                if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(http.getString().c_str());

                http.end();
            }
        }
    }
    return false;
}

void CloudConnector::update(AmsData& data, EnergyAccounting& ea) {
    if(!config.enabled || strlen(config.hostname) == 0 || config.port == 0) return;
    unsigned long now = millis();
    if(now-lastUpdate < config.interval*1000) return;
    lastUpdate = now;
    if(strlen(config.clientId) == 0 || strlen(config.clientSecret) == 0) {
        if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf("(CloudConnector) Client ID and secret is missing\n");
        return;
    }
    if(data.getListType() < 2) return;

    memset(clearBuffer, 0, CC_BUF_SIZE);

    int pos = 0;
    if(initialized) {
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("{\"id\":\"%s\",\"secret\":\"%s\",\"data\":{\"clock\":%lu,\"up\":%lu,\"lastUpdate\":%lu,\"est\":%s"), 
            config.clientId,
            config.clientSecret,
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
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 1, data.getL1Voltage(), data.getL1Current(), data.getL1ActiveImportPower(), data.getL1ActiveExportPower(), data.getL1PowerFactor());
                } else {
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 1, data.getL1Voltage(), data.getL1Current());
                }
                first = false;
            }
            if(data.getL2Voltage() > 0.0) {
                if(data.getListType() > 3) {
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 2, data.getL2Voltage(), data.getL2Current(), data.getL2ActiveImportPower(), data.getL2ActiveExportPower(), data.getL2PowerFactor());
                } else {
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 2, data.getL2Voltage(), data.getL2Current());
                }
                first = false;
            }
            if(data.getL3Voltage() > 0.0) {
                if(data.getListType() > 3) {
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE_LIST4, first ? "" : ",", 3, data.getL3Voltage(), data.getL3Current(), data.getL3ActiveImportPower(), data.getL3ActiveExportPower(), data.getL3PowerFactor());
                } else {
                    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, CC_JSON_PHASE, first ? "" : ",", 3, data.getL3Voltage(), data.getL3Current());
                }
                first = false;
            }
            pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("}"));
        }
        if(data.getListType() > 3) {
            pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"pf\":%.2f"), data.getPowerFactor());
        }

        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"realtime\":{\"import\":%.3f,\"export\":%.3f}"), ea.getUseThisHour(), ea.getProducedThisHour());
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"vcc\":%.2f,\"temp\":%.2f,\"rssi\":%d,\"free\":%d"), hw->getVcc(), hw->getTemperature(), hw->getWifiRssi(), ESP.getFreeHeap());

        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("}"));
    } else {
        if(!init()) {
            if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf("Unable to initialize cloud connector\n");
            return;
        }
        pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR("{\"id\":\"%s\",\"secret\":\"%s\",\"init\":{\"mac\":\"%s\",\"version\":\"%s\"},\"meter\":{\"manufacturer\":\"%s\",\"model\":\"%s\",\"id\":\"%s\"}"), 
            config.clientId, 
            config.clientSecret, 
            mac, 
            FirmwareVersion::VersionString,
            meterManufacturer(data.getMeterType()).c_str(),
            data.getMeterModel().c_str(),
            data.getMeterId().c_str()
        );
        initialized = true;
    }
    uint16_t crc = crc16((uint8_t*) clearBuffer, pos);
    pos += snprintf_P(clearBuffer+pos, CC_BUF_SIZE-pos, PSTR(",\"crc\":\"%04X\"}"), crc);

    if(rsa == nullptr) return;
    int ret = mbedtls_rsa_check_pubkey(rsa);
    if(ret != 0) {
        if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("mbedtls_rsa_pkcs1_encrypt return code: %d\n", ret);
        mbedtls_strerror(ret, clearBuffer, CC_BUF_SIZE);
        if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("%s\n", clearBuffer);
        return;
    }
    memset(encryptedBuffer, 0, rsa->len);

    int maxlen = 100 * (rsa->len/128);
    if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(CloudConnector) Sending %d bytes and maxlen %d\n", pos, maxlen);
    udp.beginPacket(config.hostname,7443);
    for(int i = 0; i < pos; i += maxlen) {
        int size = min(maxlen, pos-i);
        int ret = mbedtls_rsa_pkcs1_encrypt(rsa, mbedtls_ctr_drbg_random, &ctr_drbg, MBEDTLS_RSA_PUBLIC, size, (unsigned char*) (clearBuffer+i), encryptedBuffer);
        if(ret == 0) {
            udp.write(encryptedBuffer, rsa->len);
            delay(1);
        } else {
            if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("mbedtls_rsa_pkcs1_encrypt return code: %d\n", ret);
            mbedtls_strerror(ret, clearBuffer, CC_BUF_SIZE);
            if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf("%s\n", clearBuffer);
        }
    }
    udp.endPacket();
}

void CloudConnector::forceUpdate() {
    lastUpdate = 0;
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
