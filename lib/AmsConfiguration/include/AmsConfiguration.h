/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _AMSCONFIGURATION_h
#define _AMSCONFIGURATION_h
#include <EEPROM.h>
#include "Arduino.h"

#define EEPROM_SIZE 1024*3
#define EEPROM_CHECK_SUM 104 // Used to check if config is stored. Change if structure changes
#define EEPROM_CLEARED_INDICATOR 0xFC
#define EEPROM_CONFIG_ADDRESS 0

#define CONFIG_SYSTEM_START 8
#define CONFIG_NETWORK_START 40
#define CONFIG_METER_START 296
#define CONFIG_GPIO_START 368
#define CONFIG_PRICE_START 400
#define CONFIG_ENERGYACCOUNTING_START 472
#define CONFIG_WEB_START 496
#define CONFIG_DEBUG_START 632
#define CONFIG_NTP_START 640
#define CONFIG_MQTT_START 768
#define CONFIG_DOMOTICZ_START 1536
#define CONFIG_HA_START 1552
#define CONFIG_UI_START 1720
#define CONFIG_CLOUD_START 1742
#define CONFIG_UPGRADE_INFO_START 1934
#define CONFIG_ZC_START 2000

#define CONFIG_METER_START_103 32
#define CONFIG_UPGRADE_INFO_START_103 216
#define CONFIG_UI_START_103 248
#define CONFIG_GPIO_START_103 266
#define CONFIG_ENTSOE_START_103 290
#define CONFIG_WIFI_START_103 360
#define CONFIG_ENERGYACCOUNTING_START_103 576
#define CONFIG_WEB_START_103 648
#define CONFIG_DEBUG_START_103 824
#define CONFIG_DOMOTICZ_START_103 856 
#define CONFIG_NTP_START_103 872
#define CONFIG_MQTT_START_103 1004
#define CONFIG_HA_START_103 1680

#define LED_BEHAVIOUR_DEFAULT 0
#define LED_BEHAVIOUR_BOOT 1
#define LED_BEHAVIOUR_ERROR_ONLY 3
#define LED_BEHAVIOUR_OFF 9

struct ResetDataContainer {
	uint8_t cause;
	uint8_t last_cause;
	uint8_t magic;
};

struct SystemConfig {
	uint8_t boardType;
	bool vendorConfigured;
	bool userConfigured;
	uint8_t dataCollectionConsent; // 0 = unknown, 1 = accepted, 2 = declined
	char country[3];
	uint8_t energyspeedometer;
}; // 8

struct NetworkConfig {
	char ssid[32];
	char psk[64];
    char ip[16];
    char gateway[16];
    char subnet[16];
	char dns1[16];
	char dns2[16];
	char hostname[32];
	bool mdns;
	uint8_t power;
	uint8_t sleep;
	uint8_t use11b;
	bool ipv6;
	uint8_t mode;
}; // 214

struct MqttConfig {
	char host[128];
	uint16_t port;
	char clientId[32];
	char publishTopic[64];
	char subscribeTopic[64];
	char username[128];
	char password[256];
	uint8_t payloadFormat;
	bool ssl;
	uint8_t magic;
	bool stateUpdate;
	uint16_t stateUpdateInterval;
	uint16_t timeout;
	uint8_t keepalive;
}; // 685

struct WebConfig {
	uint8_t security;
	char username[37];
	char password[37];
	char context[37];
}; // 112

struct WebConfig103 {
	uint8_t security;
	char username[64];
	char password[64];
}; // 129

struct MeterConfig {
	uint32_t baud;
	uint8_t parity;
	bool invert;
	uint8_t distributionSystem;
	uint16_t mainFuse;
	uint16_t productionCapacity;
	uint8_t encryptionKey[16];
	uint8_t authenticationKey[16];
	uint32_t wattageMultiplier;
	uint32_t voltageMultiplier;
	uint32_t amperageMultiplier;
	uint32_t accumulatedMultiplier;
	uint8_t source;
	uint8_t parser;
	uint8_t bufferSize;
	uint8_t rxPin;
	bool rxPinPullup;
	uint8_t txPin;
}; // 65

