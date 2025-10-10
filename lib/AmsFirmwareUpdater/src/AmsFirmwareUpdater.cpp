#include "AmsFirmwareUpdater.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"
#include "UpgradeDefaults.h"
#include <ArduinoJson.h>
#include <cstring>
#include <TimeLib.h>

#if defined(ESP32)
#include "esp_ota_ops.h"
#include "esp_littlefs.h"
#include "driver/spi_common.h"
#include "esp_flash_spi_init.h"
#include "MD5Builder.h"
#elif defined(ESP8266)
#include "flash_hal.h"
#include "eboot_command.h"
#endif

#if defined(AMS_REMOTE_DEBUG)
AmsFirmwareUpdater::AmsFirmwareUpdater(RemoteDebug* debugger, HwTools* hw, AmsData* meterState, AmsConfiguration* configuration) {
#else
AmsFirmwareUpdater::AmsFirmwareUpdater(Print* debugger, HwTools* hw, AmsData* meterState, AmsConfiguration* configuration) {
#endif
this->debugger = debugger;
    this->hw = hw;
    this->meterState = meterState;
    this->configuration = configuration;
    memset(nextVersion, 0, sizeof(nextVersion));
    firmwareVariant = 0;
    autoUpgrade = false;
}

char* AmsFirmwareUpdater::getNextVersion() {
    return nextVersion;
}

bool AmsFirmwareUpdater::setTargetVersion(const char* version) {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Preparing upgrade to %s\n"), version);

    if(strcmp(version, FirmwareVersion::VersionString) == 0) {
        memset(updateStatus.toVersion, 0, sizeof(updateStatus.toVersion));
        return false;
    }
    if(strcmp(version, updateStatus.toVersion) == 0 && updateStatus.errorCode == AMS_UPDATE_ERR_OK) {
        return true;
    }
    strcpy(updateStatus.fromVersion, FirmwareVersion::VersionString);
    strcpy(updateStatus.toVersion, version);
    updateStatus.size = 0;
    updateStatus.retry_count = 0;
    updateStatus.block_position = 0;
    updateStatus.errorCode = AMS_UPDATE_ERR_OK;
    updateStatus.reboot_count = 0;

    if(buf == NULL) buf = (uint8_t*) malloc(UPDATE_BUF_SIZE);
    memset(buf, 0, UPDATE_BUF_SIZE);
    bufPos = 0;

    return true;
}

void AmsFirmwareUpdater::getUpgradeInformation(UpgradeInformation& upinfo) {
    memcpy(&upinfo, &updateStatus, sizeof(upinfo));
}

void AmsFirmwareUpdater::setUpgradeInformation(UpgradeInformation& upinfo) {
    memcpy(&updateStatus, &upinfo, sizeof(updateStatus));
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Upgrade status information updated\n"));

    if(strlen(updateStatus.toVersion) > 0 && strcmp(updateStatus.toVersion, FirmwareVersion::VersionString) == 0 && updateStatus.errorCode >= AMS_UPDATE_ERR_SUCCESS_SIGNAL) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Upgrade to %s already applied, clearing status\n"), updateStatus.toVersion);
        memset(updateStatus.fromVersion, 0, sizeof(updateStatus.fromVersion));
        memset(updateStatus.toVersion, 0, sizeof(updateStatus.toVersion));
        updateStatus.size = 0;
        updateStatus.block_position = 0;
        updateStatus.retry_count = 0;
        updateStatus.reboot_count = 0;
        updateStatus.errorCode = AMS_UPDATE_ERR_OK;
        updateStatusChanged = true;
    }
    if(strlen(updateStatus.toVersion) > 0 && updateStatus.size > 0 && updateStatus.block_position * UPDATE_BUF_SIZE < updateStatus.size) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Resuming uprade to %s\n"), updateStatus.toVersion);

        if(updateStatus.reboot_count++ < 8) {
            updateStatus.errorCode = AMS_UPDATE_ERR_OK;
        } else {
            updateStatus.errorCode = AMS_UPDATE_ERR_REBOOT;
        }
        updateStatusChanged = true;
    }
}

bool AmsFirmwareUpdater::isUpgradeInformationChanged() {
    return updateStatusChanged;
}

void AmsFirmwareUpdater::ackUpgradeInformationChanged() {
    updateStatusChanged = false;
}

float AmsFirmwareUpdater::getProgress() {
    if(strlen(updateStatus.toVersion) == 0 || updateStatus.size == 0 || updateStatus.errorCode >= AMS_UPDATE_ERR_SUCCESS_SIGNAL) return -1.0;
    return min((float) 100.0, ((((float) updateStatus.block_position) * UPDATE_BUF_SIZE) / updateStatus.size) * 100);
}

void AmsFirmwareUpdater::loop() {
    refreshUpgradeConfig();
    if(millis() < 30000) {
        // Wait 30 seconds before starting upgrade. This allows the device to deal with other tasks first
        // It will also allow BUS powered devices to reach a stable voltage so that hw->isVoltageOptimal will behave properly
        return; 
    }
    if(strlen(updateStatus.toVersion) > 0 && updateStatus.errorCode == AMS_UPDATE_ERR_OK) {
        if(!hw->isVoltageOptimal(0.1)) {
            writeUpdateStatus();
            return;
        }

        unsigned long start = 0, end = 0;

        if(buf == NULL) buf = (uint8_t*) malloc(UPDATE_BUF_SIZE);
        if(updateStatus.size == 0) {
            start = millis();
            if(!fetchVersionDetails()) {
                updateStatus.errorCode = AMS_UPDATE_ERR_DETAILS;
                return;
            }
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("fetch details took %lums\n"), end-start);
        } else if(updateStatus.block_position * UPDATE_BUF_SIZE < updateStatus.size) {
            HTTPClient http;
            start = millis();
            if(!fetchFirmwareChunk(http)) {
                if(updateStatus.retry_count++ == 3) {
                    updateStatus.errorCode = AMS_UPDATE_ERR_FETCH;
                    updateStatusChanged = true;
                }
                writeUpdateStatus();
                http.end();
                return;
            }
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("fetch chunk took %lums\n"), end-start);

            WiFiClient* client = http.getStreamPtr();
            if(client == NULL) {
                http.end();
                return;
            }
            client->setTimeout(15000);

            uint32_t remaining = 0;
            if(updateStatus.size > updateStatus.block_position * UPDATE_BUF_SIZE) {
                remaining = updateStatus.size - (updateStatus.block_position * UPDATE_BUF_SIZE);
            }

            if(remaining == 0) {
                http.end();
                return;
            }

            size_t expected = remaining > UPDATE_BUF_SIZE ? UPDATE_BUF_SIZE : remaining;
            memset(buf, 0, UPDATE_BUF_SIZE);

            size_t totalRead = 0;
            uint32_t idleDeadline = millis() + 20000;
            uint32_t overallDeadline = millis() + 120000;
            while(totalRead < expected) {
                size_t available = client->available();
                if(available > 0) {
                    size_t toRead = expected - totalRead;
                    if(toRead > available) {
                        toRead = available;
                    }
                    size_t chunk = client->read((uint8_t*) buf + totalRead, toRead);
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::DEBUG))
                    #endif
                    debugger->printf_P(PSTR("read buffer (%lu bytes, %d left)\n"), chunk, client->available());
                    if(chunk > 0) {
                        totalRead += chunk;
                        idleDeadline = millis() + 20000;
                        continue;
                    }
                }

                if(!client->connected() && available == 0) {
                    break;
                }

                uint32_t now = millis();
                if((int32_t) (now - idleDeadline) >= 0 || (int32_t) (now - overallDeadline) >= 0) {
                    break;
                }

                delay(50);
            }

            if(totalRead != expected) {
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::WARNING))
                #endif
                debugger->printf_P(PSTR("Expected %lu bytes but received %lu (connected=%d, available=%d)\n"), expected, totalRead, client->connected(), client->available());
                http.end();
                if(updateStatus.retry_count++ == 3) {
                    updateStatus.errorCode = AMS_UPDATE_ERR_FETCH;
                    updateStatusChanged = true;
                }
                return;
            }

            start = millis();
            if(!writeBufferToFlash(totalRead)) {
                http.end();
                return;
            }
            updateStatus.retry_count = 0;
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("write buffer took %lums\n"), end-start);

            start = millis();
            if(!hw->isVoltageOptimal(0.2)) {
                writeUpdateStatus();
            }
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("check voltage took %lums\n"), end-start);
            start = millis();
            http.end();
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("http end took %lums\n"), end-start);
        } else if(updateStatus.block_position * UPDATE_BUF_SIZE >= updateStatus.size) {
            if(!completeFirmwareUpload(updateStatus.size)) return;
            updateStatus.errorCode = AMS_UPDATE_ERR_SUCCESS_SIGNAL;
            updateStatusChanged = true;
        }
    } else {
        uint32_t seconds = millis() / 1000.0;
        if((lastVersionCheck == 0 && seconds > 20) || seconds - lastVersionCheck > 86400) {
            fetchNextVersion();
            lastVersionCheck = seconds;
        }

        if(shouldTriggerAutoUpgrade()) {
            if(setTargetVersion(nextVersion)) {
                updateStatus.size = 0;
                updateStatusChanged = true;
                if(nextAutoAttemptDay != 0) {
                    lastAutoAttemptDay = nextAutoAttemptDay;
                    nextAutoAttemptDay = 0;
                }
            }
        }
    }
}

