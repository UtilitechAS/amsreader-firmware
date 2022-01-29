#include "AmsConfiguration.h"

bool AmsConfiguration::getSystemConfig(SystemConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_SYSTEM_START, config);
		EEPROM.end();
		return true;
	} else {
		return false;
	}
}

bool AmsConfiguration::setSystemConfig(SystemConfig& config) {
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_SYSTEM_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::getWiFiConfig(WiFiConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_WIFI_START, config);
		EEPROM.end();
		return true;
	} else {
		clearWifi(config);
		return false;
	}
}

bool AmsConfiguration::setWiFiConfig(WiFiConfig& config) {
	WiFiConfig existing;
	if(getWiFiConfig(existing)) {
		wifiChanged |= strcmp(config.ssid, existing.ssid) != 0;
		wifiChanged |= strcmp(config.psk, existing.psk) != 0;
		wifiChanged |= strcmp(config.ip, existing.ip) != 0;
		if(strlen(config.ip) > 0) {
			wifiChanged |= strcmp(config.gateway, existing.gateway) != 0;
			wifiChanged |= strcmp(config.subnet, existing.subnet) != 0;
			wifiChanged |= strcmp(config.dns1, existing.dns1) != 0;
			wifiChanged |= strcmp(config.dns2, existing.dns2) != 0;
		}
		wifiChanged |= strcmp(config.hostname, existing.hostname) != 0;
		wifiChanged |= config.power != existing.power;
	} else {
		wifiChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_WIFI_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearWifi(WiFiConfig& config) {
	strcpy(config.ssid, "");
	strcpy(config.psk, "");
	clearWifiIp(config);

	uint16_t chipId;
	#if defined(ESP32)
		chipId = ESP.getEfuseMac();
	#else
		chipId = ESP.getChipId();
	#endif
	strcpy(config.hostname, (String("ams-") + String(chipId, HEX)).c_str());
	config.mdns = true;
}

void AmsConfiguration::clearWifiIp(WiFiConfig& config) {
	strcpy(config.ip, "");
	strcpy(config.gateway, "");
	strcpy(config.subnet, "");
	strcpy(config.dns1, "");
	strcpy(config.dns2, "");
}

bool AmsConfiguration::isWifiChanged() {
	return wifiChanged;
}

void AmsConfiguration::ackWifiChange() {
	wifiChanged = false;
}

bool AmsConfiguration::getMqttConfig(MqttConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_MQTT_START, config);
		EEPROM.end();
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
	} else {
		mqttChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_MQTT_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearMqtt(MqttConfig& config) {
	strcpy(config.host, "");
	config.port = 1883;
	strcpy(config.clientId, "");
	strcpy(config.publishTopic, "");
	strcpy(config.subscribeTopic, "");
	strcpy(config.username, "");
	strcpy(config.password, "");
	config.payloadFormat = 0;
	config.ssl = false;
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
		clearAuth(config);
		return false;
	}
}

bool AmsConfiguration::setWebConfig(WebConfig& config) {
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_WEB_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearAuth(WebConfig& config) {
	config.security = 0;
	strcpy(config.username, "");
	strcpy(config.password, "");
}

bool AmsConfiguration::getMeterConfig(MeterConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_METER_START, config);
		EEPROM.end();
		return true;
	} else {
		clearMeter(config);
		return false;
	}
}

bool AmsConfiguration::setMeterConfig(MeterConfig& config) {
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
	config.baud = 2400;
	config.parity = 11; // 8E1
	config.invert = false;
	config.distributionSystem = 0;
	config.mainFuse = 0;
	config.productionCapacity = 0;
	memset(config.encryptionKey, 0, 16);
	memset(config.authenticationKey, 0, 16);
}

bool AmsConfiguration::isMeterChanged() {
	return meterChanged;
}

void AmsConfiguration::ackMeterChanged() {
	meterChanged = false;
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
		config.level = 5; // Force error level when debug is disabled
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
		domoChanged |= config.elidx != existing.elidx;
		domoChanged |= config.vl1idx != existing.vl1idx;
		domoChanged |= config.vl2idx != existing.vl2idx;
		domoChanged |= config.vl3idx != existing.vl3idx;
		domoChanged |= config.cl1idx != existing.cl1idx;
	} else {
		domoChanged = true;
	}
	mqttChanged = domoChanged;
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

