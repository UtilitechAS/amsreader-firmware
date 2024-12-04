#include "AmsFirmwareUpdater.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"

#if defined(ESP32)
#include "esp_ota_ops.h"
#include "esp_littlefs.h"
#include "driver/spi_common.h"
#include "esp_flash_spi_init.h"
#include "MD5Builder.h"
#elif defined(ESP8266)
#include ""
#endif

#if defined(AMS_REMOTE_DEBUG)
AmsFirmwareUpdater::AmsFirmwareUpdater(RemoteDebug* debugger, HwTools* hw, AmsData* meterState) {
#else
AmsFirmwareUpdater::AmsFirmwareUpdater(Print* debugger, HwTools* hw, AmsData* meterState) {
#endif
this->debugger = debugger;
    this->hw = hw;
    this->meterState = meterState;
    memset(nextVersion, 0, sizeof(nextVersion));
    firmwareVariant = 0;
    autoUpgrade = false;
}

char* AmsFirmwareUpdater::getNextVersion() {
    return nextVersion;
}

bool AmsFirmwareUpdater::setTargetVersion(const char* version) {
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

    #if defined(AMS_REMOTE_DEBUG)
    if (debugger->isActive(RemoteDebug::INFO))
    #endif
    debugger->printf_P(PSTR("Preparing uprade to %s\n"), updateStatus.toVersion);

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
    if(strlen(updateStatus.toVersion) == 0 || updateStatus.size == 0) return -1.0;
    return min((float) 100.0, ((((float) updateStatus.block_position) * UPDATE_BUF_SIZE) / updateStatus.size) * 100);
}

void AmsFirmwareUpdater::loop() {
    if(strlen(updateStatus.toVersion) > 0 && updateStatus.errorCode == AMS_UPDATE_ERR_OK) {
        if(!hw->isVoltageOptimal(0.1)) {
            writeUpdateStatus();
            return;
        }

        unsigned long start = 0, end = 0;

        if(buf == NULL) buf = (char*) malloc(UPDATE_BUF_SIZE);
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
            updateStatus.retry_count = 0;
            updateStatus.block_position = 0;
            updateStatus.errorCode = AMS_UPDATE_ERR_OK;
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

            start = millis();
            WiFiClient* client = http.getStreamPtr();
            updateStatus.retry_count = 0;
            if(!client->available()) {
                http.end();
                return;
            }
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("get ptr took %lums (%d) \n"), end-start, client->available());
            size_t bytes = UPDATE_BUF_SIZE; // To start first loop
            while(bytes > 0 && client->available() > 0) {
                start = millis();
                memset(buf, 0, UPDATE_BUF_SIZE);
                bytes = client->readBytes(buf, min((uint32_t) UPDATE_BUF_SIZE, updateStatus.size - (updateStatus.block_position * UPDATE_BUF_SIZE)));
                end = millis();
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                debugger->printf_P(PSTR("read buffer took %lums (%lu bytes, %d left)\n"), end-start, bytes, client->available());
                if(bytes > 0) {
                    start = millis();
                    if(!writeBufferToFlash()) {
                        http.end();
                        return;
                    }
                    end = millis();
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::DEBUG))
                    #endif
                    debugger->printf_P(PSTR("write buffer took %lums\n"), end-start);
                }
                start = millis();
                if(!hw->isVoltageOptimal(0.2)) {
                    writeUpdateStatus();
                }
                end = millis();
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                debugger->printf_P(PSTR("check voltage took %lums\n"), end-start);
            }
            start = millis();
            http.end();
            end = millis();
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("http end took %lums\n"), end-start);
        } else if(updateStatus.block_position * UPDATE_BUF_SIZE >= updateStatus.size) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::INFO))
            #endif
            debugger->printf_P(PSTR("Firmware download complete\n"));

            if(!verifyChecksum()) {
                updateStatus.errorCode = AMS_UPDATE_ERR_MD5;
                updateStatusChanged = true;
                return;
            }
            if(!activateNewFirmware()) {
                updateStatus.errorCode = AMS_UPDATE_ERR_ACTIVATE;
                updateStatusChanged = true;
                return;
            }
            updateStatus.errorCode = AMS_UPDATE_ERR_SUCCESS_SIGNAL;
            updateStatusChanged = true;
        }
    } else {
        uint32_t seconds = millis() / 1000.0;
        if((lastVersionCheck == 0 && seconds > 20) || seconds - lastVersionCheck > 86400) {
            fetchNextVersion();
            lastVersionCheck = seconds;
        }
    }
}