struct DebugConfig {
	bool telnet;
	bool serial;
	uint8_t level;
}; // 3

struct GpioConfig {
	uint8_t apPin;
	uint8_t ledPin;
	bool ledInverted;
	uint8_t ledPinRed;
	uint8_t ledPinGreen;
	uint8_t ledPinBlue;
	bool ledRgbInverted;
	uint8_t tempSensorPin;
	uint8_t tempAnalogSensorPin;
	uint8_t vccPin;
	int16_t vccOffset;
	uint16_t vccMultiplier;
	uint8_t vccBootLimit;
	uint16_t vccResistorGnd;
	uint16_t vccResistorVcc;
	uint8_t ledDisablePin;
	uint8_t ledBehaviour;
}; // 21

struct GpioConfig103 {
	uint8_t hanPin;
	uint8_t apPin;
	uint8_t ledPin;
	bool ledInverted;
	uint8_t ledPinRed;
	uint8_t ledPinGreen;
	uint8_t ledPinBlue;
	bool ledRgbInverted;
	uint8_t tempSensorPin;
	uint8_t tempAnalogSensorPin;
	uint8_t vccPin;
	int16_t vccOffset;
	uint16_t vccMultiplier;
	uint8_t vccBootLimit;
	uint16_t vccResistorGnd;
	uint16_t vccResistorVcc;
	bool hanPinPullup;
	uint8_t ledDisablePin;
	uint8_t ledBehaviour;
}; // 23

struct DomoticzConfig {
	uint16_t elidx;
	uint16_t vl1idx;
	uint16_t vl2idx;
	uint16_t vl3idx;
	uint16_t cl1idx;
}; // 10

struct HomeAssistantConfig {
	char discoveryPrefix[64];
	char discoveryHostname[64];
	char discoveryNameTag[16];
}; // 145

struct NtpConfig {
	bool enable;
	bool dhcp;
	char server[64];
	char timezone[32];
}; // 98

struct PriceServiceConfig {
	char entsoeToken[37];
	char area[17];
	char currency[4];
	uint32_t unused1;
	bool enabled;
	uint16_t unused2;
}; // 64

struct EnergyAccountingConfig {
	uint16_t thresholds[10];
	uint8_t hours;
}; // 21

struct UiConfig {
	uint8_t showImport;
	uint8_t showExport;
	uint8_t showVoltage;
	uint8_t showAmperage;
	uint8_t showReactive;
	uint8_t showRealtime;
	uint8_t showPeaks;
	uint8_t showPricePlot;
	uint8_t showDayPlot;
	uint8_t showMonthPlot;
	uint8_t showTemperaturePlot;
	uint8_t showRealtimePlot;
	uint8_t showPerPhasePower;
	uint8_t showPowerFactor;
	uint8_t darkMode;
	char language[3];
}; // 15

struct UpgradeInformation {
	char fromVersion[8];
	char toVersion[8];
    uint32_t size;
    uint16_t block_position;
    uint8_t retry_count;
    uint8_t reboot_count;
	int8_t errorCode;
}; // 25

struct CloudConfig {
	bool enabled;
	uint8_t interval;
	char hostname[64];
	uint16_t port;
	uint8_t clientId[16];
	uint8_t proto;
}; // 88

struct ZmartChargeConfig {
	bool enabled;
	char token[21];
	char baseUrl[64];
}; // 86

class AmsConfiguration {
public:
	bool hasConfig();
	int getConfigVersion();

	bool save();

	bool getSystemConfig(SystemConfig&);
	bool setSystemConfig(SystemConfig&);
	bool isSystemConfigChanged();
	void ackSystemConfigChanged();

