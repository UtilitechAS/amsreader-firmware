#include "AmsConfiguration.h"

String AmsConfiguration::getWifiSsid() {
	return wifiSsid;
}

void AmsConfiguration::setWifiSsid(String wifiSsid) {
	wifiChanged |= this->wifiSsid != wifiSsid;
	this->wifiSsid = String(wifiSsid);
}

String AmsConfiguration::getWifiPassword() {
	return wifiPassword;
}

void AmsConfiguration::setWifiPassword(String wifiPassword) {
	wifiChanged |= this->wifiPassword != wifiPassword;
	this->wifiPassword = String(wifiPassword);
}

String AmsConfiguration::getWifiIp() {
	return wifiIp;
}

void AmsConfiguration::setWifiIp(String wifiIp) {
	wifiChanged |= this->wifiIp != wifiIp;
	this->wifiIp = String(wifiIp);
}

String AmsConfiguration::getWifiGw() {
	return wifiGw;
}

void AmsConfiguration::setWifiGw(String wifiGw) {
	wifiChanged |= this->wifiGw != wifiGw;
	this->wifiGw = String(wifiGw);
}

String AmsConfiguration::getWifiSubnet() {
	return wifiSubnet;
}

void AmsConfiguration::setWifiSubnet(String wifiSubnet) {
	wifiChanged |= this->wifiSubnet != wifiSubnet;
	this->wifiSubnet = String(wifiSubnet);
}

String AmsConfiguration::getWifiDns1() {
	return wifiDns1;
}

void AmsConfiguration::setWifiDns1(String wifiDns1) {
	wifiChanged |= this->wifiDns1 != wifiDns1;
	this->wifiDns1 = wifiDns1;
}

String AmsConfiguration::getWifiDns2() {
	return wifiDns2;
}

void AmsConfiguration::setWifiDns2(String wifiDns2) {
	wifiChanged |= this->wifiDns2 != wifiDns2;
	this->wifiDns2 = wifiDns2;
}

String AmsConfiguration::getWifiHostname() {
	return wifiHostname;
}

void AmsConfiguration::setWifiHostname(String wifiHostname) {
	wifiChanged |= this->wifiHostname != wifiHostname;
	this->wifiHostname = wifiHostname;
}

void AmsConfiguration::clearWifiIp() {
	setWifiIp("");
	setWifiGw("");
	setWifiSubnet("");
	setWifiDns1("");
	setWifiDns2("");
}

bool AmsConfiguration::isWifiChanged() {
	return wifiChanged;
}

void AmsConfiguration::ackWifiChange() {
	wifiChanged = false;
}


String AmsConfiguration::getMqttHost() {
	return mqttHost;
}

void AmsConfiguration::setMqttHost(String mqttHost) {
	mqttChanged |= this->mqttHost != mqttHost;
	this->mqttHost = String(mqttHost);
}

int AmsConfiguration::getMqttPort() {
	return mqttPort;
}

void AmsConfiguration::setMqttPort(int mqttPort) {
	mqttChanged |= this->mqttPort != mqttPort;
	this->mqttPort = mqttPort;
}

String AmsConfiguration::getMqttClientId() {
	return mqttClientId;
}

void AmsConfiguration::setMqttClientId(String mqttClientId) {
	mqttChanged |= this->mqttClientId != mqttClientId;
	this->mqttClientId = String(mqttClientId);
}

String AmsConfiguration::getMqttPublishTopic() {
	return mqttPublishTopic;
}

void AmsConfiguration::setMqttPublishTopic(String mqttPublishTopic) {
	mqttChanged |= this->mqttPublishTopic != mqttPublishTopic;
	this->mqttPublishTopic = String(mqttPublishTopic);
}

String AmsConfiguration::getMqttSubscribeTopic() {
	return mqttSubscribeTopic;
}

void AmsConfiguration::setMqttSubscribeTopic(String mqttSubscribeTopic) {
	mqttChanged |= this->mqttSubscribeTopic != mqttSubscribeTopic;
	this->mqttSubscribeTopic = String(mqttSubscribeTopic);
}

String AmsConfiguration::getMqttUser() {
	return mqttUser;
}