void AmsFirmwareUpdater::setUpgradeConfig(const UpgradeConfig& cfg) {
    bool changed = !upgradeConfigInitialized ||
                   upgradeConfig.autoUpgrade != cfg.autoUpgrade ||
                   upgradeConfig.windowStartHour != cfg.windowStartHour ||
                   upgradeConfig.windowEndHour != cfg.windowEndHour;

    upgradeConfig = cfg;
    upgradeConfigInitialized = true;
    if(upgradeConfig.windowStartHour >= 24) upgradeConfig.windowStartHour %= 24;
    if(upgradeConfig.windowEndHour >= 24) upgradeConfig.windowEndHour %= 24;
    autoUpgrade = upgradeConfig.autoUpgrade;

    if(changed) {
        nextAutoAttemptDay = 0;
        lastAutoAttemptDay = 0;
    }
}

void AmsFirmwareUpdater::setTimezone(Timezone* tz) {
    this->tz = tz;
}

void AmsFirmwareUpdater::refreshUpgradeConfig() {
    if(configuration == NULL) {
        return;
    }

    bool shouldRefresh = !upgradeConfigInitialized || configuration->isUpgradeConfigChanged();
    if(!shouldRefresh) {
        return;
    }

    UpgradeConfig cfg;
    bool hasConfig = configuration->getUpgradeConfig(cfg);
    if(!hasConfig) {
        configuration->clearUpgradeConfig(cfg);
        configuration->setUpgradeConfig(cfg);
        configuration->getUpgradeConfig(cfg);
    }

    setUpgradeConfig(cfg);
    if(configuration->isUpgradeConfigChanged()) {
        configuration->ackUpgradeConfig();
    }
}

bool AmsFirmwareUpdater::shouldTriggerAutoUpgrade() {
    nextAutoAttemptDay = 0;

    if(!autoUpgrade) return false;
    if(tz == NULL) return false;
    if(strlen(nextVersion) == 0) return false;
    if(currentVersionMatchesLatest) return false;
    if(strlen(updateStatus.toVersion) > 0) return false;
    if(!hw->isVoltageOptimal(0.2)) return false;

    time_t nowUtc = now();
    if(nowUtc <= 0) return false;

    time_t localTime = tz->toLocal(nowUtc);
    if(!isWithinAutoWindow(localTime)) return false;

    tmElements_t tm;
    breakTime(localTime, tm);
    time_t midnight = localTime - (tm.Hour * 3600UL + tm.Minute * 60UL + tm.Second);

    if(lastAutoAttemptDay == midnight) {
        return false;
    }

    nextAutoAttemptDay = midnight;
    return true;
}

bool AmsFirmwareUpdater::isWithinAutoWindow(time_t localTime) const {
    uint8_t start = upgradeConfig.windowStartHour % 24;
    uint8_t end = upgradeConfig.windowEndHour % 24;

    tmElements_t tm;
    breakTime(localTime, tm);
    uint8_t hour = tm.Hour % 24;

    if(start == end) {
        return hour == start;
    }

    if(start < end) {
        return hour >= start && hour < end;
    }

    // Window wraps around midnight
    return hour >= start || hour < end;
}

bool AmsFirmwareUpdater::computeCurrentVersionMatch() {
#if FIRMWARE_UPDATE_USE_MANIFEST
    if(!manifestInfo.loaded || manifestInfo.version.isEmpty()) {
        return false;
    }

    if(manifestInfo.version.equalsIgnoreCase(FirmwareVersion::VersionString)) {
        return true;
    }

    if(manifestInfo.md5.length() > 0) {
        String sketchMD5 = ESP.getSketchMD5();
        if(!sketchMD5.isEmpty()) {
            return sketchMD5.equalsIgnoreCase(manifestInfo.md5);
        }
    }

    return false;
#else
    return false;
#endif
}

