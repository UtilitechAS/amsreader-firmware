/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "AmsConfiguration.h"
#include "hexutils.h"
#if defined(ESP32)
#include "ESPRandom.h"
#endif

bool AmsConfiguration::getSystemConfig(SystemConfig& config) {
	EEPROM.begin(EEPROM_SIZE);
	uint8_t configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
	if(configVersion == EEPROM_CHECK_SUM) {
		EEPROM.get(CONFIG_SYSTEM_START, config);
		EEPROM.end();
		return true;
	} else {
		if(configVersion == EEPROM_CLEARED_INDICATOR) {
			config.vendorConfigured = true;
		} else {
			config.vendorConfigured = false;
			config.boardType = 0xFF;
		}
		config.userConfigured = false;
		config.dataCollectionConsent = 0;
		config.energyspeedometer = 0;
		memset(config.country, 0, 3);
		return false;
	}
}

bool AmsConfiguration::setSystemConfig(SystemConfig& config) {
	SystemConfig existing;
	if(getSystemConfig(existing)) {
		sysChanged |= config.boardType != existing.boardType;
		sysChanged |= config.vendorConfigured != existing.vendorConfigured;
		sysChanged |= config.userConfigured != existing.userConfigured;
		sysChanged |= config.dataCollectionConsent != existing.dataCollectionConsent;
		sysChanged |= strcmp(config.country, existing.country) != 0;
		sysChanged |= config.energyspeedometer != existing.energyspeedometer;
	}
	EEPROM.begin(EEPROM_SIZE);
	stripNonAscii((uint8_t*) config.country, 2);
	EEPROM.put(CONFIG_SYSTEM_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::isSystemConfigChanged() {
	return sysChanged;
}

void AmsConfiguration::ackSystemConfigChanged() {
	sysChanged = false;
}

bool AmsConfiguration::getNetworkConfig(NetworkConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_NETWORK_START, config);
		EEPROM.end();
		if(config.sleep > 2) config.sleep = 1;
		return true;
	} else {
		clearNetworkConfig(config);
		return false;
	}
}

bool AmsConfiguration::setNetworkConfig(NetworkConfig& config) {
	NetworkConfig existing;
	if(config.sleep > 2) config.sleep = 1;
	if(config.mode < 1 || config.mode > 4) config.mode = 1;
	if(getNetworkConfig(existing)) {
		networkChanged |= strcmp(config.ssid, existing.ssid) != 0;
		networkChanged |= strcmp(config.psk, existing.psk) != 0;
		networkChanged |= strcmp(config.ip, existing.ip) != 0;
		if(strlen(config.ip) > 0) {
			networkChanged |= strcmp(config.gateway, existing.gateway) != 0;
			networkChanged |= strcmp(config.subnet, existing.subnet) != 0;
			networkChanged |= strcmp(config.dns1, existing.dns1) != 0;
			networkChanged |= strcmp(config.dns2, existing.dns2) != 0;
		}
		networkChanged |= strcmp(config.hostname, existing.hostname) != 0;
		networkChanged |= config.power != existing.power;
		networkChanged |= config.sleep != existing.sleep;
		networkChanged |= config.use11b != existing.use11b;
		networkChanged |= config.mode != existing.mode;
	} else {
		networkChanged = true;
	}
	
	stripNonAscii((uint8_t*) config.ssid, 32, true);
	stripNonAscii((uint8_t*) config.psk, 64, true, false);
	stripNonAscii((uint8_t*) config.ip, 16);
	stripNonAscii((uint8_t*) config.gateway, 16);
	stripNonAscii((uint8_t*) config.subnet, 16);
	stripNonAscii((uint8_t*) config.dns1, 16);
	stripNonAscii((uint8_t*) config.dns2, 16);
	stripNonAscii((uint8_t*) config.hostname, 32);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_NETWORK_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearNetworkConfig(NetworkConfig& config) {
	memset(config.ssid, 0, 32);
	memset(config.psk, 0, 64);
	clearNetworkConfigIp(config);

	uint16_t chipId;
	#if defined(ESP32)
		chipId = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
		config.power = 195;
	#else
		chipId = ESP.getChipId();
		config.power = 205;
	#endif
	strcpy(config.hostname, (String("ams-") + String(chipId, HEX)).c_str());
	config.mdns = true;
	config.sleep = 0xFF;
	config.use11b = 1;
	config.ipv6 = false;
}

void AmsConfiguration::clearNetworkConfigIp(NetworkConfig& config) {
	memset(config.ip, 0, 16);
	memset(config.gateway, 0, 16);
	memset(config.subnet, 0, 16);
	memset(config.dns1, 0, 16);
	memset(config.dns2, 0, 16);
}

bool AmsConfiguration::isNetworkConfigChanged() {
	return networkChanged;
}

void AmsConfiguration::ackNetworkConfigChange() {
	networkChanged = false;
}

bool AmsConfiguration::getMqttConfig(MqttConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_MQTT_START, config);
		EEPROM.end();
		if(config.magic != 0x9C) {
			if(config.magic != 0x7B) {
				config.stateUpdate = false;
				config.stateUpdateInterval = 10;
			}
			config.timeout = 1000;
			config.keepalive = 60;
			config.magic = 0x9C;
		}
		return true;
	} else {
		clearMqtt(config);
		return false;
	}
}

