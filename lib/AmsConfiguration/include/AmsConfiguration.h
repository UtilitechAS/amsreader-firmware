#ifndef _AMSCONFIGURATION_h
#define _AMSCONFIGURATION_h
#include <EEPROM.h>
#include "Arduino.h"

#define EEPROM_SIZE 1024*3
#define EEPROM_CHECK_SUM 101 // Used to check if config is stored. Change if structure changes
#define EEPROM_CLEARED_INDICATOR 0xFC
#define EEPROM_CONFIG_ADDRESS 0
#define EEPROM_TEMP_CONFIG_ADDRESS 2048

#define CONFIG_SYSTEM_START 8
#define CONFIG_METER_START 32
#define CONFIG_UI_START 248
#define CONFIG_GPIO_START 266
#define CONFIG_ENTSOE_START 290
#define CONFIG_WIFI_START 360
#define CONFIG_ENERGYACCOUNTING_START 576
#define CONFIG_WEB_START 648
#define CONFIG_DEBUG_START 824
#define CONFIG_DOMOTICZ_START 856 
#define CONFIG_NTP_START 872
#define CONFIG_MQTT_START 1004

#define CONFIG_METER_START_93 224


struct SystemConfig {
	uint8_t boardType;
	bool vendorConfigured;
	bool userConfigured;
	uint8_t dataCollectionConsent; // 0 = unknown, 1 = accepted, 2 = declined
	char country[3];
}; // 7

struct WiFiConfig {
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
	uint8_t mode;
	bool autoreboot;
}; // 213

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
}; // 676

struct WebConfig {
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
}; // 61

struct MeterConfig100 {
	uint32_t baud;
	uint8_t parity;
	bool invert;
	uint8_t distributionSystem;
	uint8_t mainFuse;
	uint8_t productionCapacity;
	uint8_t encryptionKey[16];
	uint8_t authenticationKey[16];
	uint32_t wattageMultiplier;
	uint32_t voltageMultiplier;
	uint32_t amperageMultiplier;
	uint32_t accumulatedMultiplier;
	uint8_t source;
	uint8_t parser;
}; // 59

struct MeterConfig95 {
	uint32_t baud;
	uint8_t parity;
	bool invert;
	uint8_t distributionSystem;
	uint8_t mainFuse;
	uint8_t productionCapacity;
	uint8_t encryptionKey[16];
	uint8_t authenticationKey[16];
	uint16_t wattageMultiplier;
	uint16_t voltageMultiplier;
	uint16_t amperageMultiplier;
	uint16_t accumulatedMultiplier;
	uint8_t source;
	uint8_t parser;
}; // 50

struct DebugConfig {
	bool telnet;
	bool serial;
	uint8_t level;
}; // 3

struct GpioConfig {
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
}; // 20

struct DomoticzConfig {
	uint16_t elidx;
	uint16_t vl1idx;
	uint16_t vl2idx;
	uint16_t vl3idx;
	uint16_t cl1idx;
}; // 10

struct NtpConfig {
	bool enable;
	bool dhcp;
	char server[64];
	char timezone[32];
}; // 98

struct NtpConfig96 {
	bool enable;
	bool dhcp;
	int16_t offset;
	int16_t summerOffset;
	char server[64];
}; // 70

struct EntsoeConfig {
	char token[37];
	char area[17];
	char currency[4];
	uint32_t multiplier;
	bool enabled;
}; // 62

struct EnergyAccountingConfig {
	uint8_t thresholds[10];
	uint8_t hours;
}; // 11

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
}; // 11

struct TempSensorConfig {
	uint8_t address[8];
	char name[16];
	bool common;
};

class AmsConfiguration {
public:
	bool hasConfig();
	int getConfigVersion();

	bool save();

	bool getSystemConfig(SystemConfig&);
	bool setSystemConfig(SystemConfig&);

	bool getWiFiConfig(WiFiConfig&);
	bool setWiFiConfig(WiFiConfig&);
	void clearWifi(WiFiConfig&);
	void clearWifiIp(WiFiConfig&);
	bool isWifiChanged();
	void ackWifiChange();

	bool getMqttConfig(MqttConfig&);
	bool setMqttConfig(MqttConfig&);
	void clearMqtt(MqttConfig&);
	void setMqttChanged();
	bool isMqttChanged();
	void ackMqttChange();

	bool getWebConfig(WebConfig&);
	bool setWebConfig(WebConfig&);
	void clearAuth(WebConfig&);

	bool getMeterConfig(MeterConfig&);
	bool setMeterConfig(MeterConfig&);
	void clearMeter(MeterConfig&);
	bool isMeterChanged();
	void ackMeterChanged();

	bool getDebugConfig(DebugConfig&);
	bool setDebugConfig(DebugConfig&);
	void clearDebug(DebugConfig&);

	bool pinUsed(uint8_t, GpioConfig&);

	bool getGpioConfig(GpioConfig&);
	bool setGpioConfig(GpioConfig&);
	void clearGpio(GpioConfig&);

	void print(Print* debugger);

	bool getDomoticzConfig(DomoticzConfig&);
	bool setDomoticzConfig(DomoticzConfig&);
	void clearDomo(DomoticzConfig&);
	bool isDomoChanged();
	void ackDomoChange();
	
	bool getNtpConfig(NtpConfig&);
	bool setNtpConfig(NtpConfig&);
	void clearNtp(NtpConfig&);
	bool isNtpChanged();
	void ackNtpChange();

	bool getEntsoeConfig(EntsoeConfig&);
	bool setEntsoeConfig(EntsoeConfig&);
	void clearEntsoe(EntsoeConfig&);
	bool isEntsoeChanged();
	void ackEntsoeChange();

	bool getEnergyAccountingConfig(EnergyAccountingConfig&);
	bool setEnergyAccountingConfig(EnergyAccountingConfig&);
	void clearEnergyAccountingConfig(EnergyAccountingConfig&);
	bool isEnergyAccountingChanged();
	void ackEnergyAccountingChange();

	bool getUiConfig(UiConfig&);
	bool setUiConfig(UiConfig&);
	void clearUiConfig(UiConfig&);

	void loadTempSensors();
	void saveTempSensors();
	uint8_t getTempSensorCount();
	TempSensorConfig* getTempSensorConfig(uint8_t address[8]);
	void updateTempSensorConfig(uint8_t address[8], const char name[32], bool common);

    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);

	void clear();

protected:

private:
	uint8_t configVersion = 0;

	bool wifiChanged, mqttChanged, meterChanged = true, domoChanged, ntpChanged = true, entsoeChanged = false, energyAccountingChanged = true;

	uint8_t tempSensorCount = 0;
	TempSensorConfig** tempSensors = NULL;

	bool relocateConfig93(); // 2.1.0
	bool relocateConfig94(); // 2.1.0
	bool relocateConfig95(); // 2.1.4
	bool relocateConfig96(); // 2.1.14
	bool relocateConfig100(); // 2.2-dev

	void saveToFs();
	bool loadFromFs(uint8_t version);
	void deleteFromFs(uint8_t version);
};
#endif