bool AmsFirmwareUpdater::fetchNextVersion() {
#if FIRMWARE_UPDATE_USE_MANIFEST
    if(!loadManifest(true)) {
        currentVersionMatchesLatest = false;
        return false;
    }
    if(manifestInfo.version.isEmpty()) {
        currentVersionMatchesLatest = false;
        return false;
    }

    currentVersionMatchesLatest = computeCurrentVersionMatch();
    if(currentVersionMatchesLatest) {
        memset(nextVersion, 0, sizeof(nextVersion));
        return false;
    }

    strncpy(nextVersion, manifestInfo.version.c_str(), sizeof(nextVersion) - 1);
    nextVersion[sizeof(nextVersion) - 1] = '\0';
    return strlen(nextVersion) > 0;
#else
    HTTPClient http;
    const char * headerkeys[] = { "x-version" };
    http.collectHeaders(headerkeys, 1);

    const char* firmwareVariant = FIRMWARE_UPDATE_CHANNEL;

    char url[256];
    snprintf(url, sizeof(url), "%s/firmware/%s/%s/next", FIRMWARE_UPDATE_BASE_URL, chipType, firmwareVariant);
    #if defined(ESP8266)
    WiFiClient client;
    client.setTimeout(5000);
    if(http.begin(client, url)) {
    #elif defined(ESP32)
    if(http.begin(url)) {
    #endif
    http.useHTTP10(true);
    http.setTimeout(60000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setUserAgent(FIRMWARE_UPDATE_USER_AGENT);
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
        int status = http.GET();
        if(status == 204) {
            String nextVersion = http.header("x-version");
            strcpy(this->nextVersion, nextVersion.c_str());
            currentVersionMatchesLatest = nextVersion.equalsIgnoreCase(FirmwareVersion::VersionString);
            if(currentVersionMatchesLatest) {
                memset(this->nextVersion, 0, sizeof(this->nextVersion));
            }
            http.end();
            return strlen(this->nextVersion) > 0;
        } else if(status == 200) {
            memset(this->nextVersion, 0, sizeof(this->nextVersion));
            currentVersionMatchesLatest = true;
        }
        http.end();
    }
    return false;
#endif
}

bool AmsFirmwareUpdater::fetchVersionDetails() {
#if FIRMWARE_UPDATE_USE_MANIFEST
    if(!loadManifest(false)) {
        return false;
    }
    if(manifestInfo.version.isEmpty() || manifestInfo.size == 0) {
        return false;
    }

    updateStatus.size = manifestInfo.size;
    if(manifestInfo.md5.length() > 0) {
        md5 = manifestInfo.md5;
    } else {
        md5 = F("unknown");
    }
    return true;
#else
    HTTPClient http;
    const char * headerkeys[] = { "x-size" };
    http.collectHeaders(headerkeys, 1);

    const char* firmwareVariant = FIRMWARE_UPDATE_CHANNEL;

    char url[256];
    snprintf(url, sizeof(url), "%s/firmware/%s/%s/%s/details", FIRMWARE_UPDATE_BASE_URL, chipType, firmwareVariant, updateStatus.toVersion);
    #if defined(ESP8266)
    WiFiClient client;
    client.setTimeout(5000);
    if(http.begin(client, url)) {
    #elif defined(ESP32)
    if(http.begin(url)) {
    #endif
        http.useHTTP10(true);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setUserAgent(FIRMWARE_UPDATE_USER_AGENT);
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-STA-MAC"), WiFi.macAddress());
        http.addHeader(F("x-AMS-AP-MAC"), WiFi.softAPmacAddress());
        http.addHeader(F("x-AMS-free-space"), String(ESP.getFreeSketchSpace()));
        http.addHeader(F("x-AMS-sketch-size"), String(ESP.getSketchSize()));
        String sketchMD5 = ESP.getSketchMD5();
        if(!sketchMD5.isEmpty()) {
            http.addHeader(F("x-AMS-sketch-md5"), sketchMD5);
        }
        http.addHeader(F("x-AMS-chip-size"), String(ESP.getFlashChipSize()));
        http.addHeader(F("x-AMS-sdk-version"), ESP.getSdkVersion());
        http.addHeader(F("x-AMS-mode"), "sketch");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
		http.addHeader(F("x-AMS-board-type"), String(hw->getBoardType(), 10));
		if(meterState->getMeterType() != AmsTypeAutodetect) {
			http.addHeader(F("x-AMS-meter-mfg"), String(meterState->getMeterType(), 10));
		}
		if(!meterState->getMeterModel().isEmpty()) {
			http.addHeader(F("x-AMS-meter-model"), meterState->getMeterModel());
		}
        int status = http.GET();
        if(status == 204) {
            String size = http.header("x-size");
            updateStatus.size = size.toInt();
            http.end();
            return true;
        }
        http.end();
    }
    return false;
#endif
}

bool AmsFirmwareUpdater::fetchFirmwareChunk(HTTPClient& http) {
#if FIRMWARE_UPDATE_USE_MANIFEST
    if(!loadManifest(false)) {
        lastHttpStatus = -200;
        return false;
    }
    if(manifestInfo.downloadUrl.isEmpty()) {
        lastHttpStatus = -201;
        return false;
    }

    if(updateStatus.size == 0) {
        lastHttpStatus = -203;
        return false;
    }

    uint32_t start = updateStatus.block_position * UPDATE_BUF_SIZE;
    if(start >= updateStatus.size) {
        lastHttpStatus = HTTP_CODE_OK;
        return false;
    }

    uint32_t end = start + UPDATE_BUF_SIZE - 1;
    if(end >= updateStatus.size) {
        end = updateStatus.size - 1;
    }

    char range[32];
    snprintf_P(range, sizeof(range), PSTR("bytes=%lu-%lu"), static_cast<unsigned long>(start), static_cast<unsigned long>(end));

    const char* url = manifestInfo.downloadUrl.c_str();

#if defined(ESP8266)
    WiFiClient client;
    client.setTimeout(5000);
    if(http.begin(client, url)) {
#elif defined(ESP32)
    if(http.begin(url)) {
#endif
    http.useHTTP10(true);
    http.setTimeout(60000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setUserAgent(FIRMWARE_UPDATE_USER_AGENT);
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
        http.addHeader(F("Range"), range);
        int status = http.GET();
        lastHttpStatus = status;
        if(status == HTTP_CODE_PARTIAL_CONTENT || status == HTTP_CODE_OK) {
            if(md5.equals(F("unknown")) && manifestInfo.md5.length() > 0) {
                md5 = manifestInfo.md5;
            }
            return true;
        }
    }
    if(lastHttpStatus == 0) {
        lastHttpStatus = -202;
    }
    return false;
#else
    const char * headerkeys[] = { "x-MD5" };
    http.collectHeaders(headerkeys, 1);

    if(updateStatus.size == 0) {
        lastHttpStatus = -203;
        return false;
    }

    uint32_t start = updateStatus.block_position * UPDATE_BUF_SIZE;
    if(start >= updateStatus.size) {
        lastHttpStatus = HTTP_CODE_OK;
        return false;
    }

    uint32_t end = start + UPDATE_BUF_SIZE - 1;
    if(end >= updateStatus.size) {
        end = updateStatus.size - 1;
    }

    char range[32];
    snprintf_P(range, sizeof(range), PSTR("bytes=%lu-%lu"), static_cast<unsigned long>(start), static_cast<unsigned long>(end));

    const char* firmwareVariant = FIRMWARE_UPDATE_CHANNEL;

        lastHttpStatus = status;
    char url[256];
    snprintf(url, sizeof(url), "%s/firmware/%s/%s/%s/chunk", FIRMWARE_UPDATE_BASE_URL, chipType, firmwareVariant, updateStatus.toVersion);
    #if defined(ESP8266)
    WiFiClient client;
    client.setTimeout(5000);
    if(http.begin(client, url)) {
    #elif defined(ESP32)
    if(lastHttpStatus == 0) {
        lastHttpStatus = -202;
    }
    if(http.begin(url)) {
    #endif
        http.useHTTP10(true);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setUserAgent(FIRMWARE_UPDATE_USER_AGENT);
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
        http.addHeader(F("Range"), range);
        if(http.GET() == 206) {
            this->md5 = http.header("x-MD5");
            return true;
        }
    }
    return false;
#endif
}

#if FIRMWARE_UPDATE_USE_MANIFEST
bool AmsFirmwareUpdater::loadManifest(bool force) {
    if(manifestInfo.loaded && !force) {
        return true;
    }

    HTTPClient http;
    char url[256];
    snprintf(url, sizeof(url), "%s/firmware/%s/%s/%s", FIRMWARE_UPDATE_BASE_URL, chipType, FIRMWARE_UPDATE_CHANNEL, FIRMWARE_UPDATE_MANIFEST_NAME);

#if defined(ESP8266)
    WiFiClient client;
    client.setTimeout(5000);
    if(!http.begin(client, url)) {
        return manifestInfo.loaded;
    }
#elif defined(ESP32)
    if(!http.begin(url)) {
        return manifestInfo.loaded;
    }
#endif

    http.useHTTP10(true);
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setUserAgent(FIRMWARE_UPDATE_USER_AGENT);
    http.addHeader(F("Cache-Control"), "no-cache");

    bool success = false;
    int status = http.GET();
    if(status == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, *stream);
        if(!err) {
            String version = doc["version"].as<String>();
            String download = doc["url"].as<String>();
            if(download.length() == 0 && doc["download_url"].is<String>()) {
                download = doc["download_url"].as<String>();
            }
            uint32_t size = doc["size"] | 0;
            String checksum = doc["md5"].as<String>();

            if(version.length() > 0 && download.length() > 0 && size > 0) {
                String manifestUrl = url;
                int lastSlash = manifestUrl.lastIndexOf('/');
                if(lastSlash >= 0) {
                    manifestUrl = manifestUrl.substring(0, lastSlash + 1);
                }

                if(download.startsWith("http://") || download.startsWith("https://")) {
                    manifestInfo.downloadUrl = download;
                } else {
                    manifestInfo.downloadUrl = manifestUrl + download;
                }

                manifestInfo.version = version;
                manifestInfo.size = size;
                manifestInfo.md5 = checksum;
                manifestInfo.loaded = true;
                manifestInfo.fetchedAt = millis();
                manifestInfo.mqttApplied = false;

                JsonVariantConst mqttSection = doc["mqtt"];
                if(!mqttSection.isNull()) {
                    applyManifestMqttDefaults(mqttSection);
                }
                manifestInfo.mqttApplied = true;
                success = true;
            }
        } else {
#if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
#endif
            debugger->printf_P(PSTR("Manifest parse error: %s\n"), err.c_str());
        }
    }

    http.end();
    if(!success && !manifestInfo.loaded) {
#if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::WARNING))
#endif
        debugger->printf_P(PSTR("Unable to fetch manifest from %s (HTTP %d)\n"), url, status);
    }
    return manifestInfo.loaded;
}
#endif

bool AmsFirmwareUpdater::applyManifestMqttDefaults(JsonVariantConst mqttSection) {
    if(configuration == NULL || mqttSection.isNull()) {
        return false;
    }

    SystemConfig sys;
    configuration->getSystemConfig(sys);
    if(sys.userConfigured) {
#if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
#endif
        debugger->println(F("Skipping manifest MQTT defaults: user configuration in place"));
        return false;
    }

    MqttConfig mqtt;
    configuration->getMqttConfig(mqtt);
    bool changed = false;

    JsonVariantConst hostVariant = mqttSection["host"];
    bool hostProvided = false;
    if(hostVariant.is<const char*>()) {
        const char* rawHost = hostVariant.as<const char*>();
        hostProvided = rawHost != NULL && rawHost[0] != '\0';
    }

    auto updateString = [&](const char* key, char* dest, size_t len) {
        JsonVariantConst value = mqttSection[key];
        if(value.isNull() || !value.is<const char*>()) {
            return;
        }
        const char* raw = value.as<const char*>();
        if(raw == NULL || raw[0] == '\0') {
            return;
        }
        if(strncmp(dest, raw, len) != 0) {
            size_t copyLen = strlen(raw);
            if(copyLen >= len) {
                copyLen = len - 1;
            }
            memset(dest, 0, len);
            memcpy(dest, raw, copyLen);
            changed = true;
        }
    };

    auto updateUint16 = [&](const char* key, uint16_t& field) {
        JsonVariantConst value = mqttSection[key];
        if(value.isNull()) {
            return;
        }
        long parsed = 0;
        if(value.is<int>() || value.is<long>() || value.is<unsigned int>() || value.is<unsigned long>()) {
            parsed = value.as<long>();
        } else if(value.is<double>()) {
            parsed = static_cast<long>(value.as<double>());
        } else {
            return;
        }
        if(parsed < 0) {
            return;
        }
        if(parsed > 0xFFFF) {
            parsed = 0xFFFF;
        }
        uint16_t converted = static_cast<uint16_t>(parsed);
        if(field != converted) {
            field = converted;
            changed = true;
        }
    };

    auto updateUint8 = [&](const char* key, uint8_t& field) {
        JsonVariantConst value = mqttSection[key];
        if(value.isNull()) {
            return;
        }
        long parsed = 0;
        if(value.is<int>() || value.is<long>() || value.is<unsigned int>() || value.is<unsigned long>()) {
            parsed = value.as<long>();
        } else if(value.is<double>()) {
            parsed = static_cast<long>(value.as<double>());
        } else {
            return;
        }
        if(parsed < 0) {
            return;
        }
        if(parsed > 0xFF) {
            parsed = 0xFF;
        }
        uint8_t converted = static_cast<uint8_t>(parsed);
        if(field != converted) {
            field = converted;
            changed = true;
        }
    };

    auto updateBool = [&](const char* key, bool& field) {
        JsonVariantConst value = mqttSection[key];
        if(value.isNull()) {
            return;
        }
        bool parsed;
        if(value.is<bool>()) {
            parsed = value.as<bool>();
        } else if(value.is<int>() || value.is<long>() || value.is<unsigned int>() || value.is<unsigned long>()) {
            parsed = value.as<long>() != 0;
        } else {
            return;
        }
        if(field != parsed) {
            field = parsed;
            changed = true;
        }
    };

    updateString("host", mqtt.host, sizeof(mqtt.host));
    updateUint16("port", mqtt.port);
    updateString("client_id", mqtt.clientId, sizeof(mqtt.clientId));
    updateString("publish_topic", mqtt.publishTopic, sizeof(mqtt.publishTopic));
    updateString("subscribe_topic", mqtt.subscribeTopic, sizeof(mqtt.subscribeTopic));
    updateString("username", mqtt.username, sizeof(mqtt.username));
    updateString("password", mqtt.password, sizeof(mqtt.password));
    updateUint8("payload_format", mqtt.payloadFormat);
    updateBool("ssl", mqtt.ssl);
    updateBool("state_update", mqtt.stateUpdate);
    updateUint16("state_update_interval", mqtt.stateUpdateInterval);
    updateUint16("timeout", mqtt.timeout);
    updateUint8("keepalive", mqtt.keepalive);

    bool sysChanged = false;
    if(hostProvided && !sys.vendorConfigured) {
        sys.vendorConfigured = true;
        sysChanged = true;
    }

    if(changed) {
        configuration->setMqttConfig(mqtt);
#if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
#endif
        debugger->println(F("Applied MQTT defaults from manifest"));
    }

    if(sysChanged) {
        configuration->setSystemConfig(sys);
    }

    return changed || sysChanged;
}

bool AmsFirmwareUpdater::writeUpdateStatus() {
    if(updateStatus.block_position - lastSaveBlocksWritten > 32) {
        updateStatusChanged = true;
        lastSaveBlocksWritten = updateStatus.block_position;
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Writing update status to EEPROM at block %d\n"), updateStatus.block_position);
        return true;
    }
    return false;
}

bool AmsFirmwareUpdater::startFirmwareUpload(uint32_t size, const char* version) {
    if(!isFlashReadyForNextUpdateVersion(size)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("No eligable partition was found for upgrade\n"));
        return false;
    }

    if(!setTargetVersion(version)) {
        return false;
    }
    updateStatus.size = size;
    md5 = F("unknown");
    return true;
}

bool AmsFirmwareUpdater::addFirmwareUploadChunk(uint8_t* buf, size_t length) {
    for(size_t i = 0; i < length; i++) {
        this->buf[bufPos++] = buf[i];
        if(bufPos == UPDATE_BUF_SIZE) {
            if(!writeBufferToFlash(UPDATE_BUF_SIZE)) {
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::ERROR))
                #endif
                debugger->printf_P(PSTR("Unable to write to flash\n"));
                return false;
            }
            bufPos = 0;
            memset(this->buf, 0, UPDATE_BUF_SIZE);
        }
    }
    return true;
}

bool AmsFirmwareUpdater::completeFirmwareUpload(uint32_t size) {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Firmware write complete\n"));

    if(bufPos > 0) {
        writeBufferToFlash(bufPos);
        memset(this->buf, 0, UPDATE_BUF_SIZE);
        bufPos = 0;
    }
    if(md5.equals(F("unknown"))) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("No MD5, skipping verification\n"));
    } else if(verifyChecksum()) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("MD5 verified!\n"));
    } else {
        updateStatus.errorCode = AMS_UPDATE_ERR_MD5;
        updateStatusChanged = true;
        return false;
    }

    if(updateStatus.size == 0) {
        updateStatus.size = size;
    } else if(size > 0 && updateStatus.size != size) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Expected size %lu is different from actual size %ly!\n"), updateStatus.size, size);
    }

    if(!activateNewFirmware()) {
        updateStatus.errorCode = AMS_UPDATE_ERR_ACTIVATE;
        updateStatusChanged = true;
        return false;
    }
    updateStatus.errorCode = AMS_UPDATE_ERR_SUCCESS_CONFIRMED;
    updateStatusChanged = true;
    return true;
}