bool AmsConfiguration::setMqttConfig(MqttConfig& config) {
	MqttConfig existing;
	if(getMqttConfig(existing)) {
		mqttChanged |= strcmp(config.host, existing.host) != 0;
		mqttChanged |= config.port != existing.port;
		mqttChanged |= strcmp(config.clientId, existing.clientId) != 0;
		mqttChanged |= strcmp(config.publishTopic, existing.publishTopic) != 0;
		mqttChanged |= strcmp(config.subscribeTopic, existing.subscribeTopic) != 0;
		mqttChanged |= strcmp(config.username, existing.username) != 0;
		mqttChanged |= strcmp(config.password, existing.password) != 0;
		mqttChanged |= config.payloadFormat != existing.payloadFormat;
		mqttChanged |= config.ssl != existing.ssl;
		mqttChanged |= config.stateUpdate != existing.stateUpdate;
		mqttChanged |= config.stateUpdateInterval != existing.stateUpdateInterval;
	} else {
		mqttChanged = true;
	}

	stripNonAscii((uint8_t*) config.host, 128);
	stripNonAscii((uint8_t*) config.clientId, 32);
	stripNonAscii((uint8_t*) config.publishTopic, 64);
	stripNonAscii((uint8_t*) config.subscribeTopic, 64);
	stripNonAscii((uint8_t*) config.username, 128, true);
	stripNonAscii((uint8_t*) config.password, 256, true, false);
	if(config.timeout < 500) config.timeout = 1000;
	if(config.timeout > 10000) config.timeout = 1000;
	if(config.keepalive < 5) config.keepalive = 60;
	if(config.keepalive > 240) config.keepalive = 60;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_MQTT_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearMqtt(MqttConfig& config) {
	memset(config.host, 0, 128);
	config.port = 1883;

	memset(config.clientId, 0, 32);
	memset(config.publishTopic, 0, 64);
	memset(config.subscribeTopic, 0, 64);
	memset(config.username, 0, 128);
	memset(config.password, 0, 256);
	config.payloadFormat = 0;
	config.ssl = false;
	config.magic = 0x7B;
	config.stateUpdate = false;
	config.stateUpdateInterval = 10;
	config.timeout = 1000;
	config.keepalive = 60;
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

bool AmsConfiguration::getWebConfig(WebConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_WEB_START, config);
		EEPROM.end();
		return true;
	} else {
		clearWebConfig(config);
		return false;
	}
}

bool AmsConfiguration::setWebConfig(WebConfig& config) {
	WebConfig existing;
	if(getWebConfig(existing)) {
		webChanged |= strcmp(config.username, existing.username) != 0;
		webChanged |= strcmp(config.password, existing.password) != 0;
		webChanged |= strcmp(config.context, existing.context) != 0;
	} else {
		webChanged = true;
	}

	stripNonAscii((uint8_t*) config.username, 37);
	stripNonAscii((uint8_t*) config.password, 37, false, false);
	stripNonAscii((uint8_t*) config.context, 37);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_WEB_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearWebConfig(WebConfig& config) {
	config.security = 0;
	memset(config.username, 0, 37);
	memset(config.password, 0, 37);
	memset(config.context, 0, 37);
}

bool AmsConfiguration::isWebChanged() {
	return webChanged;
}

void AmsConfiguration::ackWebChange() {
	webChanged = false;
}

bool AmsConfiguration::getMeterConfig(MeterConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_METER_START, config);
		EEPROM.end();
		if(config.bufferSize < 1 || config.bufferSize > 64) {
			#if defined(ESP32)
				config.bufferSize = 2;
			#else
				config.bufferSize = 1;
			#endif
		}
		return true;
	} else {
		clearMeter(config);
		return false;
	}
}

bool AmsConfiguration::setMeterConfig(MeterConfig& config) {
	if(config.bufferSize < 1) config.bufferSize = 1;
	if(config.bufferSize > 64) config.bufferSize = 64;

	MeterConfig existing;
	if(getMeterConfig(existing)) {
		meterChanged |= config.baud != existing.baud;
		meterChanged |= config.parity != existing.parity;
		meterChanged |= config.invert != existing.invert;
		meterChanged |= config.distributionSystem != existing.distributionSystem;
		meterChanged |= config.mainFuse != existing.mainFuse;
		meterChanged |= config.productionCapacity != existing.productionCapacity;
		meterChanged |= strcmp((char*) config.encryptionKey, (char*) existing.encryptionKey);
		meterChanged |= strcmp((char*) config.authenticationKey, (char*) existing.authenticationKey);
		meterChanged |= config.bufferSize != existing.bufferSize;
		meterChanged |= config.rxPin != existing.rxPin;
		meterChanged |= config.rxPinPullup != existing.rxPinPullup;
		meterChanged |= config.txPin != existing.txPin;
		meterChanged |= config.wattageMultiplier != existing.wattageMultiplier;
		meterChanged |= config.voltageMultiplier != existing.voltageMultiplier;
		meterChanged |= config.amperageMultiplier != existing.amperageMultiplier;
		meterChanged |= config.accumulatedMultiplier != existing.accumulatedMultiplier;
		meterChanged |= config.source != existing.source;
		meterChanged |= config.parser != existing.parser;
	} else {
		meterChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_METER_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearMeter(MeterConfig& config) {
	config.rxPin = 0xFF;
	config.txPin = 0xFF;
	config.rxPinPullup = true;
	config.baud = 0;
	config.parity = 0;
	config.invert = false;
	config.distributionSystem = 2;
	config.mainFuse = 40;
	config.productionCapacity = 0;
	memset(config.encryptionKey, 0, 16);
	memset(config.authenticationKey, 0, 16);
	config.wattageMultiplier = 0;
	config.voltageMultiplier = 0;
	config.amperageMultiplier = 0;
	config.accumulatedMultiplier = 0;
	config.source = 1; // Serial
	config.parser = 0; // Auto
	config.bufferSize = 1; // 64 bytes
}

bool AmsConfiguration::isMeterChanged() {
	return meterChanged;
}

void AmsConfiguration::ackMeterChanged() {
	meterChanged = false;
}

void AmsConfiguration::setMeterChanged() {
	meterChanged = true;
}

bool AmsConfiguration::getDebugConfig(DebugConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_DEBUG_START, config);
		EEPROM.end();
		return true;
	} else {
		clearDebug(config);
		return false;
	}
}

