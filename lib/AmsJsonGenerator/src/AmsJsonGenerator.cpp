#include "AmsJsonGenerator.h"
#include "hexutils.h"
#include "AmsStorage.h"
#include "LittleFS.h"
#include "FirmwareVersion.h"

void AmsJsonGenerator::generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize) {
		uint16_t pos = snprintf_P(buf, bufSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 0; i < 24; i++) {
			pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getHourImport(i) / 1000.0, i, ds->getHourExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, bufSize-pos, PSTR("}"));
}

void AmsJsonGenerator::generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize) {
		uint16_t pos = snprintf_P(buf, bufSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 1; i < 32; i++) {
			pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getDayImport(i) / 1000.0, i, ds->getDayExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, bufSize-pos, PSTR("}"));
}

void AmsJsonGenerator::generateConfigurationJson(AmsConfiguration* config, char* buf, size_t bufSize) {
	uint16_t pos = snprintf_P(buf, bufSize, PSTR("{\"version\":\"%s\""), FirmwareVersion::VersionString);

	SystemConfig sysConfig;
	config->getSystemConfig(sysConfig);

	WebConfig webConfig;
	config->getWebConfig(webConfig);

	MeterConfig meterConfig;
	config->getMeterConfig(meterConfig);

	NetworkConfig networkConfig;
	config->getNetworkConfig(networkConfig);

	NtpConfig ntpConfig;
	config->getNtpConfig(ntpConfig);

	EnergyAccountingConfig eac;
	config->getEnergyAccountingConfig(eac);

	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);

	PriceServiceConfig price;
	config->getPriceServiceConfig(price);

	DebugConfig debugConfig;
	config->getDebugConfig(debugConfig);

	GpioConfig gpioConfig;
	config->getGpioConfig(gpioConfig);

	UiConfig ui;
	config->getUiConfig(ui);

	DomoticzConfig domo;
	config->getDomoticzConfig(domo);

	HomeAssistantConfig haconf;
	config->getHomeAssistantConfig(haconf);

	CloudConfig cloud;
	config->getCloudConfig(cloud);

	ZmartChargeConfig zcc;
	config->getZmartChargeConfig(zcc);

	// General
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"g\":{\"t\":\"%s\",\"h\":\"%s\",\"s\":%d,\"u\":\"%s\",\"p\":\"%s\",\"c\":\"%s\"}"),
		ntpConfig.timezone,
		networkConfig.hostname,
		webConfig.security,
		webConfig.username,
		strlen(webConfig.password) > 0 ? "***" : "",
		webConfig.context
	);

	// Meter
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"m\":{\"o\":%d,\"a\":%d,\"b\":%d,\"p\":%d,\"i\":%s,\"s\":%d,\"d\":%d,\"f\":%d,\"r\":%d"),
		meterConfig.source,
		meterConfig.parser,
		meterConfig.baud,
		meterConfig.parity,
		meterConfig.invert ? "true" : "false",
		meterConfig.bufferSize * 64,
		meterConfig.distributionSystem,
		meterConfig.mainFuse,
		meterConfig.productionCapacity
	);

	bool encen = false;
	for(uint8_t i = 0; i < 16; i++) {
		if(meterConfig.encryptionKey[i] > 0) {
			encen = true;
		}
	}
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"e\":{\"e\":%s,\"k\":\"%s\",\"a\":\"%s\"}"),
		encen ? "true" : "false",
		toHex(meterConfig.encryptionKey, 16).c_str(),
		toHex(meterConfig.authenticationKey, 16).c_str()
	);

	bool multEnable = false;
	if(meterConfig.wattageMultiplier != 1.0 && meterConfig.wattageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.voltageMultiplier != 1.0 && meterConfig.voltageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.amperageMultiplier != 1.0 && meterConfig.amperageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.accumulatedMultiplier != 1.0 && meterConfig.accumulatedMultiplier != 0.0)
		multEnable = true;
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"m\":{\"e\":%s,\"w\":%.3f,\"v\":%.3f,\"a\":%.3f,\"c\":%.3f}"),
		multEnable ? "true" : "false",
		meterConfig.wattageMultiplier == 0.0 ? 1.0 : meterConfig.wattageMultiplier / 1000.0,
		meterConfig.voltageMultiplier == 0.0 ? 1.0 : meterConfig.voltageMultiplier / 1000.0,
		meterConfig.amperageMultiplier == 0.0 ? 1.0 : meterConfig.amperageMultiplier / 1000.0,
		meterConfig.accumulatedMultiplier == 0.0 ? 1.0 : meterConfig.accumulatedMultiplier / 1000.0
	);

	pos += snprintf_P(buf+pos, bufSize-pos, PSTR("}")); // End of meter

	// Thresholds
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"t\":{\"t\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\"h\":%d}"),
		eac.thresholds[0],
		eac.thresholds[1],
		eac.thresholds[2],
		eac.thresholds[3],
		eac.thresholds[4],
		eac.thresholds[5],
		eac.thresholds[6],
		eac.thresholds[7],
		eac.thresholds[8],
		eac.thresholds[9],
		eac.hours
	);

	// WiFi
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"w\":{\"s\":\"%s\",\"p\":\"%s\",\"w\":%.1f,\"z\":%d,\"b\":%s}"),
		networkConfig.ssid,
		strlen(networkConfig.psk) > 0 ? "***" : "",
		networkConfig.power / 10.0,
		networkConfig.sleep,
		networkConfig.use11b ? "true" : "false"
	);

	// Network
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"n\":{\"c\":%d,\"m\":\"%s\",\"i\":\"%s\",\"s\":\"%s\",\"g\":\"%s\",\"d1\":\"%s\",\"d2\":\"%s\",\"d\":%s,\"n1\":\"%s\",\"h\":%s,\"x\":%s}"),
		networkConfig.mode,
		strlen(networkConfig.ip) > 0 ? "static" : "dhcp",
		networkConfig.ip,
		networkConfig.subnet,
		networkConfig.gateway,
		networkConfig.dns1,
		networkConfig.dns2,
		networkConfig.mdns ? "true" : "false",
		ntpConfig.server,
		ntpConfig.dhcp ? "true" : "false",
		networkConfig.ipv6 ? "true" : "false"
	);

	// MQTT
	bool qsc = false;
	bool qsr = false;
	bool qsk = false;
	if(LittleFS.begin()) {
		qsc = LittleFS.exists(FILE_MQTT_CA);
		qsr = LittleFS.exists(FILE_MQTT_CERT);
		qsk = LittleFS.exists(FILE_MQTT_KEY);
	}
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"q\":{\"h\":\"%s\",\"p\":%d,\"u\":\"%s\",\"a\":\"%s\",\"c\":\"%s\",\"b\":\"%s\",\"r\":\"%s\",\"m\":%d,\"s\":{\"e\":%s,\"c\":%s,\"r\":%s,\"k\":%s},\"t\":%d,\"d\":%d,\"i\":%d,\"k\":%d,\"e\":%d}"),
		mqttConfig.host,
		mqttConfig.port,
		mqttConfig.username,
		strlen(mqttConfig.password) > 0 ? "***" : "",
		mqttConfig.clientId,
		mqttConfig.publishTopic,
		mqttConfig.subscribeTopic,
		mqttConfig.payloadFormat,
		mqttConfig.ssl ? "true" : "false",
		qsc ? "true" : "false",
		qsr ? "true" : "false",
		qsk ? "true" : "false",
		mqttConfig.stateUpdate,
		mqttConfig.stateUpdateInterval,
		mqttConfig.timeout,
		mqttConfig.keepalive,
		mqttConfig.rebootMinutes == 0 ? "null" : String(mqttConfig.rebootMinutes, 10).c_str()
	);

	// Price
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"p\":{\"e\":%s,\"t\":\"%s\",\"r\":\"%s\",\"c\":\"%s\",\"m\":%d}"),
		price.enabled ? "true" : "false",
		price.entsoeToken,
		price.area,
		price.currency,
		price.resolutionInMinutes
	);

	// Debug
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"d\":{\"s\":%s,\"t\":%s,\"l\":%d}"),
		debugConfig.serial ? "true" : "false",
		debugConfig.telnet ? "true" : "false",
		debugConfig.level
	);

	// GPIO
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"i\":{\"h\":{\"p\":%s,\"u\":%s,\"t\":%s},\"a\":%s,\"l\":{\"p\":%s,\"i\":%s},\"r\":{\"r\":%s,\"g\":%s,\"b\":%s,\"i\":%s},\"d\":{\"d\":%s,\"b\":%d},\"t\":{\"d\":%s,\"a\":%s},\"v\":{\"p\":%s,\"o\":%.2f,\"m\":%.3f,\"d\":{\"v\":%d,\"g\":%d},\"b\":%.1f},\"p\":%d}"),
		meterConfig.rxPin == 0xff ? "null" : String(meterConfig.rxPin, 10).c_str(),
		meterConfig.rxPinPullup ? "true" : "false",
		meterConfig.txPin == 0xff ? "null" : String(meterConfig.txPin, 10).c_str(),
		gpioConfig.apPin == 0xff ? "null" : String(gpioConfig.apPin, 10).c_str(),
		gpioConfig.ledPin == 0xff ? "null" : String(gpioConfig.ledPin, 10).c_str(),
		gpioConfig.ledInverted ? "true" : "false",
		gpioConfig.ledPinRed == 0xff ? "null" : String(gpioConfig.ledPinRed, 10).c_str(),
		gpioConfig.ledPinGreen == 0xff ? "null" : String(gpioConfig.ledPinGreen, 10).c_str(),
		gpioConfig.ledPinBlue == 0xff ? "null" : String(gpioConfig.ledPinBlue, 10).c_str(),
		gpioConfig.ledRgbInverted ? "true" : "false",
		gpioConfig.ledDisablePin == 0xff ? "null" : String(gpioConfig.ledDisablePin, 10).c_str(),
		gpioConfig.ledBehaviour,
		gpioConfig.tempSensorPin == 0xff ? "null" : String(gpioConfig.tempSensorPin, 10).c_str(),
		gpioConfig.tempAnalogSensorPin == 0xff ? "null" : String(gpioConfig.tempAnalogSensorPin, 10).c_str(),
		gpioConfig.vccPin == 0xff ? "null" : String(gpioConfig.vccPin, 10).c_str(),
		gpioConfig.vccOffset / 100.0,
		gpioConfig.vccMultiplier / 1000.0,
		gpioConfig.vccResistorVcc,
		gpioConfig.vccResistorGnd,
		gpioConfig.vccBootLimit / 10.0,
		gpioConfig.powersaving
	);

	// UI
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"u\":{\"i\":%d,\"e\":%d,\"v\":%d,\"a\":%d,\"r\":%d,\"c\":%d,\"t\":%d,\"p\":%d,\"d\":%d,\"m\":%d,\"s\":%d,\"l\":%d,\"h\":%d,\"f\":%d,\"k\":%d,\"lang\":\"%s\"}"),
		ui.showImport,
		ui.showExport,
		ui.showVoltage,
		ui.showAmperage,
		ui.showReactive,
		ui.showRealtime,
		ui.showPeaks,
		ui.showPricePlot,
		ui.showDayPlot,
		ui.showMonthPlot,
		ui.showTemperaturePlot,
		ui.showRealtimePlot,
		ui.showPerPhasePower,
		ui.showPowerFactor,
		ui.darkMode,
		ui.language
	);

	// Domoticz
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"o\":{\"e\":%d,\"c\":%d,\"u1\":%d,\"u2\":%d,\"u3\":%d}"),
		domo.elidx,
		domo.cl1idx,
		domo.vl1idx,
		domo.vl2idx,
		domo.vl3idx
	);

	// Home-Assistant
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"h\":{\"t\":\"%s\",\"h\":\"%s\",\"n\":\"%s\"}"),
		haconf.discoveryPrefix,
		haconf.discoveryHostname,
		haconf.discoveryNameTag
	);

	// Cloud
	pos += snprintf_P(buf+pos, bufSize-pos, PSTR(",\"c\":{\"e\":%s,\"p\":%d,\"es\":%s,\"ze\":%s,\"zt\":\"%s\"}"),
		cloud.enabled ? "true" : "false",
		cloud.proto,
		#if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
		sysConfig.energyspeedometer == 7 ? "true" : "false",
		#else
		"null",
		#endif
		zcc.enabled ? "true" : "false",
		zcc.token
	);

	pos += snprintf_P(buf+pos, bufSize-pos, PSTR("}")); // End of config
}