void AmsConfiguration::setMqttUser(String mqttUser) {
	mqttChanged |= this->mqttUser != mqttUser;
	this->mqttUser = String(mqttUser);
}

String AmsConfiguration::getMqttPassword() {
	return mqttPassword;
}

void AmsConfiguration::setMqttPassword(String mqttPassword) {
	mqttChanged |= this->mqttPassword != mqttPassword;
	this->mqttPassword = String(mqttPassword);
}

int AmsConfiguration::getMqttPayloadFormat() {
	return this->mqttPayloadFormat;
}

void AmsConfiguration::setMqttPayloadFormat(int mqttPayloadFormat) {
	this->mqttPayloadFormat = mqttPayloadFormat;
}

bool AmsConfiguration::isMqttSsl() {
	return this->mqttSsl;
}

void AmsConfiguration::setMqttSsl(bool mqttSsl) {
	mqttChanged |= this->mqttSsl != mqttSsl;
	this->mqttSsl = mqttSsl;
}

void AmsConfiguration::clearMqtt() {
	setMqttHost("");
	setMqttPort(1883);
	setMqttClientId("");
	setMqttPublishTopic("");
	setMqttSubscribeTopic("");
	setMqttUser("");
	setMqttPassword("");
}

void AmsConfiguration::setMqttChanged() {
	mqttChanged = true;
}

bool AmsConfiguration::isMqttChanged() {
	return mqttChanged;
}

void AmsConfiguration::ackMqttChange() {
	mqttChanged = false;
}

byte AmsConfiguration::getAuthSecurity() {
	return authSecurity;
}

void AmsConfiguration::setAuthSecurity(byte authSecurity) {
	this->authSecurity = authSecurity;
}

String AmsConfiguration::getAuthUser() {
	return authUser;
}

void AmsConfiguration::setAuthUser(String authUser) {
	this->authUser = String(authUser);
}

String AmsConfiguration::getAuthPassword() {
	return authPassword;
}

void AmsConfiguration::setAuthPassword(String authPassword) {
	this->authPassword = String(authPassword);
}

void AmsConfiguration::clearAuth() {
	setAuthSecurity(0);
	setAuthUser("");
	setAuthPassword("");
}

int AmsConfiguration::getMeterType() {
	return this->meterType;
}

void AmsConfiguration::setMeterType(int meterType) {
	this->meterType = meterType;
}

int AmsConfiguration::getDistributionSystem() {
	return this->distributionSystem;
}

void AmsConfiguration::setDistributionSystem(int distributionSystem) {
	this->distributionSystem = distributionSystem;
}

int AmsConfiguration::getMainFuse() {
	return this->mainFuse;
}

void AmsConfiguration::setMainFuse(int mainFuse) {
	this->mainFuse = mainFuse;
}

int AmsConfiguration::getProductionCapacity() {
	return this->productionCapacity;
}

void AmsConfiguration::setProductionCapacity(int productionCapacity) {
	this->productionCapacity = productionCapacity;
}

bool AmsConfiguration::isDebugTelnet() {
	return this->debugTelnet;
}

void AmsConfiguration::setDebugTelnet(bool debugTelnet) {
	this->debugTelnet = debugTelnet;
}

bool AmsConfiguration::isDebugSerial() {
	return this->debugSerial;
}

void AmsConfiguration::setDebugSerial(bool debugSerial) {
	this->debugSerial = debugSerial;
}

int AmsConfiguration::getDebugLevel() {
	return this->debugLevel;
}

void AmsConfiguration::setDebugLevel(int debugLevel) {
	this->debugLevel = debugLevel;
}
//
// Domoticz start
//
int AmsConfiguration::getDomoELIDX() {
	return domoELIDX;
}
int AmsConfiguration::getDomoVL1IDX() {
	return domoVL1IDX;
}
int AmsConfiguration::getDomoVL2IDX() {
	return domoVL2IDX;
}
int AmsConfiguration::getDomoVL3IDX() {
	return domoVL3IDX;
}
int AmsConfiguration::getDomoCL1IDX() {
	return domoCL1IDX;
}
double AmsConfiguration::getDomoEnergy() {
	return domoEnergy;
}