bool AmsConfiguration::isDomoChanged() {
	return domoChanged;
}

void AmsConfiguration::ackDomoChange() {
	domoChanged = false;
}

bool AmsConfiguration::pinUsed(uint8_t pin, GpioConfig& config) {
	if(pin == 0xFF)
		return false;
	return 
		pin == config.hanPin ||
		pin == config.apPin ||
		pin == config.ledPin ||
		pin == config.ledPinRed ||
		pin == config.ledPinGreen ||
		pin == config.ledPinBlue ||
		pin == config.tempSensorPin ||
		pin == config.tempAnalogSensorPin ||
		pin == config.vccPin
	;
}

bool AmsConfiguration::getGpioConfig(GpioConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
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
	if(getGpioConfig(existing)) {
		meterChanged |= config.hanPin != existing.hanPin;
	}
	/* This currently does not work, as it checks its own pin
	if(pinUsed(config.hanPin, config)) {
		Serial.println("HAN pin already used");
		return false;
	}
	if(pinUsed(config.apPin, config)) {
		Serial.println("AP pin already used");
		return false;
	}
	if(pinUsed(config.ledPin, config)) {
		Serial.println("LED pin already used");
		return false;
	}
	if(pinUsed(config.ledPinRed, config)) {
		Serial.println("LED RED pin already used");
		return false;
	}
	if(pinUsed(config.ledPinGreen, config)) {
		Serial.println("LED GREEN pin already used");
		return false;
	}
	if(pinUsed(config.ledPinBlue, config)) {
		Serial.println("LED BLUE pin already used");
		return false;
	}
	if(pinUsed(config.tempSensorPin, config)) {
		Serial.println("Temp sensor pin already used");
		return false;
	}
	if(pinUsed(config.tempAnalogSensorPin, config)) {
		Serial.println("Analog temp sensor pin already used");
		return false;
	}
	if(pinUsed(config.vccPin, config)) {
		Serial.println("Vcc pin already used");
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

void AmsConfiguration::clearGpio(GpioConfig& config) {
	config.hanPin = 3;
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
	config.vccOffset = 0;
	config.vccMultiplier = 1000;
	config.vccBootLimit = 0;
	config.vccResistorGnd = 0;
	config.vccResistorVcc = 0;
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
				wifiChanged = true;
			} else {
				ntpChanged = true;
			}
		}
		ntpChanged |= config.dhcp != existing.dhcp;
		ntpChanged |= config.offset != existing.offset;
		ntpChanged |= config.summerOffset != existing.summerOffset;
		ntpChanged |= strcmp(config.server, existing.server) != 0;
	} else {
		ntpChanged = true;
	}
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
	config.offset = 360;
	config.summerOffset = 360;
	strcpy(config.server, "pool.ntp.org");
}

bool AmsConfiguration::getEntsoeConfig(EntsoeConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_ENTSOE_START, config);
		EEPROM.end();
		if(strlen(config.token) != 0 && strlen(config.token) != 36) {
			clearEntsoe(config);
		}
		return true;
	} else {
		return false;
	}
}

bool AmsConfiguration::setEntsoeConfig(EntsoeConfig& config) {
	EntsoeConfig existing;
	if(getEntsoeConfig(existing)) {
		entsoeChanged |= strcmp(config.token, existing.token) != 0;
		entsoeChanged |= strcmp(config.area, existing.area) != 0;
		entsoeChanged |= strcmp(config.currency, existing.currency) != 0;
		entsoeChanged |= config.multiplier != existing.multiplier;
	} else {
		entsoeChanged = true;
	}
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(CONFIG_ENTSOE_START, config);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

void AmsConfiguration::clearEntsoe(EntsoeConfig& config) {
	strcpy(config.token, "");
	strcpy(config.area, "");
	strcpy(config.currency, "");
	config.multiplier = 1000;
}

bool AmsConfiguration::isEntsoeChanged() {
	return entsoeChanged;
}

void AmsConfiguration::ackEntsoeChange() {
	entsoeChanged = false;
}


bool AmsConfiguration::getEnergyAccountingConfig(EnergyAccountingConfig& config) {
	if(hasConfig()) {
		EEPROM.begin(EEPROM_SIZE);
		EEPROM.get(CONFIG_ENERGYACCOUNTING_START, config);
		EEPROM.end();
		if(config.thresholds[9] != 255) {
			clearEnergyAccountingConfig(config);
		}
		return true;
	} else {
		return false;
	}
}

bool AmsConfiguration::setEnergyAccountingConfig(EnergyAccountingConfig& config) {
	EnergyAccountingConfig existing;
	if(getEnergyAccountingConfig(existing)) {
		for(int i = 0; i < 9; i++) {
			if(existing.thresholds[i] != config.thresholds[i]) {
				energyAccountingChanged = true;
			}
		}
		config.thresholds[9] = 255;
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
	config.thresholds[9] = 255;
}

bool AmsConfiguration::isEnergyAccountingChanged() {
	return energyAccountingChanged;
}

void AmsConfiguration::ackEnergyAccountingChange() {
	energyAccountingChanged = false;
}


void AmsConfiguration::clear() {
	EEPROM.begin(EEPROM_SIZE);
	MeterConfig meter;
	clearMeter(meter);
	EEPROM.put(CONFIG_METER_START, meter);

	WiFiConfig wifi;
	clearWifi(wifi);
	EEPROM.put(CONFIG_WIFI_START, wifi);

	MqttConfig mqtt;
	clearMqtt(mqtt);
	EEPROM.put(CONFIG_MQTT_START, mqtt);

	WebConfig web;
	clearAuth(web);
	EEPROM.put(CONFIG_WEB_START, web);

	DomoticzConfig domo;
	clearDomo(domo);
	EEPROM.put(CONFIG_DOMOTICZ_START, domo);

	NtpConfig ntp;
	clearNtp(ntp);
	EEPROM.put(CONFIG_NTP_START, ntp);

	EntsoeConfig entsoe;
	clearEntsoe(entsoe);
	EEPROM.put(CONFIG_ENTSOE_START, entsoe);

	EnergyAccountingConfig eac;
	clearEnergyAccountingConfig(eac);
	EEPROM.put(CONFIG_ENERGYACCOUNTING_START, eac);

	EEPROM.put(EEPROM_CONFIG_ADDRESS, -1);
	EEPROM.commit();
	EEPROM.end();
}

bool AmsConfiguration::hasConfig() {
	if(configVersion == 0) {
		EEPROM.begin(EEPROM_SIZE);
		configVersion = EEPROM.read(EEPROM_CONFIG_ADDRESS);
		EEPROM.end();
	}
	switch(configVersion) {
		case 86:
			configVersion = -1; // Prevent loop
			if(relocateConfig86()) {
				configVersion = 87;
			} else {
				configVersion = 0;
				return false;
			}
		case 87:
			configVersion = -1; // Prevent loop
			if(relocateConfig87()) {
				configVersion = 88;
			} else {
				configVersion = 0;
				return false;
			}
		case 90:
			configVersion = -1; // Prevent loop
			if(relocateConfig90()) {
				configVersion = 91;
			} else {
				configVersion = 0;
				return false;
			}
		case 91:
			configVersion = -1; // Prevent loop
			if(relocateConfig91()) {
				configVersion = 92;
			} else {
				configVersion = 0;
				return false;
			}
		case 92:
			configVersion = -1; // Prevent loop
			if(relocateConfig92()) {
				configVersion = 93;
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
	return configVersion == EEPROM_CHECK_SUM;
}

int AmsConfiguration::getConfigVersion() {
	return configVersion;
}

void AmsConfiguration::loadTempSensors() {
	EEPROM.begin(EEPROM_SIZE);
	TempSensorConfig* tempSensors[32];
	int address = EEPROM_TEMP_CONFIG_ADDRESS;
	int c = 0;
	int storedCount = EEPROM.read(address++);
	Serial.print("Sensors: ");
	Serial.println(storedCount);
	if(storedCount > 0 && storedCount <= 32) {
		for(int i = 0; i < storedCount; i++) {
			TempSensorConfig* tsc = new TempSensorConfig();
			EEPROM.get(address, *tsc);
			if(tsc->address[0] != 0xFF) {
				tempSensors[c++] = tsc;
			}
			address += sizeof(*tsc);
		}
	}
	this->tempSensors = new TempSensorConfig*[c];
	for(int i = 0; i < c; i++) {
		this->tempSensors[i] = tempSensors[i];
	}
	tempSensorCount = c;
	EEPROM.end();
}

void AmsConfiguration::saveTempSensors() {
	int address = EEPROM_TEMP_CONFIG_ADDRESS;
	EEPROM.put(address++, tempSensorCount);
	for(int i = 0; i < tempSensorCount; i++) {
		TempSensorConfig* tsc = tempSensors[i];
		if(tsc->address[0] != 0xFF) {
			EEPROM.put(address, *tsc);
			address += sizeof(*tsc);
		}
	}
}

bool AmsConfiguration::relocateConfig86() {
	MqttConfig86 mqtt86;
	MqttConfig mqtt;
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.get(CONFIG_MQTT_START_86, mqtt86);
	strcpy(mqtt.host, mqtt86.host);
	mqtt.port = mqtt86.port;
	strcpy(mqtt.clientId, mqtt86.clientId);
	strcpy(mqtt.publishTopic, mqtt86.publishTopic);
	strcpy(mqtt.subscribeTopic, mqtt86.subscribeTopic);
	strcpy(mqtt.username, mqtt86.username);
	strcpy(mqtt.password, mqtt86.password);
	mqtt.payloadFormat = mqtt86.payloadFormat;
	mqtt.ssl = mqtt86.ssl;
	EEPROM.put(CONFIG_MQTT_START, mqtt);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, 87);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::relocateConfig87() {
	MeterConfig87 meter87;
	MeterConfig meter;
	EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CONFIG_METER_START_87, meter87);
	if(meter87.type < 5) {
		meter.baud = 2400;
		meter.parity = meter87.type == 3 || meter87.type == 4 ? 3 : 11;
		meter.invert = false;
	} else {
		meter.baud = 115200;
		meter.parity = 3;
		meter.invert = meter87.type == 6;
	}
	meter.distributionSystem = meter87.distributionSystem;
	meter.mainFuse = meter87.mainFuse;
	meter.productionCapacity = meter87.productionCapacity;
	EEPROM.put(CONFIG_METER_START, meter);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, 88);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::relocateConfig90() {
	EntsoeConfig entsoe;
	EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CONFIG_ENTSOE_START_90, entsoe);
	EEPROM.put(CONFIG_ENTSOE_START, entsoe);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, 91);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::relocateConfig91() {
	WiFiConfig91 wifi91;
	WiFiConfig wifi;
	EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CONFIG_WIFI_START_91, wifi91);
	strcpy(wifi.ssid, wifi91.ssid);
	strcpy(wifi.psk, wifi91.psk);
	strcpy(wifi.ip, wifi91.ip);
	strcpy(wifi.gateway, wifi91.gateway);
	strcpy(wifi.subnet, wifi91.subnet);
	strcpy(wifi.dns1, wifi91.dns1);
	strcpy(wifi.dns2, wifi91.dns2);
	strcpy(wifi.hostname, wifi91.hostname);
	wifi.mdns = wifi91.mdns;
	EEPROM.put(CONFIG_WIFI_START, wifi);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, 92);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::relocateConfig92() {
	WiFiConfig wifi;
	EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(CONFIG_WIFI_START, wifi);
	#if defined(ESP32)
		wifi.power = 195;
	#elif defined(ESP8266)
		wifi.power = 205;
	#endif
	EEPROM.put(CONFIG_WIFI_START, wifi);

	EnergyAccountingConfig eac;
	clearEnergyAccountingConfig(eac);
	EEPROM.put(CONFIG_ENERGYACCOUNTING_START, eac);

	EEPROM.put(EEPROM_CONFIG_ADDRESS, 93);
	bool ret = EEPROM.commit();
	EEPROM.end();
	return ret;
}

bool AmsConfiguration::save() {
	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(EEPROM_CONFIG_ADDRESS, EEPROM_CHECK_SUM);
	saveTempSensors();
	bool success = EEPROM.commit();
	EEPROM.end();

	configVersion = EEPROM_CHECK_SUM;
	return success;
}

uint8_t AmsConfiguration::getTempSensorCount() {
	return tempSensorCount;
}

TempSensorConfig* AmsConfiguration::getTempSensorConfig(uint8_t address[8]) {
	if(tempSensors == NULL)
		return NULL;
    for(int x = 0; x < tempSensorCount; x++) {
        TempSensorConfig *conf = tempSensors[x];
        if(isSensorAddressEqual(conf->address, address)) {
			return conf;
		}
	}
	return NULL;
}

void AmsConfiguration::updateTempSensorConfig(uint8_t address[8], const char name[32], bool common) {
    bool found = false;
	if(tempSensors != NULL) {
		for(int x = 0; x < tempSensorCount; x++) {
			TempSensorConfig *data = tempSensors[x];
			if(isSensorAddressEqual(data->address, address)) {
				found = true;
				strcpy(data->name, name);
				data->common = common;
			}
		}
	}
    if(!found) {
		TempSensorConfig** tempSensors = new TempSensorConfig*[tempSensorCount+1];
		if(this->tempSensors != NULL) {
			for(int i = 0;i < tempSensorCount; i++) {
				tempSensors[i] = this->tempSensors[i];
			}
		}
        TempSensorConfig *data = new TempSensorConfig();
        memcpy(data->address, address, 8);
        strcpy(data->name, name);
        data->common = common;
        tempSensors[tempSensorCount++] = data;
		if(this->tempSensors != NULL) {
			delete this->tempSensors;
		}
		this->tempSensors = tempSensors;
    }
}

bool AmsConfiguration::isSensorAddressEqual(uint8_t a[8], uint8_t b[8]) {
    for(int i = 0; i < 8; i++) {
        if(a[i] != b[i]) return false;
    }
    return true;
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

int AmsConfiguration::readInt(int address, int *value) {
	int lower = EEPROM.read(address);
	int higher = EEPROM.read(address + 1);
	*value = lower + (higher << 8);
	return 2;
}

int AmsConfiguration::readBool(int address, bool *value) {
	byte y = EEPROM.read(address);
	*value = (bool)y;
	return 1;
}

int AmsConfiguration::readByte(int address, byte *value) {
	*value = EEPROM.read(address);
	return 1;
}

void AmsConfiguration::print(Print* debugger)
{
	debugger->println("-----------------------------------------------");
	WiFiConfig wifi;
	if(getWiFiConfig(wifi)) {
		debugger->println("--WiFi configuration--");
		debugger->printf("SSID:                 '%s'\r\n", wifi.ssid);
		debugger->printf("Psk:                  '%s'\r\n", wifi.psk);
		if(strlen(wifi.ip) > 0) {
			debugger->printf("IP:                   '%s'\r\n", wifi.ip);
			debugger->printf("Gateway:              '%s'\r\n", wifi.gateway);
			debugger->printf("Subnet:               '%s'\r\n", wifi.subnet);
			debugger->printf("DNS1:                 '%s'\r\n", wifi.dns1);
			debugger->printf("DNS2:                 '%s'\r\n", wifi.dns2);
		}
		debugger->printf("Hostname:             '%s'\r\n", wifi.hostname);
		debugger->printf("mDNS:                 '%s'\r\n", wifi.mdns ? "Yes" : "No");
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	MqttConfig mqtt;
	if(getMqttConfig(mqtt)) {
		debugger->println("--MQTT configuration--");
		if(strlen(mqtt.host) > 0) {
			debugger->printf("Enabled:              Yes\r\n");
			debugger->printf("Host:                 '%s'\r\n", mqtt.host);
			debugger->printf("Port:                 %i\r\n", mqtt.port);
			debugger->printf("Client ID:            '%s'\r\n", mqtt.clientId);
			debugger->printf("Publish topic:        '%s'\r\n", mqtt.publishTopic);
			debugger->printf("Subscribe topic:      '%s'\r\n", mqtt.subscribeTopic);
			if (strlen(mqtt.username) > 0) {
				debugger->printf("Username:             '%s'\r\n", mqtt.username);
				debugger->printf("Password:             '%s'\r\n", mqtt.password);
			}
			debugger->printf("Payload format:       %i\r\n", mqtt.payloadFormat);
			debugger->printf("SSL:                  %s\r\n", mqtt.ssl ? "Yes" : "No");
		} else {
			debugger->printf("Enabled:              No\r\n");
		}
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	WebConfig web;
	if(getWebConfig(web)) {
		debugger->println("--Web configuration--");
		debugger->printf("Security:             %i\r\n", web.security);
		if (web.security > 0) {
			debugger->printf("Username:             '%s'\r\n", web.username);
			debugger->printf("Password:             '%s'\r\n", web.password);
		}
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	MeterConfig meter;
	if(getMeterConfig(meter)) {
		debugger->println("--Meter configuration--");
		debugger->printf("Baud:                 %i\r\n", meter.baud);
		debugger->printf("Parity:               %i\r\n", meter.parity);
		debugger->printf("Invert serial:        %s\r\n", meter.invert ? "Yes" : "No");
		debugger->printf("Distribution system:  %i\r\n", meter.distributionSystem);
		debugger->printf("Main fuse:            %i\r\n", meter.mainFuse);
		debugger->printf("Production Capacity:  %i\r\n", meter.productionCapacity);
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	GpioConfig gpio;
	if(getGpioConfig(gpio)) {
		debugger->println("--GPIO configuration--");
		debugger->printf("HAN pin:              %i\r\n", gpio.hanPin);
		debugger->printf("LED pin:              %i\r\n", gpio.ledPin);
		debugger->printf("LED inverted:         %s\r\n", gpio.ledInverted ? "Yes" : "No");
		debugger->printf("LED red pin:          %i\r\n", gpio.ledPinRed);
		debugger->printf("LED green pin:        %i\r\n", gpio.ledPinGreen);
		debugger->printf("LED blue pin:         %i\r\n", gpio.ledPinBlue);
		debugger->printf("LED inverted:         %s\r\n", gpio.ledRgbInverted ? "Yes" : "No");
		debugger->printf("AP pin:               %i\r\n", gpio.apPin);
		debugger->printf("Temperature pin:      %i\r\n", gpio.tempSensorPin);
		debugger->printf("Temp analog pin:      %i\r\n", gpio.tempAnalogSensorPin);
		debugger->printf("Vcc pin:              %i\r\n", gpio.vccPin);
		if(gpio.vccMultiplier > 0) {
			debugger->printf("Vcc multiplier:       %f\r\n", gpio.vccMultiplier / 1000.0);
		}
		if(gpio.vccOffset > 0) {
			debugger->printf("Vcc offset:           %f\r\n", gpio.vccOffset / 100.0);
		}
		if(gpio.vccBootLimit > 0) {
			debugger->printf("Vcc boot limit:       %f\r\n", gpio.vccBootLimit / 10.0);
		}
		debugger->printf("GND resistor:         %i\r\n", gpio.vccResistorGnd);
		debugger->printf("Vcc resistor:         %i\r\n", gpio.vccResistorVcc);
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	DomoticzConfig domo;
	if(getDomoticzConfig(domo)) {
		debugger->println("--Domoticz configuration--");
		if(mqtt.payloadFormat == 3 && domo.elidx > 0) {
			debugger->printf("Enabled:              Yes\r\n");
			debugger->printf("Domoticz ELIDX:       %i\r\n", domo.elidx);
			debugger->printf("Domoticz VL1IDX:      %i\r\n", domo.vl1idx);
			debugger->printf("Domoticz VL2IDX:      %i\r\n", domo.vl2idx);
			debugger->printf("Domoticz VL3IDX:      %i\r\n", domo.vl3idx);
			debugger->printf("Domoticz CL1IDX:      %i\r\n", domo.cl1idx);
		} else {
			debugger->printf("Enabled:              No\r\n");
		}
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	NtpConfig ntp;
	if(getNtpConfig(ntp)) {
		debugger->println("--NTP configuration--");
		debugger->printf("Enabled:              %s\r\n", ntp.enable ? "Yes" : "No");
		if(ntp.enable) {
			debugger->printf("Offset:               %i\r\n", ntp.offset);
			debugger->printf("Summer offset:        %i\r\n", ntp.summerOffset);
			debugger->printf("Server:               %s\r\n", ntp.server);
			debugger->printf("DHCP:                 %s\r\n", ntp.dhcp ? "Yes" : "No");
		}
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	EntsoeConfig entsoe;
	if(getEntsoeConfig(entsoe)) {
		debugger->println("--ENTSO-E configuration--");
		debugger->printf("Token:                %s\r\n", entsoe.token);
		if(strlen(entsoe.token) > 0) {
			debugger->printf("Area:                 %s\r\n", entsoe.area);
			debugger->printf("Currency:             %s\r\n", entsoe.currency);
			debugger->printf("Multiplier:           %f\r\n", entsoe.multiplier / 1000.0);
		}
		debugger->println("");
		delay(10);
		Serial.flush();
	}

	debugger->printf("Temp sensor count:    %i\r\n", this->getTempSensorCount());

	debugger->println("-----------------------------------------------");
}
