#ifndef _AMSCONFIGURATION_h
#define _AMSCONFIGURATION_h
#include <EEPROM.h>
#include "Arduino.h"

struct ConfigObject {
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

	int domoELIDX;
	int domoVL1IDX;
	int domoVL2IDX;
	int domoVL3IDX;
	int domoCL1IDX;
};

class AmsConfiguration {
public:
	bool hasConfig();
	int getConfigVersion();

	bool load();
	bool save();
	
	char* getWifiSsid();
	void setWifiSsid(const char* wifiSsid);
	char* getWifiPassword();
	void setWifiPassword(const char* wifiPassword);
	char* getWifiIp();
	void setWifiIp(const char* wifiIp);
	char* getWifiGw();
	void setWifiGw(const char* wifiGw);
	char* getWifiSubnet();
	void setWifiSubnet(const char* wifiSubnet);
	char* getWifiDns1();
	void setWifiDns1(const char* wifiDns1);
	char* getWifiDns2();
	void setWifiDns2(const char* wifiDns1);
	char* getWifiHostname();
	void setWifiHostname(const char* wifiHostname);
	void clearWifiIp();

	bool isWifiChanged();
	void ackWifiChange();

	char* getMqttHost();
	void setMqttHost(const char* mqttHost);
	uint16_t getMqttPort();
	void setMqttPort(uint16_t mqttPort);
	char* getMqttClientId();
	void setMqttClientId(const char* mqttClientId);
	char* getMqttPublishTopic();
	void setMqttPublishTopic(const char* mqttPublishTopic);
	char* getMqttSubscribeTopic();
	void setMqttSubscribeTopic(const char* mqttSubscribeTopic);
	char* getMqttUser();
	void setMqttUser(const char* mqttUser);
	char* getMqttPassword();
	void setMqttPassword(const char* mqttPassword);
	uint8_t getMqttPayloadFormat();
	void setMqttPayloadFormat(uint8_t mqttPayloadFormat);
	bool isMqttSsl();
	void setMqttSsl(bool mqttSsl);
	void clearMqtt();

	void setMqttChanged();
	bool isMqttChanged();
	void ackMqttChange();

	byte getAuthSecurity();
	void setAuthSecurity(byte authSecurity);
	char* getAuthUser();
	void setAuthUser(const char* authUser);
	char* getAuthPassword();
	void setAuthPassword(const char* authPassword);
	void clearAuth();

	uint8_t getMeterType();
	void setMeterType(uint8_t meterType);
	uint8_t getDistributionSystem();
	void setDistributionSystem(uint8_t distributionSystem);
	uint8_t getMainFuse();
	void setMainFuse(uint8_t mainFuse);
	uint8_t getProductionCapacity();
	void setProductionCapacity(uint8_t productionCapacity);

	bool isDebugTelnet();
	void setDebugTelnet(bool debugTelnet);
	bool isDebugSerial();
	void setDebugSerial(bool debugSerial);
	uint8_t getDebugLevel();
	void setDebugLevel(uint8_t debugLevel);

	bool pinUsed(uint8_t pin);

	uint8_t getHanPin();
	void setHanPin(uint8_t hanPin);
	uint8_t getApPin();
	void setApPin(uint8_t apPin);
	uint8_t getLedPin();
	void setLedPin(uint8_t ledPin);
	bool isLedInverted();
	void setLedInverted(bool ledInverted);
	
	uint8_t getLedPinRed();
	void setLedPinRed(uint8_t ledPinRed);
	uint8_t getLedPinGreen();
	void setLedPinGreen(uint8_t ledPinGreen);
	uint8_t getLedPinBlue();
	void setLedPinBlue(uint8_t ledPinBlue);
	bool isLedRgbInverted();
	void setLedRgbInverted(bool ledRgbInverted);

	uint8_t getTempSensorPin();
	void setTempSensorPin(uint8_t tempSensorPin);
	uint8_t getVccPin();
	void setVccPin(uint8_t vccPin);
	double getVccMultiplier();
	void setVccMultiplier(double vccMultiplier);
	double getVccBootLimit();
	void setVccBootLimit(double vccBootLimit);

	void print(Print* debugger);

	int getDomoELIDX();
	int getDomoVL1IDX();
	int getDomoVL2IDX();
	int getDomoVL3IDX();
	int getDomoCL1IDX();
	double getDomoEnergy();
	void setDomoELIDX(int domoELIDX);
	void setDomoVL1IDX(int domoVL1IDX);
	void setDomoVL2IDX(int domoVL2IDX);
	void setDomoVL3IDX(int domoVL3IDX);
	void setDomoCL1IDX(int domoCL1IDX);
	void clearDomo();

	bool isDomoChanged();
	void ackDomoChange();

protected:

private:
	int configVersion = 0;
	ConfigObject config {
		"", // SSID
		"", // PSK
		"", // IP
		"", // GW
		"", // Subnet
		"", // DNS 1
		"", // DNS 2
		"", // Hostname
		"", // MQTT host
		1883, // Port
		"", // Client ID
		"", // Publish topic
		"", // Subscribe topic
		"", // Username
		"", // Password
		0, // Format
		false, // SSL
		0, // Web security
		"", // Username
		"", // Password
		0, // Meter type
		0, // Distribution system
		0, // Main fuse
		0, // Production capacity
		false, // Debug telnet
		false, // Debug serial
		5, // Debug level
		0x03, // HAN pin
		0xFF, // AP pin
		0x02, // LED pin
		true, // Inverted
		0xFF, // Red
		0xFF, // Green
		0xFF, // Blue
		true, // Inverted
		0xFF, // Temp sensor
		0xFF, // Vcc
		100, // Multiplier
		0, // Boot limit
		//Domoticz
		0, // ELIDX
		0, // VL1IDX
		0, // VL2IDX
		0, // VL3IDX
		0 // CL1IDX
	};
	bool wifiChanged, mqttChanged, domoChanged;

	const int EEPROM_SIZE = 2048;
	const int EEPROM_CHECK_SUM = 82; // Used to check if config is stored. Change if structure changes
	const int EEPROM_CONFIG_ADDRESS = 0;

	bool loadConfig80(int address);
	bool loadConfig81(int address);

	int readString(int pAddress, char* pString[]);
	int readInt(int pAddress, int *pValue);
	int readBool(int pAddress, bool *pValue);
	int readByte(int pAddress, byte *pValue);
};
#endif