void AmsConfiguration::setDomoELIDX(int domoELIDX) {
	domoChanged |= this->domoELIDX != domoELIDX;
	this->domoELIDX = domoELIDX;
}
void AmsConfiguration::setDomoVL1IDX(int domoVL1IDX) {
	domoChanged |= this->domoVL1IDX != domoVL1IDX;
	this->domoVL1IDX = domoVL1IDX;
}
void AmsConfiguration::setDomoVL2IDX(int domoVL2IDX) {
	domoChanged |= this->domoVL2IDX != domoVL2IDX;
	this->domoVL2IDX = domoVL2IDX;
}
void AmsConfiguration::setDomoVL3IDX(int domoVL3IDX) {
	domoChanged |= this->domoVL3IDX != domoVL3IDX;
	this->domoVL3IDX = domoVL3IDX;
}
void AmsConfiguration::setDomoCL1IDX(int domoCL1IDX) {
	domoChanged |= this->domoCL1IDX != domoCL1IDX;
	this->domoCL1IDX = domoCL1IDX;
}
void AmsConfiguration::setDomoEnergy(double domoEnergy) {
	domoChanged |= this->domoEnergy != domoEnergy;
	this->domoEnergy = domoEnergy;
}
void AmsConfiguration::clearDomo() {
	setDomoELIDX(0);
	setDomoVL1IDX(0);	
	setDomoVL2IDX(0);	
	setDomoVL3IDX(0);
	setDomoCL1IDX(0);
	setDomoEnergy(-1.0);	
}

bool AmsConfiguration::isDomoChanged() {
	return domoChanged;
}

void AmsConfiguration::ackDomoChange() {
	domoChanged = false;
}
//
// Domoticz end
//

bool AmsConfiguration::hasConfig() {
	if(configVersion == 0) {
		EEPROM.begin(EEPROM_SIZE);
		configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
		EEPROM.end();
	}
	switch(configVersion) {
		case 71:
		case 72:
		case 75:
		case 80:
		case 81:
		case 82:
			return true;
		default:
			configVersion = 0;
			return false;
	}
}

int AmsConfiguration::getConfigVersion() {
	return configVersion;
}

bool AmsConfiguration::load() {
	int address = EEPROM_CONFIG_ADDRESS;
	bool success = false;

	EEPROM.begin(EEPROM_SIZE);
	int cs = EEPROM.read(address);
	address++;
	switch(cs) {
		case 71: // Same as 72
		case 72:
			success = loadConfig72(address);
			break;
		case 75:
			success = loadConfig75(address);
			break;
		case 80:
			success = loadConfig80(address);
			break;
		case 81:
			success = loadConfig81(address);
			break;
		case 82:
			success = loadConfig82(address);
			break;
	}
	EEPROM.end();
	return success;
}

bool AmsConfiguration::loadConfig72(int address) {
	char* temp;

	address += readString(address, &temp);
	setWifiSsid(temp);
	address += readString(address, &temp);
	setWifiPassword(temp);

	byte b;
	address += readByte(address, &b);
	setMeterType(b);

	address += readString(address, &temp);
	setMqttHost(temp);
	int port;
	address += readInt(address, &port);
	setMqttPort(port);
	address += readString(address, &temp);
	setMqttClientId(temp);
	address += readString(address, &temp);
	setMqttPublishTopic(temp);
	address += readString(address, &temp);
	setMqttSubscribeTopic(temp);

	bool secure = false;
	address += readBool(address, &secure);
	if (secure) {
		address += readString(address, &temp);
		setMqttUser(temp);
		address += readString(address, &temp);
		setMqttPassword(temp);
	} else {
		setMqttUser("");
		setMqttPassword("");
	}
	setMqttPayloadFormat(0);

	clearAuth();

	setWifiIp("");
	setWifiGw("");
	setWifiSubnet("");
	setMainFuse(0);
	setProductionCapacity(0);
	setDistributionSystem(0);

	ackWifiChange();

	setDebugLevel(3); // 3=Info
	setDebugTelnet(false);
	setDebugSerial(false);

	return true;
}

