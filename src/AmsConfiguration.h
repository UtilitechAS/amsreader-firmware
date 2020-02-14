#ifndef _AMSCONFIGURATION_h
#define _AMSCONFIGURATION_h
#include <EEPROM.h>
#include "Arduino.h"

class AmsConfiguration {
public:
	bool hasConfig();
	bool load();
	bool save();
	
	String getWifiSsid();
	void setWifiSsid(String wifiSsid);
	String getWifiPassword();
	void setWifiPassword(String wifiPassword);
	String getWifiIp();
	void setWifiIp(String wifiIp);
	String getWifiGw();
	void setWifiGw(String wifiGw);
	String getWifiSubnet();
	void setWifiSubnet(String wifiSubnet);
	void clearWifiIp();

	bool isWifiChanged();
	void ackWifiChange();

	String getMqttHost();
	void setMqttHost(String mqttHost);
	int getMqttPort();
	void setMqttPort(int mqttPort);
	String getMqttClientId();
	void setMqttClientId(String mqttClientId);
	String getMqttPublishTopic();
	void setMqttPublishTopic(String mqttPublishTopic);
	String getMqttSubscribeTopic();
	void setMqttSubscribeTopic(String mqttSubscribeTopic);
	String getMqttUser();
	void setMqttUser(String mqttUser);
	String getMqttPassword();
	void setMqttPassword(String mqttPassword);
	void clearMqtt();

	bool isMqttChanged();
	void ackMqttChange();

	byte getAuthSecurity();
	void setAuthSecurity(byte authSecurity);
	String getAuthUser();
	void setAuthUser(String authUser);
	String getAuthPassword();
	void setAuthPassword(String authPassword);
	void clearAuth();

	int getMeterType();
	void setMeterType(int meterType);
	int getDistributionSystem();
	void setDistributionSystem(int distributionSystem);
	int getMainFuse();
	void setMainFuse(int mainFuse);
	int getProductionCapacity();
	void setProductionCapacity(int productionCapacity);

	void print(Stream* debugger);
	
protected:

private:
	String wifiSsid;
	String wifiPassword;
    String wifiIp;
    String wifiGw;
    String wifiSubnet;
	bool wifiChanged;
	
	String mqttHost;
	int mqttPort;
	String mqttClientId;
	String mqttPublishTopic;
	String mqttSubscribeTopic;
	String mqttUser;
	String mqttPassword;
	bool mqttChanged;

	byte authSecurity;
	String authUser;
	String authPassword;

	int meterType, distributionSystem, mainFuse, productionCapacity;

	const int EEPROM_SIZE = 512;
	const byte EEPROM_CHECK_SUM = 80; // Used to check if config is stored. Change if structure changes
	const int EEPROM_CONFIG_ADDRESS = 0;

	int saveString(int pAddress, const char* pString);
	int readString(int pAddress, char* pString[]);
	int saveInt(int pAddress, int pValue);
	int readInt(int pAddress, int *pValue);
	int saveBool(int pAddress, bool pValue);
	int readBool(int pAddress, bool *pValue);
	int saveByte(int pAddress, byte pValue);
	int readByte(int pAddress, byte *pValue);

	template <class T> int writeAnything(int ee, const T& value);
	template <class T> int readAnything(int ee, T& value);
};
#endif