#if defined(ESP32)
bool AmsFirmwareUpdater::isFlashReadyForNextUpdateVersion(uint32_t size) {
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    if(partition == NULL) return false;
    esp_partition_info_t p_info;
    if(!findPartition(partition->label, &p_info)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to find partition info for next update partition\n"));
        return false;
    }
    if(p_info.pos.size < size) return false;
    return true;
}

bool AmsFirmwareUpdater::writeBufferToFlash(size_t length) {
    if(length == 0) {
        return true;
    }

    if(length > UPDATE_BUF_SIZE) {
        length = UPDATE_BUF_SIZE;
    }

    uint32_t offset = updateStatus.block_position * UPDATE_BUF_SIZE;
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    esp_err_t eraseErr = esp_partition_erase_range(partition, offset, UPDATE_BUF_SIZE);
    if(eraseErr != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("esp_partition_erase_range(%s, %lu, %lu) failed with %d\n"), partition->label, offset, UPDATE_BUF_SIZE, eraseErr);
        updateStatus.errorCode = AMS_UPDATE_ERR_ERASE;
        return false;
    }
    esp_err_t writeErr = esp_partition_write(partition, offset, buf, length);
    if(writeErr != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("esp_partition_write(%s, %lu, buf, %lu) failed with %d\n"), partition->label, offset, UPDATE_BUF_SIZE, writeErr);
        updateStatus.errorCode = AMS_UPDATE_ERR_WRITE;
        return false;
    }
    updateStatus.block_position++;
    return true;
}