bool AmsConfiguration::loadConfig75(int address) {
	char* temp;

	address += readString(address, &temp);
	setWifiSsid(temp);
	address += readString(address, &temp);
	setWifiPassword(temp);

	byte b;
	address += readByte(address, &b);
	setMeterType(b);

	bool mqtt = false;
	address += readBool(address, &mqtt);
	if(mqtt) {
		address += readString(address, &temp);
		setMqttHost(temp);
		int port;
		address += readInt(address, &port);
		setMqttPort(port);
		address += readString(address, &temp);
		setMqttClientId(temp);
		address += readString(address, &temp);
		setMqttPublishTopic(temp);
		address += readString(address, &temp);
		setMqttSubscribeTopic(temp);
	}

	bool secure = false;
	address += readBool(address, &secure);
	if (secure) {
		address += readString(address, &temp);
		setMqttUser(temp);
		address += readString(address, &temp);
		setMqttPassword(temp);
	} else {
		setMqttUser("");
		setMqttPassword("");
	}
	setMqttPayloadFormat(0);

	address += readByte(address, &authSecurity);
	if (authSecurity > 0) {
		address += readString(address, &temp);
		setAuthUser(temp);
		address += readString(address, &temp);
		setAuthPassword(temp);
	} else {
		clearAuth();
	}

	int i;
	address += readInt(address, &i);
	setMainFuse(i);
	address += readByte(address, &b);
	setDistributionSystem(b);

	setWifiIp("");
	setWifiGw("");
	setWifiSubnet("");
	setProductionCapacity(0);

	ackWifiChange();

	setDebugLevel(3); // 3=Info
	setDebugTelnet(false);
	setDebugSerial(false);

	return true;
}

bool AmsConfiguration::loadConfig80(int address) {
	char* temp;

	address += readString(address, &temp);
	setWifiSsid(temp);
	address += readString(address, &temp);
	setWifiPassword(temp);
	address += readString(address, &temp);
	setWifiIp(temp);
	address += readString(address, &temp);
	setWifiGw(temp);
	address += readString(address, &temp);
	setWifiSubnet(temp);

	bool mqtt = false;
	address += readBool(address, &mqtt);
	if(mqtt) {
		address += readString(address, &temp);
		setMqttHost(temp);
		int port;
		address += readInt(address, &port);
		setMqttPort(port);
		address += readString(address, &temp);
		setMqttClientId(temp);
		address += readString(address, &temp);
		setMqttPublishTopic(temp);
		address += readString(address, &temp);
		setMqttSubscribeTopic(temp);

		bool secure = false;
		address += readBool(address, &secure);
		if (secure)
		{
			address += readString(address, &temp);
			setMqttUser(temp);
			address += readString(address, &temp);
			setMqttPassword(temp);
		} else {
			setMqttUser("");
			setMqttPassword("");
		}
	} else {
		clearMqtt();
	}
	setMqttPayloadFormat(0);

	address += readByte(address, &authSecurity);
	if (authSecurity > 0) {
		address += readString(address, &temp);
		setAuthUser(temp);
		address += readString(address, &temp);
		setAuthPassword(temp);
	} else {
		clearAuth();
	}

	int i;
	address += readInt(address, &i);
	setMeterType(i);
	address += readInt(address, &i);
	setDistributionSystem(i);
	address += readInt(address, &i);
	setMainFuse(i);
	address += readInt(address, &i);
	setProductionCapacity(i);

	ackWifiChange();

	setDebugLevel(3); // 3=Info
	setDebugTelnet(false);
	setDebugSerial(false);

	return true;
}