bool AmsFirmwareUpdater::fetchNextVersion() {
    HTTPClient http;
    const char * headerkeys[] = { "x-version" };
    http.collectHeaders(headerkeys, 1);

    char firmwareVariant[10] = "stable";

    char url[128];
    snprintf_P(url, 128, PSTR("http://hub.amsleser.no/hub/firmware/%s/%s/next"), chipType, firmwareVariant);
    if(http.begin(url)) {
        http.useHTTP10(true);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setUserAgent("AMS-Firmware-Updater");
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
        int status = http.GET();
        if(status == 204) {
            String nextVersion = http.header("x-version");
            strcpy(this->nextVersion, nextVersion.c_str());
            if(autoUpgrade && strcmp(updateStatus.toVersion, this->nextVersion) != 0) {
                strcpy(updateStatus.toVersion, this->nextVersion);
                updateStatus.size = 0;
            }
            http.end();
            return strlen(this->nextVersion) > 0;
        } else if(status == 200) {
            memset(this->nextVersion, 0, sizeof(this->nextVersion));
        }
        http.end();
    }
    return false;
}

bool AmsFirmwareUpdater::fetchVersionDetails() {
    HTTPClient http;
    const char * headerkeys[] = { "x-size" };
    http.collectHeaders(headerkeys, 1);

    char firmwareVariant[10] = "stable";

    char url[128];
    snprintf_P(url, 128, PSTR("http://hub.amsleser.no/hub/firmware/%s/%s/%s/details"), chipType, firmwareVariant, updateStatus.toVersion);
    if(http.begin(url)) {
        http.useHTTP10(true);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setUserAgent("AMS-Firmware-Updater");
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
}

bool AmsFirmwareUpdater::fetchFirmwareChunk(HTTPClient& http) {
    const char * headerkeys[] = { "x-MD5" };
    http.collectHeaders(headerkeys, 1);

    uint32_t start = updateStatus.block_position * UPDATE_BUF_SIZE;
    uint32_t end = start + (UPDATE_BUF_SIZE * 1);
    char range[24];
    snprintf_P(range, 24, PSTR("bytes=%lu-%lu"), start, end);

    char firmwareVariant[10] = "stable";

    char url[128];
    snprintf_P(url, 128, PSTR("http://hub.amsleser.no/hub/firmware/%s/%s/%s/chunk"), chipType, firmwareVariant, updateStatus.toVersion);
    if(http.begin(url)) {
        http.useHTTP10(true);
        http.setTimeout(30000);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setUserAgent("AMS-Firmware-Updater");
        http.addHeader(F("Cache-Control"), "no-cache");
        http.addHeader(F("x-AMS-version"), FirmwareVersion::VersionString);
        http.addHeader(F("Range"), range);
        if(http.GET() == 206) {
            this->md5 = http.header("x-MD5");
            return true;
        }
    }
    return false;
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

#if defined(ESP32)
bool AmsFirmwareUpdater::writeBufferToFlash() {
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
    esp_err_t writeErr = esp_partition_write(partition, offset, buf, UPDATE_BUF_SIZE);
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

    if(buf == NULL) buf = (char*) malloc(UPDATE_BUF_SIZE);

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
        while((size = src.readBytes(buf, UPDATE_BUF_SIZE)) > 0) {
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

    readPartition(3, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_APP, part.subtype != ESP_PARTITION_SUBTYPE_APP_OTA_1)
        return false;

    readPartition(4, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_SPIFFS)
        return false;

    readPartition(5, &part);
    if(part.magic != ESP_PARTITION_MAGIC || part.type != ESP_PARTITION_TYPE_DATA, part.subtype != ESP_PARTITION_SUBTYPE_DATA_COREDUMP)
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
    md5.getChars(buf);
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
    bool ret = writeNewPartitionChecksum(6);

    // Clearing app1 partition
    if(esp_flash_erase_region(NULL, p_app1.pos.offset, p_app1.pos.size) != ESP_OK) {
        #if defined(AMS_REMOTE_DEBUG)
        if (debugger->isActive(RemoteDebug::ERROR))
        #endif
        debugger->printf_P(PSTR("Unable to erase app1\n"));
    }

    return ret;
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
    bool ret = writeNewPartitionChecksum(7);

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

    return ret;
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
    bool ret = writeNewPartitionChecksum(6);

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

    return ret;
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
#endif