bool AmsFirmwareUpdater::activateNewFirmware() {
    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    esp_err_t ret = esp_ota_set_boot_partition(partition);
    if(ret != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to activate firmware (%d)\n"), ret);
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::relocateOrRepartitionIfNecessary() {
    const esp_partition_t* active = esp_ota_get_running_partition();
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Firmware currently running from %s\n"), active->label);
    if(active->type != ESP_PARTITION_TYPE_APP) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Not running on APP partition?!\n"));
        return false;
    }

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::DEBUG))
    #endif
    debugger->printf_P(PSTR("Small partition, repartitioning\n"));

    if(buf == NULL) buf = (uint8_t*) malloc(UPDATE_BUF_SIZE);

    if(hasTwoSpiffs()) {
        if(spiffsOnCorrectLocation()) {
            moveLittleFsFromApp1ToNew();
            return writePartitionTableFinal();
        } else {
            moveLittleFsFromOldToApp1();
            return writePartitionTableWithSpiffsAtApp1AndNew();
        }

    } else if(hasLargeEnoughAppPartitions()) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
        debugger->printf_P(PSTR("Partition is large enough, no change\n"));
        return false;
    } else if(!canMigratePartitionTable()) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::WARNING))
        #endif
        debugger->printf_P(PSTR("Not able to migrate partition table\n"));
        return false;
    } else if(active->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_MIN) {
        // Before we repartition, we need to make sure the firmware is running fra first app partition
        return relocateAppToFirst(active);
    } else if(hasFiles()) {
        return writePartitionTableWithSpiffsAtOldAndApp1();
    } else {
        return writePartitionTableFinal();
    }
}