bool AmsConfiguration::loadConfig81(int address) {
	char* temp;

	address += readString(address, &temp);
	setWifiSsid(temp);
	address += readString(address, &temp);
	setWifiPassword(temp);

	bool staticIp = false;
	address += readBool(address, &staticIp);
	if(staticIp) {
		address += readString(address, &temp);
		setWifiIp(temp);
		address += readString(address, &temp);
		setWifiGw(temp);
		address += readString(address, &temp);
		setWifiSubnet(temp);
		address += readString(address, &temp);
		setWifiDns1(temp);
		address += readString(address, &temp);
		setWifiDns2(temp);
	}
	address += readString(address, &temp);
	setWifiHostname(temp);
	bool mqtt = false;
	address += readBool(address, &mqtt);
	if(mqtt) {
		address += readString(address, &temp);
		setMqttHost(temp);
		int port;
		address += readInt(address, &port);
		setMqttPort(port);
		address += readString(address, &temp);
		setMqttClientId(temp);
		address += readString(address, &temp);
		setMqttPublishTopic(temp);
		address += readString(address, &temp);
		setMqttSubscribeTopic(temp);

		bool secure = false;
		address += readBool(address, &secure);
		if (secure)
		{
			address += readString(address, &temp);
			setMqttUser(temp);
			address += readString(address, &temp);
			setMqttPassword(temp);
		} else {
			setMqttUser("");
			setMqttPassword("");
		}
		int payloadFormat;
		address += readInt(address, &payloadFormat);
		setMqttPayloadFormat(payloadFormat);
	} else {
		clearMqtt();
	}

	address += readByte(address, &authSecurity);
	if (authSecurity > 0) {
		address += readString(address, &temp);
		setAuthUser(temp);
		address += readString(address, &temp);
		setAuthPassword(temp);
	} else {
		clearAuth();
	}

	int i;
	address += readInt(address, &i);
	setMeterType(i);
	address += readInt(address, &i);
	setDistributionSystem(i);
	address += readInt(address, &i);
	setMainFuse(i);
	address += readInt(address, &i);
	setProductionCapacity(i);

	bool debugTelnet = false;
	address += readBool(address, &debugTelnet);
	setDebugTelnet(debugTelnet);
	bool debugSerial = false;
	address += readBool(address, &debugSerial);
	setDebugSerial(debugSerial);
	address += readInt(address, &i);
	setDebugLevel(i);

	ackWifiChange();

	return true;
}

bool AmsConfiguration::loadConfig82(int address) {
	char* temp;

	address += readString(address, &temp);
	setWifiSsid(temp);
	address += readString(address, &temp);
	setWifiPassword(temp);

	bool staticIp = false;
	address += readBool(address, &staticIp);
	if(staticIp) {
		address += readString(address, &temp);
		setWifiIp(temp);
		address += readString(address, &temp);
		setWifiGw(temp);
		address += readString(address, &temp);
		setWifiSubnet(temp);
		address += readString(address, &temp);
		setWifiDns1(temp);
		address += readString(address, &temp);
		setWifiDns2(temp);
	}
	address += readString(address, &temp);
	setWifiHostname(temp);
	bool mqtt = false;
	address += readBool(address, &mqtt);
	if(mqtt) {
		address += readString(address, &temp);
		setMqttHost(temp);
		int port;
		address += readInt(address, &port);
		setMqttPort(port);
		address += readString(address, &temp);
		setMqttClientId(temp);
		address += readString(address, &temp);
		setMqttPublishTopic(temp);
		address += readString(address, &temp);
		setMqttSubscribeTopic(temp);

		bool secure = false;
		address += readBool(address, &secure);
		if (secure)
		{
			address += readString(address, &temp);
			setMqttUser(temp);
			address += readString(address, &temp);
			setMqttPassword(temp);
		} else {
			setMqttUser("");
			setMqttPassword("");
		}
		int payloadFormat;
		address += readInt(address, &payloadFormat);
		setMqttPayloadFormat(payloadFormat);
		bool ssl = false;
		address += readBool(address, &ssl);
		setMqttSsl(ssl);
	} else {
		clearMqtt();
	}

	address += readByte(address, &authSecurity);
	if (authSecurity > 0) {
		address += readString(address, &temp);
		setAuthUser(temp);
		address += readString(address, &temp);
		setAuthPassword(temp);
	} else {
		clearAuth();
	}

	int i;
	address += readInt(address, &i);
	setMeterType(i);
	address += readInt(address, &i);
	setDistributionSystem(i);
	address += readInt(address, &i);
	setMainFuse(i);
	address += readInt(address, &i);
	setProductionCapacity(i);

	bool debugTelnet = false;
	address += readBool(address, &debugTelnet);
	setDebugTelnet(debugTelnet);
	bool debugSerial = false;
	address += readBool(address, &debugSerial);
	setDebugSerial(debugSerial);
	address += readInt(address, &i);
	setDebugLevel(i);

	bool domo = false;
	address += readBool(address, &domo);
	if(domo) {
		int domoELIDX;
		address += readInt(address, &domoELIDX);
		setDomoELIDX(domoELIDX);
		int domoVL1IDX;
		address += readInt(address, &domoVL1IDX);
		setDomoVL1IDX(domoVL1IDX);
		int domoVL2IDX;
		address += readInt(address, &domoVL2IDX);
		setDomoVL2IDX(domoVL2IDX);
		int domoVL3IDX;
		address += readInt(address, &domoVL3IDX);
		setDomoVL3IDX(domoVL3IDX);
		int domoCL1IDX;
		address += readInt(address, &domoCL1IDX);
		setDomoCL1IDX(domoCL1IDX);
		
	} else {
		clearDomo();
	}
	
	ackWifiChange();

	return true;
}

