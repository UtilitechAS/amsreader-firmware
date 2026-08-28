/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */
#include "AmsJsonGenerator.h"
#include "hexutils.h"
#include "AmsStorage.h"
#include "LittleFS.h"
#include "FirmwareVersion.h"
#include "AmsData.h"
#include "PriceService.h"
#include "NtpStatus.h"
#include "Uptime.h"
#include "mqtt/AmsMqttHandler.h"
#if defined(AMS_CLOUD)
#include "cloud/CloudConnector.h"
#endif
#if defined(ZMART_CHARGE)
#include "cloud/ZmartChargeCloudConnector.h"
#endif

// SNTP resyncs roughly hourly; flag the NTP service as degraded if no sync has
// landed in this long, allowing a couple of missed cycles before warning.
#define NTP_STALE_AFTER_SECONDS 10800

uint8_t AmsJsonGenerator::hanState(AmsData* meterState) {
    if(meterState == NULL) return 2;
    uint64_t millis = millis64();
    if(meterState->getLastError() != 0) return 3;
    // State 0 means disabled, which the HAN port never is. Waiting for the first
    // frame after boot is the connecting state.
    if(meterState->getLastUpdateMillis() == 0 && millis < 30000) return 2;
    if(millis - meterState->getLastUpdateMillis() < 15000) return 1;
    if(millis - meterState->getLastUpdateMillis() < 30000) return 2;
    return 3;
}

uint8_t AmsJsonGenerator::mqttHandlerState(AmsMqttHandler* h) {
    if(h == NULL) return 2;
    if(h->connected()) return 1;
    return h->lastError() == 0 ? 2 : 3;
}

// Formats one entry of the services array. Kept in one place so the web payload
// and the compact MQTT payload (#1128) cannot drift apart.
static void appendServiceEntry(String& out, bool withDetail, const char* key, uint8_t state, int16_t err, const char* detail, const char* name) {
    char entry[320];
    if(withDetail) {
        snprintf_P(entry, sizeof(entry), PSTR("{\"k\":\"%s\",\"s\":%d,\"e\":%d%s%s%s,\"d\":\"%s\"}"),
            key, state, err,
            name != NULL ? ",\"n\":\"" : "", name != NULL ? name : "", name != NULL ? "\"" : "",
            detail == NULL ? "" : detail);
    } else {
        snprintf_P(entry, sizeof(entry), PSTR("{\"k\":\"%s\",\"s\":%d,\"e\":%d}"), key, state, err);
    }
    if(!out.isEmpty()) out += ",";
    out += entry;
}