bool AmsFirmwareUpdater::relocateAppToFirst(const esp_partition_t* active) {
    const esp_partition_t* app0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_MIN, NULL);

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Relocating %s to %s\n"), active->label, app0->label);

    esp_partition_info_t p_active, p_app0;
    if(!findPartition(active->label, &p_active)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to find partition info for active partition\n"));
        return false;
    }
    if(!findPartition(app0->label, &p_app0)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to find partition info for active partition\n"));
        return false;
    }

    if(!copyData(&p_active, &p_app0)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to copy app0 to app1\n"));
        return false;
    }

    if(esp_ota_set_boot_partition(app0) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to set app0 active\n"));
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::readPartition(uint8_t num, const esp_partition_info_t* partition) {
    uint32_t pos = num * sizeof(*partition);
    if(esp_flash_read(NULL, (uint8_t*) partition, AMS_PARTITION_TABLE_OFFSET + pos, sizeof(*partition)) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to read partition %d\n"), num);
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::writePartition(uint8_t num, const esp_partition_info_t* partition) {
    uint32_t pos = num * sizeof(*partition);
    if(esp_flash_write(NULL, (uint8_t*) partition, AMS_PARTITION_TABLE_OFFSET + pos, sizeof(*partition)) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to write partition %d\n"), num);
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::copyData(const esp_partition_info_t* src, esp_partition_info_t* dst, bool eraseFirst) {
    if(eraseFirst && esp_flash_erase_region(NULL, dst->pos.offset, dst->pos.size) != ESP_OK) {
        return false;
    }
    uint32_t pos = 0;
    while(pos < min(src->pos.size, dst->pos.size)) {
        if(esp_flash_read(NULL, buf, src->pos.offset + pos, UPDATE_BUF_SIZE) != ESP_OK) {
            return false;
        }
        if(esp_flash_write(NULL, buf, dst->pos.offset + pos, UPDATE_BUF_SIZE) != ESP_OK) {
            return false;
        }
        pos += UPDATE_BUF_SIZE;
    }
    return true;
}

bool AmsFirmwareUpdater::copyFile(fs::LittleFSFS* srcFs, fs::LittleFSFS* dstFs, const char* filename) {
    if(srcFs->exists(filename)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::INFO))
        #endif
        debugger->printf_P(PSTR("Copying file %s\n"), filename);
        File src = srcFs->open(filename, "r");
        File dst = dstFs->open(filename, "w");

        size_t size;
        while((size = src.readBytes((char*) buf, UPDATE_BUF_SIZE)) > 0) {
            dst.write((uint8_t*) buf, size);
        }
        dst.flush();
        dst.close();
        src.close();
        return true;
    }
    return false;
}

bool AmsFirmwareUpdater::verifyChecksum() {
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (!partition) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Partition for update not found\n"));
        return false;
    }

    MD5Builder md5;
    md5.begin();
    uint32_t offset = 0;
    uint32_t lengthLeft = updateStatus.size;
    while( lengthLeft > 0) {
        size_t bytes = (lengthLeft < UPDATE_BUF_SIZE) ? lengthLeft : UPDATE_BUF_SIZE;
        if(esp_partition_read(partition, offset, buf, bytes) != ESP_OK) {
            updateStatus.errorCode = AMS_UPDATE_ERR_READ;
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("Unable to read for MD5, offset %lu, bytes %lu\n"), offset, bytes);
            return false;
        }
        md5.add((uint8_t*) buf, bytes);
        lengthLeft -= bytes;
        offset += bytes;

        delay(1);
    }
    md5.calculate();

    if(md5.toString().equals(this->md5)) {
        return true;
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("MD5 %s does not match expected %s\n"), md5.toString().c_str(), this->md5.c_str());
        return false;
    }
}

bool AmsFirmwareUpdater::findPartition(const char* label, const esp_partition_info_t* info) {
    for(uint8_t i = 0; i < 10; i++) {
        uint16_t size = sizeof(*info);
        uint32_t pos = i * size;
        readPartition(i, info);
        if(info->magic == ESP_PARTITION_MAGIC) {
            if(strcmp((const char*) info->label, label) == 0) {
                return true;
            }
        } else {
            return false;
        }
    }

}

bool AmsFirmwareUpdater::hasLargeEnoughAppPartitions() {
    esp_partition_info_t part;
    readPartition(2, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_APP, part.subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0 || part.pos.size < AMS_PARTITION_APP_SIZE)
        return false;

    readPartition(3, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_APP, part.subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1 || part.pos.size < AMS_PARTITION_APP_SIZE)
        return false;

    return true;
}

bool AmsFirmwareUpdater::canMigratePartitionTable() {
    size_t appAndSpiffs = 0;
    esp_partition_info_t part;
    readPartition(0, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_NVS)
        return false;

    readPartition(1, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_OTA)
        return false;

    readPartition(2, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_APP, part.subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0)
        return false;
    appAndSpiffs += part.pos.size;

    readPartition(3, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_APP, part.subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1)
        return false;
    appAndSpiffs += part.pos.size;

    readPartition(4, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_SPIFFS)
        return false;
    appAndSpiffs += part.pos.size;

    readPartition(5, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_COREDUMP)
        return false;

    if(appAndSpiffs < (AMS_PARTITION_APP_SIZE * 2) + AMS_PARTITION_MIN_SPIFFS_SIZE)
        return false;

    return true;

}

bool AmsFirmwareUpdater::hasTwoSpiffs() {
    uint8_t count = 0;
    esp_partition_info_t part;
    for(uint8_t i = 0; i < 10; i++) {
        uint16_t size = sizeof(part);
        uint32_t pos = i * size;
        readPartition(i, &part);
        if(part.magic == ESP_PARTITION_MAGIC) {
            if(part.type == ESP_PARTITION_TYPE_DATA && part.subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS) {
                count++;
            }
        } else {
            break;
        }
    }
    return count == 2;
}

bool AmsFirmwareUpdater::spiffsOnCorrectLocation() {
    esp_partition_info_t p_app0;
    readPartition(2, &p_app0);

    uint32_t expectedOffset = p_app0.pos.offset + AMS_PARTITION_APP_SIZE + AMS_PARTITION_APP_SIZE;

    esp_partition_info_t part;
    for(uint8_t i = 0; i < 10; i++) {
        uint16_t size = sizeof(part);
        uint32_t pos = i * size;
        readPartition(i, &part);
        if(part.magic == ESP_PARTITION_MAGIC) {
            if(part.type == ESP_PARTITION_TYPE_DATA && part.subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS && part.pos.offset == expectedOffset) {
                return true;
            }
        } else {
            break;
        }
    }

    return false;
}

bool AmsFirmwareUpdater::hasFiles() {
    if(!LittleFS.begin()) return false;
    if(LittleFS.exists(FILE_MQTT_CA)) return true;
    if(LittleFS.exists(FILE_MQTT_CERT)) return true;
    if(LittleFS.exists(FILE_MQTT_KEY)) return true;
    if(LittleFS.exists(FILE_DAYPLOT)) return true;
    if(LittleFS.exists(FILE_MONTHPLOT)) return true;
    if(LittleFS.exists(FILE_ENERGYACCOUNTING)) return true;
    if(LittleFS.exists(FILE_PRICE_CONF)) return true;
    return false;
}

bool AmsFirmwareUpdater::clearPartitionTable() {
    esp_err_t p_erase_err = esp_flash_erase_region(NULL, AMS_PARTITION_TABLE_OFFSET, 4096);
    if(p_erase_err != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase partition table (%d)\n"), p_erase_err);
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::writeNewPartitionChecksum(uint8_t num) {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Creating MD5 for partition table\n"));

    memset(buf, 0, UPDATE_BUF_SIZE);
    MD5Builder md5;
    md5.begin();

    esp_partition_info_t part, p_null;
    memset(&p_null, 0, sizeof(p_null));
    uint32_t md5pos = num * sizeof(part);
    for(uint8_t i = 0; i < num; i++) {
        uint16_t size = sizeof(part);
        uint32_t pos = i * size;
        readPartition(i, &part);
        if(part.magic == ESP_PARTITION_MAGIC) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            debugger->printf_P(PSTR("Partition %d, magic: %04X, offset: %X, size: %d, type: %d:%d, label: %s, flags: %04X\n"), i, part.magic, part.pos.offset, part.pos.size, part.type, part.subtype, part.label, part.flags);            
            md5.add((uint8_t*) &part, sizeof(part));
        } else {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::WARNING))
            #endif
            debugger->printf_P(PSTR("Overwriting invalid partition at %d\n"), i);
            writePartition(i, &p_null);
        }
    }

    md5.calculate();
    md5.getChars((char*) buf);
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::DEBUG))
    #endif
    debugger->printf_P(PSTR("Writing MD5 %s to position %d\n"), buf, md5pos);

    // Writing MD5 header and MD5 sum
    part.magic = ESP_PARTITION_MAGIC_MD5;
    if(esp_flash_write(NULL, (uint8_t*) &part, AMS_PARTITION_TABLE_OFFSET + md5pos, 2) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to write md5 header\n"));
        return false;
    }
    md5.getBytes((uint8_t*) buf);
    if(esp_flash_write(NULL, buf, AMS_PARTITION_TABLE_OFFSET + md5pos + ESP_PARTITION_MD5_OFFSET, ESP_ROM_MD5_DIGEST_LEN) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to write md5\n"));
        return false;
    }
    return true;
}