bool AmsConfiguration::setDebugConfig(DebugConfig& config) {
	if(!config.serial && !config.telnet)
		config.level = 4; // Force warning level when debug is disabled
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_DEBUG_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearDebug(DebugConfig& config) {
	config.level = 5;
	config.telnet = false;
	config.serial = false;
}

bool AmsConfiguration::getDomoticzConfig(DomoticzConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_DOMOTICZ_START, config);
		EEPROM.end();
		return true;
	} else {
		clearDomo(config);
		return false;
	}
}

bool AmsConfiguration::setDomoticzConfig(DomoticzConfig& config) {
	DomoticzConfig existing;
	if(getDomoticzConfig(existing)) {
		mqttChanged |= config.elidx != existing.elidx;
		mqttChanged |= config.vl1idx != existing.vl1idx;
		mqttChanged |= config.vl2idx != existing.vl2idx;
		mqttChanged |= config.vl3idx != existing.vl3idx;
		mqttChanged |= config.cl1idx != existing.cl1idx;
	} else {
		mqttChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_DOMOTICZ_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearDomo(DomoticzConfig& config) {
	config.elidx = 0;
	config.vl1idx = 0;
	config.vl2idx = 0;
	config.vl3idx = 0;
	config.cl1idx = 0;
}

bool AmsConfiguration::getHomeAssistantConfig(HomeAssistantConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_HA_START, config);
		EEPROM.end();
		if(stripNonAscii((uint8_t*) config.discoveryPrefix, 64) || stripNonAscii((uint8_t*) config.discoveryHostname, 64) || stripNonAscii((uint8_t*) config.discoveryNameTag, 16)) {
			clearHomeAssistantConfig(config);
			return false;
		}
		return true;
	} else {
		clearHomeAssistantConfig(config);
		return false;
	}
}

bool AmsConfiguration::setHomeAssistantConfig(HomeAssistantConfig& config) {
	HomeAssistantConfig existing;
	if(getHomeAssistantConfig(existing)) {
		mqttChanged |= strcmp(config.discoveryPrefix, existing.discoveryPrefix) != 0;
		mqttChanged |= strcmp(config.discoveryHostname, existing.discoveryHostname) != 0;
		mqttChanged |= strcmp(config.discoveryNameTag, existing.discoveryNameTag) != 0;
	} else {
		mqttChanged = true;
	}

	stripNonAscii((uint8_t*) config.discoveryPrefix, 64);
	stripNonAscii((uint8_t*) config.discoveryHostname, 64);
	stripNonAscii((uint8_t*) config.discoveryNameTag, 16);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_HA_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearHomeAssistantConfig(HomeAssistantConfig& config) {
	memset(config.discoveryPrefix, 0, 64);
	memset(config.discoveryHostname, 0, 64);
	memset(config.discoveryNameTag, 0, 16);
}

bool AmsConfiguration::pinUsed(uint8_t pin, GpioConfig& config) {
	if(pin == 0xFF)
		return false;
	return 
		pin == config.apPin ||
		pin == config.ledPin ||
		pin == config.ledPinRed ||
		pin == config.ledPinGreen ||
		pin == config.ledPinBlue ||
		pin == config.tempSensorPin ||
		pin == config.tempAnalogSensorPin ||
		pin == config.vccPin ||
		pin == config.ledDisablePin
	;
}

bool AmsConfiguration::getGpioConfig(GpioConfig& config) {
	EEPROM.begin(EEPROM_SIZE);
	uint8_t configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
	if(configVersion == EEPROM_CHECK_SUM || configVersion == EEPROM_CLEARED_INDICATOR) {
		EEPROM.get(CONFIG_GPIO_START, config);
		EEPROM.end();
		return true;
	} else {
		clearGpio(config);
		return false;
	}
}

bool AmsConfiguration::setGpioConfig(GpioConfig& config) {
	GpioConfig existing;
	/* This currently does not work, as it checks its own pin
	if(pinUsed(config.hanPin, config)) {
		debugger->println(F("HAN pin already used"));
		return false;
	}
	if(pinUsed(config.apPin, config)) {
		debugger->println(F("AP pin already used"));
		return false;
	}
	if(pinUsed(config.ledPin, config)) {
		debugger->println(F("LED pin already used"));
		return false;
	}
	if(pinUsed(config.ledPinRed, config)) {
		debugger->println(F("LED RED pin already used"));
		return false;
	}
	if(pinUsed(config.ledPinGreen, config)) {
		debugger->println(F("LED GREEN pin already used"));
		return false;
	}
	if(pinUsed(config.ledPinBlue, config)) {
		debugger->println(F("LED BLUE pin already used"));
		return false;
	}
	if(pinUsed(config.tempSensorPin, config)) {
		debugger->println(F("Temp sensor pin already used"));
		return false;
	}
	if(pinUsed(config.tempAnalogSensorPin, config)) {
		debugger->println(F("Analog temp sensor pin already used"));
		return false;
	}
	if(pinUsed(config.vccPin, config)) {
		debugger->println(F("Vcc pin already used"));
		return false;
	}
	if(pinUsed(config.ledDisablePin, config)) {
		debugger->println(F("ledDisablePin already used"));
		return false;
	}
	*/
	if(config.apPin >= 0)
		pinMode(config.apPin, INPUT_PULLUP);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_GPIO_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearGpio(GpioConfig& config, bool all) {
	config.apPin = 0xFF;
	config.ledPin = 0xFF;
	config.ledInverted = true;
	config.ledPinRed = 0xFF;
	config.ledPinGreen = 0xFF;
	config.ledPinBlue = 0xFF;
	config.ledRgbInverted = true;
	config.tempSensorPin = 0xFF;
	config.tempAnalogSensorPin = 0xFF;
	config.vccPin = 0xFF;
	config.ledDisablePin = 0xFF;

	if(all) {
		config.vccOffset = 0;
		config.vccMultiplier = 1000;
		config.vccBootLimit = 0;
		config.vccResistorGnd = 0;
		config.vccResistorVcc = 0;
		config.ledBehaviour = LED_BEHAVIOUR_DEFAULT;
	}
}

bool AmsConfiguration::getNtpConfig(NtpConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_NTP_START, config);
		EEPROM.end();
		return true;
	} else {
		clearNtp(config);
		return false;
	}
}