String AmsJsonGenerator::generateServicesJson(const ServiceStatusContext& ctx, bool withDetail) {
    String out = "";
    if(ctx.config == NULL) return out;

    {
        String meterModel = ctx.meterState == NULL ? String("") : String(ctx.meterState->getMeterModel());
        if(!meterModel.isEmpty())
            meterModel.replace(F("\\"), F("\\\\"));
        appendServiceEntry(out, withDetail, "han", hanState(ctx.meterState),
            ctx.meterState == NULL ? 0 : ctx.meterState->getLastError(), meterModel.c_str(), NULL);
    }

    MqttConfig mqttConfig;
    bool haveMqttConfig = ctx.config->getMqttConfig(mqttConfig);
    if(haveMqttConfig && strlen(mqttConfig.host) > 0) {
        uint8_t s;
        int16_t err = 0;
        if(!ctx.mqttEnabled) {
            s = 0;
        } else {
            s = mqttHandlerState(ctx.mqttHandler);
            if(ctx.mqttHandler != NULL) err = (int16_t) ctx.mqttHandler->lastError();
        }
        appendServiceEntry(out, withDetail, "mqtt", s, err, mqttConfig.host, NULL);
    }

    #if defined(CUSTOM_MQTT_HOST)
    {
        uint8_t s = mqttHandlerState(ctx.customMqttHandler);
        int16_t err = ctx.customMqttHandler == NULL ? 0 : (int16_t) ctx.customMqttHandler->lastError();
        #if defined(CUSTOM_MQTT_NAME)
        appendServiceEntry(out, withDetail, "mqtt_c", s, err, CUSTOM_MQTT_HOST, CUSTOM_MQTT_NAME);
        #else
        appendServiceEntry(out, withDetail, "mqtt_c", s, err, CUSTOM_MQTT_HOST, NULL);
        #endif
    }
    #endif

    #if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
    {
        SystemConfig sys;
        ctx.config->getSystemConfig(sys);
        if(sys.energyspeedometer == 7) {
            uint8_t s = mqttHandlerState(ctx.energySpeedometer);
            int16_t err = ctx.energySpeedometer == NULL ? 0 : (int16_t) ctx.energySpeedometer->lastError();
            appendServiceEntry(out, withDetail, "mqtt_es", s, err, "", NULL);
        }
    }
    #endif

    PriceServiceConfig priceCfg;
    if(ctx.config->getPriceServiceConfig(priceCfg) && priceCfg.enabled && strlen(priceCfg.area) > 0) {
        uint8_t s;
        int16_t err = ctx.ps == NULL ? 0 : ctx.ps->getLastError();
        if(ctx.ps == NULL) {
            s = 2;
        } else if(err != 0) {
            s = 3;
        } else if(ctx.ps->hasPrice()) {
            s = 1;
        } else {
            s = 2;
        }
        appendServiceEntry(out, withDetail, "price", s, err, priceCfg.area, NULL);
    }

    {
        NtpConfig ntp;
        if(ctx.config->getNtpConfig(ntp) && ntp.enable) {
            const char* server = strlen(ntp.server) > 0 ? ntp.server : "pool.ntp.org";
            // A set-but-stale clock (NTP stopped resyncing) silently corrupts
            // day-boundary accounting, so flag staleness rather than only
            // reporting whether the clock was ever set.
            uint64_t lastSync = ntpLastSyncMillis();
            uint8_t s;
            if(lastSync == 0) {
                s = 2; // No SNTP sync since boot yet
            } else {
                uint32_t ageSec = (uint32_t) ((millis64() - lastSync) / 1000);
                s = ageSec > NTP_STALE_AFTER_SECONDS ? 2 : 1;
            }
            appendServiceEntry(out, withDetail, "ntp", s, 0, server, NULL);
        }
    }

    #if defined(AMS_CLOUD)
    {
        CloudConfig cc;
        if(ctx.config->getCloudConfig(cc) && cc.enabled) {
            uint8_t s;
            int16_t err = ctx.cloud == NULL ? 0 : ctx.cloud->getLastError();
            if(ctx.cloud == NULL || !ctx.cloud->isInitialized()) {
                s = 2;
            } else {
                unsigned long since = millis() - ctx.cloud->getLastUpdate();
                uint32_t maxAge = ((uint32_t) cc.interval) * 3000;
                s = (ctx.cloud->getLastUpdate() > 0 && since > maxAge) ? 3 : 1;
            }
            appendServiceEntry(out, withDetail, "cloud", s, err, cc.hostname, NULL);
        }
    }
    #endif

    #if defined(ZMART_CHARGE)
    {
        ZmartChargeConfig zc;
        if(ctx.config->getZmartChargeConfig(zc) && zc.enabled) {
            uint8_t s;
            int16_t err = ctx.zcloud == NULL ? 0 : ctx.zcloud->getLastError();
            if(ctx.zcloud == NULL || ctx.zcloud->getLastUpdate() == 0) {
                s = 2;
            } else {
                s = ctx.zcloud->isLastFailed() ? 3 : 1;
            }
            appendServiceEntry(out, withDetail, "zc", s, err, zc.baseUrl, NULL);
        }
    }
    #endif

    return out;
}

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