bool AmsFirmwareUpdater::writePartitionTableWithSpiffsAtOldAndApp1() {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Writing partition table with LittleFS at old location and app1\n"));

    esp_partition_info_t p_nvs, p_ota, p_app0, p_app1, p_spiffs, p_coredump;
    readPartition(0, &p_nvs);
    readPartition(1, &p_ota);
    readPartition(2, &p_app0);
    readPartition(3, &p_app1);
    readPartition(4, &p_spiffs);
    readPartition(5, &p_coredump);

    memset(p_app1.label, 0, 16);
	memcpy_P(p_app1.label, PSTR("tmpfs"), 5);
    p_app1.type = ESP_PARTITION_TYPE_DATA;
    p_app1.subtype = ESP_PARTITION_SUBTYPE_DATA_SPIFFS;

    memset(p_spiffs.label, 0, 16);
	memcpy_P(p_spiffs.label, PSTR("oldfs"), 5);


    clearPartitionTable();
    if(!writePartition(0, &p_nvs)) return false;
    if(!writePartition(1, &p_ota)) return false;
    if(!writePartition(2, &p_app0)) return false;
    if(!writePartition(3, &p_app1)) return false;
    if(!writePartition(4, &p_spiffs)) return false;
    if(!writePartition(5, &p_coredump)) return false;
    if(!writeNewPartitionChecksum(6)) return false;

    // Clearing app1 partition
    if(esp_flash_erase_region(NULL, p_app1.pos.offset, p_app1.pos.size) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase app1\n"));
    }

    return true;
}

bool AmsFirmwareUpdater::writePartitionTableWithSpiffsAtApp1AndNew() {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Writing partition table with LittleFS at app1 and new location\n"));

    esp_partition_info_t p_nvs, p_ota, p_app0, p_app1, p_dummy, p_spiffs, p_coredump;
    readPartition(0, &p_nvs);
    readPartition(1, &p_ota);
    readPartition(2, &p_app0);
    readPartition(3, &p_app1);
    readPartition(4, &p_spiffs);
    readPartition(5, &p_coredump);

    memset(p_spiffs.label, 0, 16);
	memcpy_P(p_spiffs.label, PSTR("newfs"), 5);
    p_spiffs.pos.offset = p_app0.pos.offset + AMS_PARTITION_APP_SIZE + AMS_PARTITION_APP_SIZE;
    p_spiffs.pos.size = p_coredump.pos.offset - p_spiffs.pos.offset;

    p_dummy.magic = ESP_PARTITION_MAGIC;
    p_dummy.type = ESP_PARTITION_TYPE_DATA;
    p_dummy.subtype = ESP_PARTITION_SUBTYPE_DATA_FAT;
    p_dummy.pos.offset = p_app1.pos.offset + p_app1.pos.size;
    p_dummy.pos.size = p_spiffs.pos.offset - p_dummy.pos.offset;
    p_dummy.flags = p_app0.flags;
    memset(p_dummy.label, 0, 16);
    memcpy_P(p_dummy.label, PSTR("dummy"), 5);

    clearPartitionTable();
    if(!writePartition(0, &p_nvs)) return false;
    if(!writePartition(1, &p_ota)) return false;
    if(!writePartition(2, &p_app0)) return false;
    if(!writePartition(3, &p_app1)) return false;
    if(!writePartition(4, &p_dummy)) return false;
    if(!writePartition(5, &p_spiffs)) return false;
    if(!writePartition(6, &p_coredump)) return false;
    if(!writeNewPartitionChecksum(7)) return false;

    // Clearing dummy partition
    if(esp_flash_erase_region(NULL, p_dummy.pos.offset, p_dummy.pos.size) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase dummy partition\n"));
    }

    // Clearing spiffs partition
    if(esp_flash_erase_region(NULL, p_spiffs.pos.offset, p_spiffs.pos.size) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase spiffs partition\n"));
    }

    return true;
}

bool AmsFirmwareUpdater::writePartitionTableFinal() {
    esp_partition_info_t p_nvs, p_ota, p_app0, p_app1, p_spiffs, p_coredump;
    readPartition(0, &p_nvs);
    readPartition(1, &p_ota);
    readPartition(2, &p_app0);
    readPartition(3, &p_app1);
    readPartition(4, &p_spiffs);
    if(p_spiffs.subtype != ESP_PARTITION_SUBTYPE_DATA_SPIFFS) {
        readPartition(5, &p_spiffs);
        readPartition(6, &p_coredump);
    } else {
        readPartition(5, &p_coredump);
    }

    p_app0.magic = ESP_PARTITION_MAGIC;
    p_app0.type = ESP_PARTITION_TYPE_APP;
    p_app0.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
    p_app0.pos.offset = p_ota.pos.offset + p_ota.pos.size;
    p_app0.pos.size = AMS_PARTITION_APP_SIZE;

    p_app1.magic = ESP_PARTITION_MAGIC;
    p_app1.type = ESP_PARTITION_TYPE_APP;
    p_app1.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
    p_app1.pos.offset = p_app0.pos.offset + p_app0.pos.size;
    p_app1.pos.size = AMS_PARTITION_APP_SIZE;
    p_app1.flags = p_app0.flags;

    p_spiffs.magic = ESP_PARTITION_MAGIC;
    p_spiffs.type = ESP_PARTITION_TYPE_DATA;
    p_spiffs.subtype = ESP_PARTITION_SUBTYPE_DATA_SPIFFS;
    p_spiffs.pos.offset = p_app1.pos.offset + p_app1.pos.size;
    p_spiffs.pos.size = p_coredump.pos.offset - p_spiffs.pos.offset;
    p_spiffs.flags = p_app0.flags;

    memset(p_app0.label, 0, 16);
    memset(p_app1.label, 0, 16);
    memset(p_spiffs.label, 0, 16);

    memcpy_P(p_app0.label, PSTR("app0"), 4);
    memcpy_P(p_app1.label, PSTR("app1"), 4);
    memcpy_P(p_spiffs.label, PSTR("spiffs"), 6);

    clearPartitionTable();
    if(!writePartition(0, &p_nvs)) return false;
    if(!writePartition(1, &p_ota)) return false;
    if(!writePartition(2, &p_app0)) return false;
    if(!writePartition(3, &p_app1)) return false;
    if(!writePartition(4, &p_spiffs)) return false;
    if(!writePartition(5, &p_coredump)) return false;
    if(!writeNewPartitionChecksum(6)) return false;

    // Clearing app1 partition
    if(esp_flash_erase_region(NULL, p_app1.pos.offset, p_app1.pos.size) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase app1 partition\n"));
    }

    // Recreating image header on app1, just by copying from app0
    esp_image_header_t h_app0;
    if(esp_flash_read(NULL, buf, p_app0.pos.offset, sizeof(&h_app0)) == ESP_OK) {
        if(esp_flash_write(NULL, buf, p_app1.pos.offset, sizeof(&h_app0)) != ESP_OK) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("Unable to write header to app1\n"));
        }
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to read header from app0\n"));
    }

    return true;
}