bool AmsConfiguration::save() {
	int address = EEPROM_CONFIG_ADDRESS;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(address, EEPROM_CHECK_SUM);
	address++;

	address += saveString(address, wifiSsid.c_str());
	address += saveString(address, wifiPassword.c_str());
	if(!wifiIp.isEmpty()) {
		address += saveBool(address, true);
		address += saveString(address, wifiIp.c_str());
		address += saveString(address, wifiGw.c_str());
		address += saveString(address, wifiSubnet.c_str());
		address += saveString(address, wifiDns1.c_str());
		address += saveString(address, wifiDns2.c_str());
	} else {
		address += saveBool(address, false);
	}
	address += saveString(address, wifiHostname.c_str());
	if(!mqttHost.isEmpty()) {
		address += saveBool(address, true);
		address += saveString(address, mqttHost.c_str());
		address += saveInt(address, mqttPort);
		address += saveString(address, mqttClientId.c_str());
		address += saveString(address, mqttPublishTopic.c_str());
		address += saveString(address, mqttSubscribeTopic.c_str());
		if (!mqttUser.isEmpty()) {
			address += saveBool(address, true);
			address += saveString(address, mqttUser.c_str());
			address += saveString(address, mqttPassword.c_str());
		} else {
			address += saveBool(address, false);
		}
		address += saveInt(address, mqttPayloadFormat);
		address += saveBool(address, mqttSsl);
	} else {
		address += saveBool(address, false);
	}

	address += saveByte(address, authSecurity);
	if (authSecurity > 0) {
		address += saveString(address, authUser.c_str());
		address += saveString(address, authPassword.c_str());
	}

	address += saveInt(address, meterType);
	address += saveInt(address, distributionSystem);
	address += saveInt(address, mainFuse);
	address += saveInt(address, productionCapacity);

	address += saveBool(address, debugTelnet);
	address += saveBool(address, debugSerial);
	address += saveInt(address, debugLevel);
//
//  Domoticz 
//
	if(domoELIDX) {
		address += saveBool(address, true);
		address += saveInt(address, domoELIDX);	
		address += saveInt(address, domoVL1IDX);	
		address += saveInt(address, domoVL2IDX);
		address += saveInt(address, domoVL3IDX);
		address += saveInt(address, domoCL1IDX);
	} else {
		address += saveBool(address, false);
	}
	bool success = EEPROM.commit();
	EEPROM.end();

	configVersion = EEPROM_CHECK_SUM;

	return success;
}

int AmsConfiguration::readString(int pAddress, char* pString[]) {
	int address = 0;
	byte length = EEPROM.read(pAddress + address);
	address++;

	char* buffer = new char[length];
	for (int i = 0; i<length; i++)
	{
		buffer[i] = EEPROM.read(pAddress + address++);
	}
	*pString = buffer;
	return address;
}