	bool getNetworkConfig(NetworkConfig&);
	bool setNetworkConfig(NetworkConfig&);
	void clearNetworkConfig(NetworkConfig&);
	void clearNetworkConfigIp(NetworkConfig&);
	bool isNetworkConfigChanged();
	void ackNetworkConfigChange();

	bool getMqttConfig(MqttConfig&);
	bool setMqttConfig(MqttConfig&);
	void clearMqtt(MqttConfig&);
	void setMqttChanged();
	bool isMqttChanged();
	void ackMqttChange();

	bool getWebConfig(WebConfig&);
	bool setWebConfig(WebConfig&);
	void clearWebConfig(WebConfig&);
	bool isWebChanged();
	void ackWebChange();

	bool getMeterConfig(MeterConfig&);
	bool setMeterConfig(MeterConfig&);
	void clearMeter(MeterConfig&);
	void setMeterChanged();
	bool isMeterChanged();
	void ackMeterChanged();

	bool getDebugConfig(DebugConfig&);
	bool setDebugConfig(DebugConfig&);
	void clearDebug(DebugConfig&);

	bool pinUsed(uint8_t, GpioConfig&);

	bool getGpioConfig(GpioConfig&);
	bool setGpioConfig(GpioConfig&);
	void clearGpio(GpioConfig& config, bool all=true);

	void print(Print* debugger);

	bool getDomoticzConfig(DomoticzConfig&);
	bool setDomoticzConfig(DomoticzConfig&);
	void clearDomo(DomoticzConfig&);

	bool getHomeAssistantConfig(HomeAssistantConfig&);
	bool setHomeAssistantConfig(HomeAssistantConfig&);
	void clearHomeAssistantConfig(HomeAssistantConfig&);
	
	bool getNtpConfig(NtpConfig&);
	bool setNtpConfig(NtpConfig&);
	void clearNtp(NtpConfig&);
	bool isNtpChanged();
	void ackNtpChange();

	bool getPriceServiceConfig(PriceServiceConfig&);
	bool setPriceServiceConfig(PriceServiceConfig&);
	void clearPriceServiceConfig(PriceServiceConfig&);
	bool isPriceServiceChanged();
	void ackPriceServiceChange();

	bool getEnergyAccountingConfig(EnergyAccountingConfig&);
	bool setEnergyAccountingConfig(EnergyAccountingConfig&);
	void clearEnergyAccountingConfig(EnergyAccountingConfig&);
	bool isEnergyAccountingChanged();
	void ackEnergyAccountingChange();

	bool getUiConfig(UiConfig&);
	bool setUiConfig(UiConfig&);
	void clearUiConfig(UiConfig&);
	void setUiLanguageChanged();
	bool isUiLanguageChanged();
	void ackUiLanguageChange();

	bool getUpgradeInformation(UpgradeInformation&);
	bool setUpgradeInformation(UpgradeInformation&);
	void clearUpgradeInformation(UpgradeInformation&);

	bool getCloudConfig(CloudConfig&);
	bool setCloudConfig(CloudConfig&);
	void clearCloudConfig(CloudConfig&);
	bool isCloudChanged();
	void ackCloudConfig();
	
	bool getZmartChargeConfig(ZmartChargeConfig&);
	bool setZmartChargeConfig(ZmartChargeConfig&);
	void clearZmartChargeConfig(ZmartChargeConfig&);
	bool isZmartChargeConfigChanged();
	void ackZmartChargeConfig();
	

	void clear();

protected:

private:
	uint8_t configVersion = 0;

	bool sysChanged = false, networkChanged = false, mqttChanged = false, webChanged = false, meterChanged = true, ntpChanged = true, priceChanged = false, energyAccountingChanged = true, cloudChanged = true, uiLanguageChanged = false, zcChanged = true;

	bool relocateConfig103(); // 2.2.12, until, but not including 2.3

	void saveToFs();
	bool loadFromFs(uint8_t version);
	void deleteFromFs(uint8_t version);
};
#endif