bool AmsConfiguration::setNtpConfig(NtpConfig& config) {
	NtpConfig existing;
	if(getNtpConfig(existing)) {
		if(config.enable != existing.enable) {
			if(!existing.enable) {
				networkChanged = true;
			} else {
				ntpChanged = true;
			}
		}
		ntpChanged |= config.dhcp != existing.dhcp;
		ntpChanged |= strcmp(config.server, existing.server) != 0;
		ntpChanged |= strcmp(config.timezone, existing.timezone) != 0;
	} else {
		ntpChanged = true;
	}

	stripNonAscii((uint8_t*) config.server, 64);
	stripNonAscii((uint8_t*) config.timezone, 32);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_NTP_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::isNtpChanged() {
	return ntpChanged;
}

void AmsConfiguration::ackNtpChange() {
	ntpChanged = false;
}

void AmsConfiguration::clearNtp(NtpConfig& config) {
	config.enable = true;
	config.dhcp = true;
	strcpy_P(config.server, PSTR("pool.ntp.org"));
	strcpy_P(config.timezone, PSTR("Europe/Oslo"));
}

bool AmsConfiguration::getPriceServiceConfig(PriceServiceConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_PRICE_START, config);
		EEPROM.end();
		if(strlen(config.entsoeToken) != 0 && strlen(config.entsoeToken) != 36) {
			clearPriceServiceConfig(config);
			return false;
		}
		return true;
	} else {
		clearPriceServiceConfig(config);
		return false;
	}
}

bool AmsConfiguration::setPriceServiceConfig(PriceServiceConfig& config) {
	PriceServiceConfig existing;
	if(getPriceServiceConfig(existing)) {
		priceChanged |= strcmp(config.entsoeToken, existing.entsoeToken) != 0;
		priceChanged |= strcmp(config.area, existing.area) != 0;
		priceChanged |= strcmp(config.currency, existing.currency) != 0;
		priceChanged |= config.enabled != existing.enabled;
	} else {
		priceChanged = true;
	}

	stripNonAscii((uint8_t*) config.entsoeToken, 37);
	stripNonAscii((uint8_t*) config.area, 17);
	stripNonAscii((uint8_t*) config.currency, 4);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_PRICE_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearPriceServiceConfig(PriceServiceConfig& config) {
	memset(config.entsoeToken, 0, 37);
	memset(config.area, 0, 17);
	memset(config.currency, 0, 4);
	config.unused1 = 1000;
	config.enabled = false;
	config.unused2 = 0;
}

bool AmsConfiguration::isPriceServiceChanged() {
	return priceChanged;
}

void AmsConfiguration::ackPriceServiceChange() {
	priceChanged = false;
}


bool AmsConfiguration::getEnergyAccountingConfig(EnergyAccountingConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_ENERGYACCOUNTING_START, config);
		EEPROM.end();
		if(config.thresholds[9] != 0xFFFF) {
			clearEnergyAccountingConfig(config);
			return false;
		}
		if(config.hours > 5) config.hours = 5;
		return true;
	} else {
		clearEnergyAccountingConfig(config);
		return false;
	}
}

bool AmsConfiguration::setEnergyAccountingConfig(EnergyAccountingConfig& config) {
	if(config.hours > 5) config.hours = 5;
	EnergyAccountingConfig existing;
	if(getEnergyAccountingConfig(existing)) {
		for(int i = 0; i < 9; i++) {
			if(existing.thresholds[i] != config.thresholds[i]) {
				energyAccountingChanged = true;
			}
		}
		config.thresholds[9] = 0xFFFF;
		energyAccountingChanged |= config.hours != existing.hours;
	} else {
		energyAccountingChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_ENERGYACCOUNTING_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearEnergyAccountingConfig(EnergyAccountingConfig& config) {
	config.thresholds[0] = 5;
	config.thresholds[1] = 10;
	config.thresholds[2] = 15;
	config.thresholds[3] = 20;
	config.thresholds[4] = 25;
	config.thresholds[5] = 50;
	config.thresholds[6] = 75;
	config.thresholds[7] = 100;
	config.thresholds[8] = 150;
	config.thresholds[9] = 0xFFFF;
	config.hours = 3;
}

bool AmsConfiguration::isEnergyAccountingChanged() {
	return energyAccountingChanged;
}

void AmsConfiguration::ackEnergyAccountingChange() {
	energyAccountingChanged = false;
}

bool AmsConfiguration::getUiConfig(UiConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_UI_START, config);
		if(config.showImport > 2) clearUiConfig(config); // Must be wrong
		EEPROM.end();
		return true;
	} else {
		clearUiConfig(config);
		return false;
	}
}