int AmsConfiguration::saveString(int pAddress, const char* pString) {
	int address = 0;
	int length = pString ? strlen(pString) + 1 : 0;
	EEPROM.put(pAddress + address, length);
	address++;

	for (int i = 0; i < length; i++)
	{
		EEPROM.put(pAddress + address, pString[i]);
		address++;
	}

	return address;
}

int AmsConfiguration::readInt(int address, int *value) {
	int lower = EEPROM.read(address);
	int higher = EEPROM.read(address + 1);
	*value = lower + (higher << 8);
	return 2;
}

int AmsConfiguration::saveInt(int address, int value) {
	byte lowByte = value & 0xFF;
	byte highByte = ((value >> 8) & 0xFF);

	EEPROM.write(address, lowByte);
	EEPROM.write(address + 1, highByte);

	return 2;
}

int AmsConfiguration::readBool(int address, bool *value) {
	byte y = EEPROM.read(address);
	*value = (bool)y;
	return 1;
}

int AmsConfiguration::saveBool(int address, bool value) {
	byte y = (byte)value;
	EEPROM.write(address, y);
	return 1;
}

int AmsConfiguration::readByte(int address, byte *value) {
	*value = EEPROM.read(address);
	return 1;
}

int AmsConfiguration::saveByte(int address, byte value) {
	EEPROM.write(address, value);
	return 1;
}

template <class T> int AmsConfiguration::writeAnything(int ee, const T& value) {
	const byte* p = (const byte*)(const void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		EEPROM.write(ee++, *p++);
	return i;
}

template <class T> int AmsConfiguration::readAnything(int ee, T& value) {
	byte* p = (byte*)(void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		*p++ = EEPROM.read(ee++);
	return i;
}

void AmsConfiguration::print(Print* debugger)
{
	debugger->println("Configuration:");
	debugger->println("-----------------------------------------------");
	debugger->printf("WiFi SSID:            %s\r\n", this->getWifiSsid().c_str());
	debugger->printf("WiFi Psk:             %s\r\n", this->getWifiPassword().c_str());
	
	if(!getWifiIp().isEmpty()) {
		debugger->printf("IP:                   %s\r\n", this->getWifiIp().c_str());
		debugger->printf("Gateway:              %s\r\n", this->getWifiGw().c_str());
		debugger->printf("Subnet:               %s\r\n", this->getWifiSubnet().c_str());
		debugger->printf("Primary DNS:          %s\r\n", this->getWifiDns1().c_str());
		debugger->printf("Secondary DNS:        %s\r\n", this->getWifiDns2().c_str());
	}

	if(!getMqttHost().isEmpty()) {
		debugger->printf("mqttHost:             %s\r\n", this->getMqttHost().c_str());
		debugger->printf("mqttPort:             %i\r\n", this->getMqttPort());
		debugger->printf("mqttClientID:         %s\r\n", this->getMqttClientId().c_str());
		debugger->printf("mqttPublishTopic:     %s\r\n", this->getMqttPublishTopic().c_str());
		debugger->printf("mqttSubscribeTopic:   %s\r\n", this->getMqttSubscribeTopic().c_str());
		if (this->getMqttUser()) {
			debugger->printf("SECURE MQTT CONNECTION:\r\n");
			debugger->printf("mqttUser:             %s\r\n", this->getMqttUser().c_str());
			debugger->printf("mqttPass:             %s\r\n", this->getMqttPassword().c_str());
		}
		debugger->printf("payload format:       %i\r\n", this->getMqttPayloadFormat());
	}

	if (this->getAuthSecurity()) {
		debugger->printf("WEB AUTH:\r\n");
		debugger->printf("authSecurity:         %i\r\n", this->getAuthSecurity());
		debugger->printf("authUser:             %s\r\n", this->getAuthUser().c_str());
		debugger->printf("authPass:             %s\r\n", this->getAuthPassword().c_str());
	}

	debugger->printf("meterType:            %i\r\n", this->getMeterType());
	debugger->printf("distSys:              %i\r\n", this->getDistributionSystem());
	debugger->printf("fuseSize:             %i\r\n", this->getMainFuse());
	debugger->printf("productionCapacity:   %i\r\n", this->getProductionCapacity());

	debugger->println("-----------------------------------------------");
}