bool AmsFirmwareUpdater::moveLittleFsFromOldToApp1() {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Moving LittleFS from old to temporary\n"));

    fs::LittleFSFS oldFs, tmpFs;
    if(oldFs.begin(false, "/oldfs", 10, "oldfs")) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Successfully found LittleFS at old location\n"));
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to start existing filesystem\n"));
        return false;
    }

    if(tmpFs.begin(true, "/tmpfs", 10, "tmpfs")) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Successfully created LittleFS at temporary location\n"));
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to start temporary filesystem\n"));
        return false;
    }
    copyFile(&oldFs, &tmpFs, FILE_MQTT_CA);
    copyFile(&oldFs, &tmpFs, FILE_MQTT_CERT);
    copyFile(&oldFs, &tmpFs, FILE_MQTT_KEY);
    copyFile(&oldFs, &tmpFs, FILE_DAYPLOT);
    copyFile(&oldFs, &tmpFs, FILE_MONTHPLOT);
    copyFile(&oldFs, &tmpFs, FILE_ENERGYACCOUNTING);
    copyFile(&oldFs, &tmpFs, FILE_PRICE_CONF);
    return true;
}

bool AmsFirmwareUpdater::moveLittleFsFromApp1ToNew() {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Moving LittleFS from temporary to new\n"));

    fs::LittleFSFS newFs, tmpFs;
    if(tmpFs.begin(false, "/tmpfs", 10, "tmpfs")) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Successfully found LittleFS at temporary location\n"));
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to start temporary filesystem\n"));
        return false;
    }
    if(newFs.begin(true, "/newfs", 10, "newfs")) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Successfully created LittleFS at new location\n"));
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to start new filesystem\n"));
        return false;
    }
    copyFile(&tmpFs, &newFs, FILE_MQTT_CA);
    copyFile(&tmpFs, &newFs, FILE_MQTT_CERT);
    copyFile(&tmpFs, &newFs, FILE_MQTT_KEY);
    copyFile(&tmpFs, &newFs, FILE_DAYPLOT);
    copyFile(&tmpFs, &newFs, FILE_MONTHPLOT);
    copyFile(&tmpFs, &newFs, FILE_ENERGYACCOUNTING);
    copyFile(&tmpFs, &newFs, FILE_PRICE_CONF);
    return true;
}
#elif defined(ESP8266)
uintptr_t AmsFirmwareUpdater::getFirmwareUpdateStart() {
    return (AMS_FLASH_SKETCH_SIZE + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
}

bool AmsFirmwareUpdater::isFlashReadyForNextUpdateVersion(uint32_t size) {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Checking if we can upgrade\n"));

    if(FS_PHYS_ADDR < (getFirmwareUpdateStart() + AMS_FLASH_SKETCH_SIZE)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("No room for OTA update\n"));
        return false;
    }

    if(!ESP.checkFlashConfig(false)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("checkFlashConfig failed\n"));
        return false;
    }

    if(size > AMS_FLASH_SKETCH_SIZE) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("New firmware does not fit flash\n"));
        return false;
    }
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Ready for next update version\n"));
    return true;
}

bool AmsFirmwareUpdater::writeBufferToFlash(size_t length) {
    if(length == 0) {
        return true;
    }

    if(length > UPDATE_BUF_SIZE) {
        length = UPDATE_BUF_SIZE;
    }

    // ESP8266 flash writes must be 4-byte aligned
    size_t paddedLength = (length + 3) & ~((size_t)3);
    if(paddedLength > UPDATE_BUF_SIZE) {
        paddedLength = UPDATE_BUF_SIZE;
    }

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Writing buffer to flash\n"));
    yield();
    uint32_t offset = updateStatus.block_position * UPDATE_BUF_SIZE;
    uintptr_t currentAddress = getFirmwareUpdateStart() + offset;
    uint32_t sector = currentAddress/FLASH_SECTOR_SIZE;

    if (currentAddress % FLASH_SECTOR_SIZE == 0) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::DEBUG))
        #endif
        debugger->printf_P(PSTR("flashEraseSector(%lu)\n"), sector);
        yield();
        if(!ESP.flashEraseSector(sector)) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("flashEraseSector(%lu) failed\n"), sector);
            updateStatus.errorCode = AMS_UPDATE_ERR_ERASE;
            return false;
        }
    }

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::DEBUG))
    #endif
    debugger->printf_P(PSTR("flashWrite(%lu)\n"), sector);
    yield();
    if(!ESP.flashWrite(currentAddress, buf, paddedLength)) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("flashWrite(%lu, buf, %lu) failed\n"), currentAddress, paddedLength);
        updateStatus.errorCode = AMS_UPDATE_ERR_WRITE;
        return false;
    }
    updateStatus.block_position++;
    return true;
}

bool AmsFirmwareUpdater::verifyChecksum() {
    MD5Builder md5;
    md5.begin();
    uint32_t offset = 0;
    uint32_t lengthLeft = updateStatus.size;
    while( lengthLeft > 0) {
        size_t bytes = (lengthLeft < UPDATE_BUF_SIZE) ? lengthLeft : UPDATE_BUF_SIZE;
        uintptr_t currentAddress = getFirmwareUpdateStart() + offset;
        if(!ESP.flashRead(currentAddress, buf, bytes)) {
            updateStatus.errorCode = AMS_UPDATE_ERR_READ;
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::ERROR))
            #endif
            debugger->printf_P(PSTR("Unable to read for MD5, offset %lu, bytes %lu\n"), offset, bytes);
            return false;
        }
        md5.add((uint8_t*) buf, bytes);
        lengthLeft -= bytes;
        offset += bytes;

        delay(1);
    }
    md5.calculate();

    if(md5.toString().equals(this->md5)) {
        return true;
    } else {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("MD5 %s does not match expected %s\n"), md5.toString().c_str(), this->md5.c_str());
        return false;
    }
}

bool AmsFirmwareUpdater::activateNewFirmware() {
    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Activating new firmware, start at %lu, size is %lu\n"), getFirmwareUpdateStart(), updateStatus.size);

    eboot_command ebcmd;
    ebcmd.action = ACTION_COPY_RAW;
    ebcmd.args[0] = getFirmwareUpdateStart();
    ebcmd.args[1] = 0x00000;
    ebcmd.args[2] = updateStatus.size;
    eboot_command_write(&ebcmd);
    return true;
}
#endif
