#ifndef _AMSCONFIGURATION_h
#define _AMSCONFIGURATION_h
#include <EEPROM.h>
#include "Arduino.h"

#define EEPROM_SIZE 1024 * 3
#define EEPROM_CHECK_SUM 86 // Used to check if config is stored. Change if structure changes
#define EEPROM_CONFIG_ADDRESS 0
#define EEPROM_TEMP_CONFIG_ADDRESS 2048

#define CONFIG_SYSTEM_START 8
#define CONFIG_WIFI_START 16
#define CONFIG_MQTT_START 224
#define CONFIG_WEB_START 648
#define CONFIG_METER_START 784
#define CONFIG_DEBUG_START 824
#define CONFIG_GPIO_START 832
#define CONFIG_DOMOTICZ_START 856 
#define CONFIG_NTP_START 872
#define CONFIG_ENTSOE_START 944

struct SystemConfig {
	uint8_t boardType;
}; // 1

struct WiFiConfig {
	char ssid[32];
	char psk[64];
    char ip[15];
    char gateway[15];
    char subnet[15];
	char dns1[15];
	char dns2[15];
	char hostname[32];
	bool mdns;
}; // 204 

struct MqttConfig {
	char host[128];
	uint16_t port;
	char clientId[32];
	char publishTopic[64];
	char subscribeTopic[64];
	char username[64];
	char password[64];
	uint8_t payloadFormat;
	bool ssl;
}; // 420

struct WebConfig {
	uint8_t security;
	char username[64];
	char password[64];
}; // 129

struct MeterConfig {
	uint8_t type;
	uint8_t distributionSystem;
	uint8_t mainFuse;
	uint8_t productionCapacity;
	uint8_t encryptionKey[16];
	uint8_t authenticationKey[16];
	bool substituteMissing;
}; // 37

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
}; // 16

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
	int16_t offset;
	int16_t summerOffset;
	char server[64];
}; // 70

struct EntsoeConfig {
	char token[37];
	char area[17];
	char currency[4];
	uint16_t multiplier;
}; // 60

struct ConfigObject83 {
	uint8_t boardType;
	char wifiSsid[32];
	char wifiPassword[64];
    char wifiIp[15];
    char wifiGw[15];
    char wifiSubnet[15];
	char wifiDns1[15];
	char wifiDns2[15];
	char wifiHostname[32];
	char mqttHost[128];
	uint16_t mqttPort;
	char mqttClientId[32];
	char mqttPublishTopic[64];
	char mqttSubscribeTopic[64];
	char mqttUser[64];
	char mqttPassword[64];
	uint8_t mqttPayloadFormat;
	bool mqttSsl;
	uint8_t authSecurity;
	char authUser[64];
	char authPassword[64];

	uint8_t meterType;
	uint8_t distributionSystem;
	uint8_t mainFuse;
	uint8_t productionCapacity;
	uint8_t meterEncryptionKey[16];
	uint8_t meterAuthenticationKey[16];
	bool substituteMissing;
	bool sendUnknown;

	bool debugTelnet;
	bool debugSerial;
	uint8_t debugLevel;

	uint8_t hanPin;
	uint8_t apPin;
	uint8_t ledPin;
	bool ledInverted;
	uint8_t ledPinRed;
	uint8_t ledPinGreen;
	uint8_t ledPinBlue;
	bool ledRgbInverted;
	uint8_t tempSensorPin;
	uint8_t vccPin;
	int16_t vccOffset;
	uint16_t vccMultiplier;
	uint8_t vccBootLimit;

	uint16_t domoELIDX;
	uint16_t domoVL1IDX;
	uint16_t domoVL2IDX;
	uint16_t domoVL3IDX;
	uint16_t domoCL1IDX;

	bool mDnsEnable;
	bool ntpEnable;
	bool ntpDhcp;
	int16_t ntpOffset;
	int16_t ntpSummerOffset;
	char ntpServer[64];

	uint8_t tempAnalogSensorPin;
};

struct ConfigObject82 {
	uint8_t boardType;
	char wifiSsid[32];
	char wifiPassword[64];
    char wifiIp[15];
    char wifiGw[15];
    char wifiSubnet[15];
	char wifiDns1[15];
	char wifiDns2[15];
	char wifiHostname[32];
	char mqttHost[128];
	uint16_t mqttPort;
	char mqttClientId[32];
	char mqttPublishTopic[64];
	char mqttSubscribeTopic[64];
	char mqttUser[64];
	char mqttPassword[64];
	uint8_t mqttPayloadFormat;
	bool mqttSsl;
	uint8_t authSecurity;
	char authUser[64];
	char authPassword[64];

	uint8_t meterType;
	uint8_t distributionSystem;
	uint8_t mainFuse;
	uint8_t productionCapacity;
	bool substituteMissing;
	bool sendUnknown;

	bool debugTelnet;
	bool debugSerial;
	uint8_t debugLevel;

	uint8_t hanPin;
	uint8_t apPin;
	uint8_t ledPin;
	bool ledInverted;
	uint8_t ledPinRed;
	uint8_t ledPinGreen;
	uint8_t ledPinBlue;
	bool ledRgbInverted;
	uint8_t tempSensorPin;
	uint8_t vccPin;
	uint16_t vccMultiplier;
	uint8_t vccBootLimit;

	uint16_t domoELIDX;
	uint16_t domoVL1IDX;
	uint16_t domoVL2IDX;
	uint16_t domoVL3IDX;
	uint16_t domoCL1IDX;
};

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

	uint8_t getTempSensorCount();
	TempSensorConfig* getTempSensorConfig(uint8_t address[8]);
	void updateTempSensorConfig(uint8_t address[8], const char name[32], bool common);

    bool isSensorAddressEqual(uint8_t a[8], uint8_t b[8]);

	void clear();

protected:

private:
	uint8_t configVersion = 0;

	bool wifiChanged, mqttChanged, meterChanged = true, domoChanged, ntpChanged = true;

	uint8_t tempSensorCount = 0;
	TempSensorConfig** tempSensors;

	void loadTempSensors();
	void saveTempSensors();

	bool loadConfig82(int address);
	bool loadConfig83(int address);

	int readString(int pAddress, char* pString[]);
	int readInt(int pAddress, int *pValue);
	int readBool(int pAddress, bool *pValue);
	int readByte(int pAddress, byte *pValue);
};
#endif