bool AmsConfiguration::setUiConfig(UiConfig& config) {
	UiConfig existing;
	if(getUiConfig(existing)) {
		uiLanguageChanged |= strcmp(config.language, existing.language) != 0;
	} else {
		uiLanguageChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_UI_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearUiConfig(UiConfig& config) {
	// 1 = Enable, 2 = Auto, 0 = Disable
	config.showImport = 1;
	config.showExport = 2;
	config.showVoltage = 2;
	config.showAmperage = 2;
	config.showReactive = 0;
	config.showRealtime = 1;
	config.showPeaks = 2;
	config.showPricePlot = 2;
	config.showDayPlot = 1;
	config.showMonthPlot = 1;
	config.showTemperaturePlot = 2;
	config.showRealtimePlot = 2;
	config.showPerPhasePower = 2;
	config.showPowerFactor = 2;
	config.darkMode = 2;
	memset(config.language, 0, 3);
}

bool AmsConfiguration::isUiLanguageChanged() {
	return uiLanguageChanged;
}

void AmsConfiguration::ackUiLanguageChange() {
	uiLanguageChanged = false;
}

bool AmsConfiguration::setUpgradeInformation(UpgradeInformation& upinfo) {
	stripNonAscii((uint8_t*) upinfo.fromVersion, 8);
	stripNonAscii((uint8_t*) upinfo.toVersion, 8);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_UPGRADE_INFO_START, upinfo);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::getUpgradeInformation(UpgradeInformation& upinfo) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_UPGRADE_INFO_START, upinfo);
		EEPROM.end();
		if(stripNonAscii((uint8_t*) upinfo.fromVersion, 8) || stripNonAscii((uint8_t*) upinfo.toVersion, 8)) {
			clearUpgradeInformation(upinfo);
			return false;
		}
		return true;
	} else {
		clearUpgradeInformation(upinfo);
		return false;
	}
}

void AmsConfiguration::clearUpgradeInformation(UpgradeInformation& upinfo) {
	memset(upinfo.fromVersion, 0, 8);
	memset(upinfo.toVersion, 0, 8);
	upinfo.errorCode = 0;
	upinfo.size = 0;
	upinfo.block_position = 0;
	upinfo.retry_count = 0;
	upinfo.reboot_count = 0;
}

bool AmsConfiguration::getCloudConfig(CloudConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_CLOUD_START, config);
		EEPROM.end();
		if(config.proto > 2) config.proto = 0;
		return true;
	} else {
		clearCloudConfig(config);
		return false;
	}
}

