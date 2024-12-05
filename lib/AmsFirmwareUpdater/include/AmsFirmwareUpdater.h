#pragma once
#include <stdint.h>
#include <Print.h>
#include "HwTools.h"
#include "AmsData.h"
#include "AmsConfiguration.h"

#if defined(ESP32)
#include "esp_flash_partitions.h"
#include "LittleFS.h"
#include "WiFi.h"
#include "HTTPClient.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif

#define AMS_PARTITION_TABLE_OFFSET 0x8000
#define AMS_PARTITION_APP0_OFFSET 0x10000
#define AMS_PARTITION_APP_SIZE 0x1D0000
#define AMS_PARTITION_MIN_SPIFFS_SIZE 0x20000
#endif

#define AMS_UPDATE_ERR_OK 0
#define AMS_UPDATE_ERR_DETAILS 1
#define AMS_UPDATE_ERR_FETCH 2
#define AMS_UPDATE_ERR_ERASE 3
#define AMS_UPDATE_ERR_WRITE 4
#define AMS_UPDATE_ERR_READ 5
#define AMS_UPDATE_ERR_MD5 6
#define AMS_UPDATE_ERR_ACTIVATE 7
#define AMS_UPDATE_ERR_REBOOT 64
#define AMS_UPDATE_ERR_SUCCESS_SIGNAL 122
#define AMS_UPDATE_ERR_SUCCESS_CONFIRMED 123

#define UPDATE_BUF_SIZE 4096

class AmsFirmwareUpdater {
public:
    #if defined(AMS_REMOTE_DEBUG)
    AmsFirmwareUpdater(RemoteDebug* debugger, HwTools* hw, AmsData* meterState);
    #else
    AmsFirmwareUpdater(Print* debugger, HwTools* hw, AmsData* meterState);
    #endif
    bool relocateOrRepartitionIfNecessary();
    void loop();

    char* getNextVersion();
    bool setTargetVersion(const char* version);
    void getUpgradeInformation(UpgradeInformation&);
    float getProgress();
    bool activateDownloadedFirmware();

    void setUpgradeInformation(UpgradeInformation&);
    bool isUpgradeInformationChanged();
    void ackUpgradeInformationChanged();

    bool startFirmwareUpload(uint32_t size, const char* version);
    bool addFirmwareUploadChunk(uint8_t* buf, size_t length);
    bool completeFirmwareUpload();

private:
    #if defined(ESP8266)
		char chipType[10] = "esp8266";
	#elif defined(CONFIG_IDF_TARGET_ESP32S2)
		char chipType[10] = "esp32s2";
	#elif defined(CONFIG_IDF_TARGET_ESP32S3)
		char chipType[10] = "esp32s3";
	#elif defined(CONFIG_IDF_TARGET_ESP32C3)
		char chipType[10] = "esp32c3";
	#elif defined(ESP32)
		#if defined(CONFIG_FREERTOS_UNICORE)
			char chipType[10] = "esp32solo";
		#else
			char chipType[10] = "esp32";
		#endif
	#endif

    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
    #else
    Stream* debugger;
    #endif
    HwTools* hw;
    AmsData* meterState;

    bool updateStatusChanged = false;
    UpgradeInformation updateStatus = {"","",0,0,0,0,0};
    uint16_t lastSaveBlocksWritten = 0;
    String md5;

    uint32_t lastVersionCheck = 0;
    uint8_t firmwareVariant;
    bool autoUpgrade;
    char nextVersion[10];


    bool fetchNextVersion();
    bool fetchVersionDetails();
    bool fetchFirmwareChunk(HTTPClient& http);
    bool writeBufferToFlash();
    bool verifyChecksum();
    bool activateNewFirmware();
    bool writeUpdateStatus();
    uint32_t sketchSize(sketchSize_t response);

    #if defined(ESP32)
    char* buf = NULL;
    uint16_t bufPos = 0;

    bool readPartition(uint8_t num, const esp_partition_info_t* info);
    bool writePartition(uint8_t num, const esp_partition_info_t* info);
    bool copyData(const esp_partition_info_t* src, esp_partition_info_t* dst, bool eraseFirst=true);
    bool copyFile(fs::LittleFSFS* src, fs::LittleFSFS* dst, const char* filename);
    uint8_t* extractFileData(const char* filename, size_t& size);
    void saveFileData(const char* filename, uint8_t* data, size_t size);

    bool relocateAppToFirst(const esp_partition_t* active);
    bool findPartition(const char* label, const esp_partition_info_t* info);

    bool hasLargeEnoughAppPartitions();
    bool canMigratePartitionTable();
    bool hasTwoSpiffs();
    bool spiffsOnCorrectLocation();
    bool hasFiles();

    bool clearPartitionTable();
    bool writeNewPartitionChecksum(uint8_t num);
    bool writePartitionTableWithSpiffsAtOldAndApp1();
    bool writePartitionTableWithSpiffsAtApp1AndNew();
    bool writePartitionTableFinal();

    bool moveLittleFsFromOldToApp1();
    bool moveLittleFsFromApp1ToNew();

    #endif
};