void AmsJsonGenerator::generateConfigurationJson(AmsConfiguration* config, JsonSink& sink) {
	// Scratch for one section at a time; sized for the largest (MQTT host + credentials).
	// Lives on the stack so it costs no heap, and is flushed to the sink per section so
	// the total document size is bounded by the sink, not by this buffer.
	char chunk[768];
	#define EMIT(...) do { \
			int _n = snprintf_P(chunk, sizeof(chunk), __VA_ARGS__); \
			if(_n < 0) _n = 0; \
			else if((size_t)_n >= sizeof(chunk)) _n = sizeof(chunk) - 1; \
			sink.write(chunk, (size_t)_n); \
		} while(0)

	EMIT(PSTR("{\"version\":\"%s\""), FirmwareVersion::VersionString);

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
	EMIT(PSTR(",\"g\":{\"t\":\"%s\",\"h\":\"%s\",\"s\":%d,\"u\":\"%s\",\"p\":\"%s\",\"c\":\"%s\"}"),
		ntpConfig.timezone,
		networkConfig.hostname,
		webConfig.security,
		webConfig.username,
		strlen(webConfig.password) > 0 ? "***" : "",
		webConfig.context
	);

	// Meter
	EMIT(PSTR(",\"m\":{\"o\":%d,\"a\":%d,\"b\":%d,\"p\":%d,\"i\":%s,\"s\":%d,\"d\":%d,\"f\":%d,\"r\":%d"),
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
	EMIT(PSTR(",\"e\":{\"e\":%s,\"k\":\"%s\",\"a\":\"%s\"}"),
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
	EMIT(PSTR(",\"m\":{\"e\":%s,\"w\":%.3f,\"v\":%.3f,\"a\":%.3f,\"c\":%.3f}"),
		multEnable ? "true" : "false",
		meterConfig.wattageMultiplier == 0.0 ? 1.0 : meterConfig.wattageMultiplier / 1000.0,
		meterConfig.voltageMultiplier == 0.0 ? 1.0 : meterConfig.voltageMultiplier / 1000.0,
		meterConfig.amperageMultiplier == 0.0 ? 1.0 : meterConfig.amperageMultiplier / 1000.0,
		meterConfig.accumulatedMultiplier == 0.0 ? 1.0 : meterConfig.accumulatedMultiplier / 1000.0
	);

	EMIT(PSTR("}")); // End of meter

	// Thresholds
	EMIT(PSTR(",\"t\":{\"t\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\"h\":%d}"),
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
	EMIT(PSTR(",\"w\":{\"s\":\"%s\",\"p\":\"%s\",\"w\":%.1f,\"z\":%d,\"b\":%s}"),
		networkConfig.ssid,
		strlen(networkConfig.psk) > 0 ? "***" : "",
		networkConfig.power / 10.0,
		networkConfig.sleep,
		networkConfig.use11b ? "true" : "false"
	);

	// Network
	EMIT(PSTR(",\"n\":{\"c\":%d,\"m\":\"%s\",\"i\":\"%s\",\"s\":\"%s\",\"g\":\"%s\",\"d1\":\"%s\",\"d2\":\"%s\",\"d\":%s,\"n1\":\"%s\",\"h\":%s,\"x\":%s}"),
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
	EMIT(PSTR(",\"q\":{\"h\":\"%s\",\"p\":%d,\"u\":\"%s\",\"a\":\"%s\",\"c\":\"%s\",\"b\":\"%s\",\"r\":\"%s\",\"m\":%d,\"s\":{\"e\":%s,\"c\":%s,\"r\":%s,\"k\":%s},\"t\":%d,\"d\":%d,\"i\":%d,\"k\":%d,\"e\":%s,\"dc\":%s}"),
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
		mqttConfig.rebootMinutes == 0 ? "null" : String(mqttConfig.rebootMinutes, 10).c_str(),
		mqttConfig.allowDestructiveCommands ? "true" : "false"
	);

	// Price
	EMIT(PSTR(",\"p\":{\"e\":%s,\"t\":\"%s\",\"r\":\"%s\",\"c\":\"%s\",\"m\":%d}"),
		price.enabled ? "true" : "false",
		price.entsoeToken,
		price.area,
		price.currency,
		price.resolutionInMinutes
	);

	// Debug
	EMIT(PSTR(",\"d\":{\"s\":%s,\"t\":%s,\"l\":%d}"),
		debugConfig.serial ? "true" : "false",
		debugConfig.telnet ? "true" : "false",
		debugConfig.level
	);

	// GPIO
	EMIT(PSTR(",\"i\":{\"h\":{\"p\":%s,\"u\":%s,\"t\":%s},\"a\":%s,\"l\":{\"p\":%s,\"i\":%s},\"r\":{\"r\":%s,\"g\":%s,\"b\":%s,\"i\":%s},\"d\":{\"d\":%s,\"b\":%d},\"t\":{\"d\":%s,\"a\":%s},\"v\":{\"p\":%s,\"o\":%.2f,\"m\":%.3f,\"d\":{\"v\":%d,\"g\":%d},\"b\":%.1f},\"p\":%d}"),
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
	EMIT(PSTR(",\"u\":{\"i\":%d,\"e\":%d,\"v\":%d,\"a\":%d,\"r\":%d,\"c\":%d,\"t\":%d,\"p\":%d,\"d\":%d,\"m\":%d,\"s\":%d,\"l\":%d,\"h\":%d,\"f\":%d,\"k\":%d,\"lang\":\"%s\"}"),
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
	EMIT(PSTR(",\"o\":{\"e\":%d,\"c\":%d,\"u1\":%d,\"u2\":%d,\"u3\":%d}"),
		domo.elidx,
		domo.cl1idx,
		domo.vl1idx,
		domo.vl2idx,
		domo.vl3idx
	);

	// Home-Assistant
	EMIT(PSTR(",\"h\":{\"t\":\"%s\",\"h\":\"%s\",\"n\":\"%s\"}"),
		haconf.discoveryPrefix,
		haconf.discoveryHostname,
		haconf.discoveryNameTag
	);

	// Cloud
	EMIT(PSTR(",\"c\":{\"e\":%s,\"p\":%d,\"es\":%s,\"ze\":%s,\"zt\":\"%s\"}"),
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

	EMIT(PSTR("}")); // End of config
	#undef EMIT
}

void AmsJsonGenerator::generateConfigurationJson(AmsConfiguration* config, char* buf, size_t bufSize) {
	BufferJsonSink sink(buf, bufSize);
	generateConfigurationJson(config, sink);
}