bool AmsConfiguration::setCloudConfig(CloudConfig& config) {
	CloudConfig existing;
	if(getCloudConfig(existing)) {
		cloudChanged |= config.enabled != existing.enabled;
		cloudChanged |= config.interval!= existing.interval;
		cloudChanged |= config.port!= existing.port;
		cloudChanged |= config.proto!= existing.proto;
		cloudChanged |= strcmp(config.hostname, existing.hostname) != 0;
		cloudChanged |= memcmp(config.clientId, existing.clientId, 16) != 0;
	} else {
		cloudChanged = true;
	}

	stripNonAscii((uint8_t*) config.hostname, 64);

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_CLOUD_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearCloudConfig(CloudConfig& config) {
	config.enabled = false;
	strcpy_P(config.hostname, PSTR("cloud.amsleser.no"));
	config.proto = 1;
	config.port = 7443;
	config.interval = 10;
	memset(config.clientId, 0, 16);
}

bool AmsConfiguration::isCloudChanged() {
	return cloudChanged;
}

void AmsConfiguration::ackCloudConfig() {
	cloudChanged = false;
}

void AmsConfiguration::setUiLanguageChanged() {
	uiLanguageChanged = true;
}

void AmsConfiguration::clear() {
	EEPROM.begin(EEPROM_SIZE);

	SystemConfig sys;
	EEPROM.get(CONFIG_SYSTEM_START, sys);
	sys.userConfigured = false;
	sys.dataCollectionConsent = 0;
	sys.energyspeedometer = 0;
	memset(sys.country, 0, 3);
	EEPROM.put(CONFIG_SYSTEM_START, sys);

	MeterConfig meter;
	clearMeter(meter);
	EEPROM.put(CONFIG_METER_START, meter);

	NetworkConfig network;
	clearNetworkConfig(network);
	EEPROM.put(CONFIG_NETWORK_START, network);

	MqttConfig mqtt;
	clearMqtt(mqtt);
	EEPROM.put(CONFIG_MQTT_START, mqtt);

	WebConfig web;
	clearWebConfig(web);
	EEPROM.put(CONFIG_WEB_START, web);

	DomoticzConfig domo;
	clearDomo(domo);
	EEPROM.put(CONFIG_DOMOTICZ_START, domo);

	HomeAssistantConfig haconf;
	clearHomeAssistantConfig(haconf);
	EEPROM.put(CONFIG_HA_START, haconf);

	NtpConfig ntp;
	clearNtp(ntp);
	EEPROM.put(CONFIG_NTP_START, ntp);

	PriceServiceConfig price;
	clearPriceServiceConfig(price);
	EEPROM.put(CONFIG_PRICE_START, price);

	EnergyAccountingConfig eac;
	clearEnergyAccountingConfig(eac);
	EEPROM.put(CONFIG_ENERGYACCOUNTING_START, eac);

	DebugConfig debug;
	clearDebug(debug);
	EEPROM.put(CONFIG_DEBUG_START, debug);

	UiConfig ui;
	clearUiConfig(ui);
	EEPROM.put(CONFIG_UI_START, ui);

	UpgradeInformation upinfo;
	clearUpgradeInformation(upinfo);
	EEPROM.put(CONFIG_UPGRADE_INFO_START, upinfo);

	CloudConfig cloud;
	clearCloudConfig(cloud);
	EEPROM.put(CONFIG_CLOUD_START, cloud);

	EEPROM.put(EEPROM_CONFIG_ADDRESS, EEPROM_CLEARED_INDICATOR);
	EEPROM.commit();
	EEPROM.end();
}

bool AmsConfiguration::hasConfig() {
	if(configVersion == 0) {
		EEPROM.begin(EEPROM_SIZE);
		configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
		EEPROM.end();
	}
	if(configVersion > EEPROM_CHECK_SUM) {
		if(loadFromFs(EEPROM_CHECK_SUM)) {
			configVersion = EEPROM_CHECK_SUM;
		} else {
			configVersion = 0;
		}
	} else {
		switch(configVersion) {
			case 103:
				configVersion = -1; // Prevent loop
				if(relocateConfig103()) {
					configVersion = 104;
				} else {
					configVersion = 0;
					return false;
				}
			case EEPROM_CHECK_SUM:
				return true;
			default:
				configVersion = 0;
				return false;
		}
	}
	return configVersion == EEPROM_CHECK_SUM;
}

int AmsConfiguration::getConfigVersion() {
	return configVersion;
}

bool AmsConfiguration::relocateConfig103() {
	EEPROM.begin(EEPROM_SIZE);

	MeterConfig meter;
	UpgradeInformation upinfo;
	UiConfig ui;
	GpioConfig103 gpio103;
	PriceServiceConfig price;
	NetworkConfig wifi;
	EnergyAccountingConfig eac;
	WebConfig103 web103;
	DebugConfig debug;
	DomoticzConfig domo;
	NtpConfig ntp;
	MqttConfig mqtt;
	HomeAssistantConfig ha;

	EEPROM.get(CONFIG_METER_START_103, meter);
	EEPROM.get(CONFIG_UPGRADE_INFO_START_103, upinfo);
	EEPROM.get(CONFIG_UI_START_103, ui);
	EEPROM.get(CONFIG_GPIO_START_103, gpio103);
	EEPROM.get(CONFIG_ENTSOE_START_103, price);
	EEPROM.get(CONFIG_WIFI_START_103, wifi);
	EEPROM.get(CONFIG_ENERGYACCOUNTING_START_103, eac);
	EEPROM.get(CONFIG_WEB_START_103, web103);
	EEPROM.get(CONFIG_DEBUG_START_103, debug);
	EEPROM.get(CONFIG_DOMOTICZ_START_103, domo);
	EEPROM.get(CONFIG_NTP_START_103, ntp);
	EEPROM.get(CONFIG_MQTT_START_103, mqtt);
	EEPROM.get(CONFIG_HA_START_103, ha);

	meter.rxPin = gpio103.hanPin;
	meter.txPin = 0xFF;
	meter.rxPinPullup = gpio103.hanPinPullup;
	meter.source = 1;
	meter.parser = 0;
	#if defined(ESP8266)
		if(meter.rxPin != 3 && meter.rxPin != 113) {
			meter.bufferSize = 1;
		}
	#endif
	wifi.mode = 1; // 1 == WiFi client
	wifi.ipv6 = false;

	GpioConfig gpio = {
		gpio103.apPin,
		gpio103.ledPin,
		gpio103.ledInverted,
		gpio103.ledPinRed,
		gpio103.ledPinGreen,
		gpio103.ledPinBlue,
		gpio103.ledRgbInverted,
		gpio103.tempSensorPin,
		gpio103.tempAnalogSensorPin,
		gpio103.vccPin,
		gpio103.vccOffset,
		gpio103.vccMultiplier,
		gpio103.vccBootLimit,
		gpio103.vccResistorGnd,
		gpio103.vccResistorVcc,
		gpio103.ledDisablePin,
		gpio103.ledBehaviour
	};

	WebConfig web = {web103.security};
	strcpy(web.username, web103.username);
	strcpy(web.password, web103.password);
	memset(web.context, 0, 37);

	strcpy_P(ui.language, PSTR("en"));
	ui.showRealtimePlot = 2;
	ui.showPerPhasePower = 2;
	ui.showPowerFactor = 2;
	ui.darkMode = 2;

	EEPROM.put(CONFIG_UPGRADE_INFO_START, upinfo);
	EEPROM.put(CONFIG_NETWORK_START, wifi);
	EEPROM.put(CONFIG_METER_START, meter);
	EEPROM.put(CONFIG_GPIO_START, gpio);
	EEPROM.put(CONFIG_PRICE_START, price);
	EEPROM.put(CONFIG_ENERGYACCOUNTING_START, eac);
	EEPROM.put(CONFIG_WEB_START, web);
	EEPROM.put(CONFIG_DEBUG_START, debug);
	EEPROM.put(CONFIG_NTP_START, ntp);
	EEPROM.put(CONFIG_MQTT_START, mqtt);
	EEPROM.put(CONFIG_DOMOTICZ_START, domo);
	EEPROM.put(CONFIG_HA_START, ha);
	EEPROM.put(CONFIG_UI_START, ui);

	CloudConfig cloud;
	clearCloudConfig(cloud);
	EEPROM.put(CONFIG_CLOUD_START, cloud);

	EEPROM.put(EEPROM_CONFIG_ADDRESS, 104);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::save() {
	EEPROM.begin(EEPROM_SIZE);
	uint8_t configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, EEPROM_CHECK_SUM);
	bool success = EEPROM.commit();
	EEPROM.end();

	configVersion = EEPROM_CHECK_SUM;
	return success;
}

void AmsConfiguration::saveToFs() {
	
}

bool AmsConfiguration::loadFromFs(uint8_t version) {
	return false;
}

void AmsConfiguration::deleteFromFs(uint8_t version) {

}

void AmsConfiguration::print(Print* debugger)
{
	debugger->println(F("-----------------------------------------------"));
	NetworkConfig network;
	if(getNetworkConfig(network)) {
		debugger->println(F("--Network configuration--"));
		switch(network.mode) {
			case 1:
				debugger->printf_P(PSTR("Mode:                 'WiFi client'\r\n"));
				break;
			case 2:
				debugger->printf_P(PSTR("Mode:                 'WiFi AP'\r\n"));
				break;
			case 3:
				debugger->printf_P(PSTR("Mode:                 'Ethernet'\r\n"));
				break;
		}
		debugger->printf_P(PSTR("SSID:                 '%s'\r\n"), network.ssid);
		debugger->printf_P(PSTR("Psk:                  '%s'\r\n"), network.psk);
		if(strlen(network.ip) > 0) {
			debugger->printf_P(PSTR("IP:                   '%s'\r\n"), network.ip);
			debugger->printf_P(PSTR("Gateway:              '%s'\r\n"), network.gateway);
			debugger->printf_P(PSTR("Subnet:               '%s'\r\n"), network.subnet);
			debugger->printf_P(PSTR("DNS1:                 '%s'\r\n"), network.dns1);
			debugger->printf_P(PSTR("DNS2:                 '%s'\r\n"), network.dns2);
		}
		debugger->printf_P(PSTR("Hostname:             '%s'\r\n"), network.hostname);
		debugger->printf_P(PSTR("IPv6:                 '%s'\r\n"), network.ipv6 ? "Yes" : "No");
		debugger->printf_P(PSTR("mDNS:                 '%s'\r\n"), network.mdns ? "Yes" : "No");
		debugger->printf_P(PSTR("802.11b:              '%s'\r\n"), network.use11b ? "Yes" : "No");
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	MqttConfig mqtt;
	if(getMqttConfig(mqtt)) {
		debugger->println(F("--MQTT configuration--"));
		if(strlen(mqtt.host) > 0) {
			debugger->printf_P(PSTR("Enabled:              Yes\r\n"));
			debugger->printf_P(PSTR("Host:                 '%s'\r\n"), mqtt.host);
			debugger->printf_P(PSTR("Port:                 %i\r\n"), mqtt.port);
			debugger->printf_P(PSTR("Client ID:            '%s'\r\n"), mqtt.clientId);
			debugger->printf_P(PSTR("Publish topic:        '%s'\r\n"), mqtt.publishTopic);
			debugger->printf_P(PSTR("Subscribe topic:      '%s'\r\n"), mqtt.subscribeTopic);
			if (strlen(mqtt.username) > 0) {
				debugger->printf_P(PSTR("Username:             '%s'\r\n"), mqtt.username);
				debugger->printf_P(PSTR("Password:             '%s'\r\n"), mqtt.password);
			}
			debugger->printf_P(PSTR("Payload format:       %i\r\n"), mqtt.payloadFormat);
			debugger->printf_P(PSTR("SSL:                  %s\r\n"), mqtt.ssl ? "Yes" : "No");
		} else {
			debugger->printf_P(PSTR("Enabled:              No\r\n"));
		}
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	WebConfig web;
	if(getWebConfig(web)) {
		debugger->println(F("--Web configuration--"));
		debugger->printf_P(PSTR("Security:             %i\r\n"), web.security);
		if (web.security > 0) {
			debugger->printf_P(PSTR("Username:             '%s'\r\n"), web.username);
			debugger->printf_P(PSTR("Password:             '%s'\r\n"), web.password);
		}
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	MeterConfig meter;
	if(getMeterConfig(meter)) {
		debugger->println(F("--Meter configuration--"));
		debugger->printf_P(PSTR("HAN RX:               %i\r\n"), meter.rxPin);
		debugger->printf_P(PSTR("HAN RX pullup         %s\r\n"), meter.rxPinPullup ? "Yes" : "No");
		debugger->printf_P(PSTR("Baud:                 %i\r\n"), meter.baud);
		debugger->printf_P(PSTR("Parity:               %i\r\n"), meter.parity);
		debugger->printf_P(PSTR("Invert serial:        %s\r\n"), meter.invert ? "Yes" : "No");
		debugger->printf_P(PSTR("Buffer size:          %i\r\n"), meter.bufferSize * 64);
		debugger->printf_P(PSTR("Distribution system:  %i\r\n"), meter.distributionSystem);
		debugger->printf_P(PSTR("Main fuse:            %i\r\n"), meter.mainFuse);
		debugger->printf_P(PSTR("Production Capacity:  %i\r\n"), meter.productionCapacity);
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	GpioConfig gpio;
	if(getGpioConfig(gpio)) {
		debugger->println(F("--GPIO configuration--"));
		debugger->printf_P(PSTR("LED pin:              %i\r\n"), gpio.ledPin);
		debugger->printf_P(PSTR("LED inverted:         %s\r\n"), gpio.ledInverted ? "Yes" : "No");
		debugger->printf_P(PSTR("LED red pin:          %i\r\n"), gpio.ledPinRed);
		debugger->printf_P(PSTR("LED green pin:        %i\r\n"), gpio.ledPinGreen);
		debugger->printf_P(PSTR("LED blue pin:         %i\r\n"), gpio.ledPinBlue);
		debugger->printf_P(PSTR("LED inverted:         %s\r\n"), gpio.ledRgbInverted ? "Yes" : "No");
		debugger->printf_P(PSTR("AP pin:               %i\r\n"), gpio.apPin);
		debugger->printf_P(PSTR("Temperature pin:      %i\r\n"), gpio.tempSensorPin);
		debugger->printf_P(PSTR("Temp analog pin:      %i\r\n"), gpio.tempAnalogSensorPin);
		debugger->printf_P(PSTR("Vcc pin:              %i\r\n"), gpio.vccPin);
		debugger->printf_P(PSTR("LED disable pin:      %i\r\n"), gpio.ledDisablePin);
		debugger->printf_P(PSTR("LED behaviour:        %i\r\n"), gpio.ledBehaviour);
		if(gpio.vccMultiplier > 0) {
			debugger->printf_P(PSTR("Vcc multiplier:       %f\r\n"), gpio.vccMultiplier / 1000.0);
		}
		if(gpio.vccOffset > 0) {
			debugger->printf_P(PSTR("Vcc offset:           %f\r\n"), gpio.vccOffset / 100.0);
		}
		if(gpio.vccBootLimit > 0) {
			debugger->printf_P(PSTR("Vcc boot limit:       %f\r\n"), gpio.vccBootLimit / 10.0);
		}
		debugger->printf_P(PSTR("GND resistor:         %i\r\n"), gpio.vccResistorGnd);
		debugger->printf_P(PSTR("Vcc resistor:         %i\r\n"), gpio.vccResistorVcc);
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	DomoticzConfig domo;
	if(getDomoticzConfig(domo)) {
		debugger->println(F("--Domoticz configuration--"));
		if(mqtt.payloadFormat == 3 && domo.elidx > 0) {
			debugger->printf_P(PSTR("Enabled:              Yes\r\n"));
			debugger->printf_P(PSTR("Domoticz ELIDX:       %i\r\n"), domo.elidx);
			debugger->printf_P(PSTR("Domoticz VL1IDX:      %i\r\n"), domo.vl1idx);
			debugger->printf_P(PSTR("Domoticz VL2IDX:      %i\r\n"), domo.vl2idx);
			debugger->printf_P(PSTR("Domoticz VL3IDX:      %i\r\n"), domo.vl3idx);
			debugger->printf_P(PSTR("Domoticz CL1IDX:      %i\r\n"), domo.cl1idx);
		} else {
			debugger->printf_P(PSTR("Enabled:              No\r\n"));
		}
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	NtpConfig ntp;
	if(getNtpConfig(ntp)) {
		debugger->println(F("--NTP configuration--"));
		debugger->printf_P(PSTR("Enabled:              %s\r\n"), ntp.enable ? "Yes" : "No");
		if(ntp.enable) {
			debugger->printf_P(PSTR("Timezone:             %s\r\n"), ntp.timezone);
			debugger->printf_P(PSTR("Server:               %s\r\n"), ntp.server);
			debugger->printf_P(PSTR("DHCP:                 %s\r\n"), ntp.dhcp ? "Yes" : "No");
		}
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	PriceServiceConfig price;
	if(getPriceServiceConfig(price)) {
		if(strlen(price.area) > 0) {
			debugger->println(F("--Price configuration--"));
			debugger->printf_P(PSTR("Area:                 %s\r\n"), price.area);
			debugger->printf_P(PSTR("Currency:             %s\r\n"), price.currency);
			debugger->printf_P(PSTR("ENTSO-E Token:        %s\r\n"), price.entsoeToken);
		}
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	UiConfig ui;
	if(getUiConfig(ui)) {
		debugger->println(F("--UI configuration--"));
		debugger->printf_P(PSTR("Language:             %s\r\n"), ui.language);
		debugger->println(F(""));
		delay(10);
		debugger->flush();
	}

	#if defined(ESP32)
	CloudConfig cc;
	if(getCloudConfig(cc)) {
		String uuid = ESPRandom::uuidToString(cc.clientId);;
		debugger->println(F("--Cloud configuration--"));
		debugger->printf_P(PSTR("Enabled:              %s\r\n"), cc.enabled ? "Yes" : "No");
		debugger->printf_P(PSTR("Hostname:             %s\r\n"), cc.hostname);
		debugger->printf_P(PSTR("Client ID:            %s\r\n"), uuid.c_str());
		debugger->printf_P(PSTR("Interval:             %d\r\n"), cc.interval);
	}
	#endif

	debugger->println(F("-----------------------------------------------"));
	debugger->flush();
}
