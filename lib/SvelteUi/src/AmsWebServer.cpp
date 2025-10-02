/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "AmsWebServer.h"
#include "AmsWebHeaders.h"
#include "FirmwareVersion.h"
#include "base64.h"
#include "hexutils.h"

#include "html/index_html.h"
#include "html/index_css.h"
#include "html/index_js.h"
#include "html/favicon_svg.h"
#include "html/data_json.h"
#include "html/response_json.h"
#include "html/sysinfo_json.h"
#include "html/tariff_json.h"
#include "html/conf_general_json.h"
#include "html/conf_meter_json.h"
#include "html/conf_wifi_json.h"
#include "html/conf_net_json.h"
#include "html/conf_mqtt_json.h"
#include "html/conf_price_json.h"
#include "html/conf_price_row_json.h"
#include "html/conf_thresholds_json.h"
#include "html/conf_debug_json.h"
#include "html/conf_gpio_json.h"
#include "html/conf_domoticz_json.h"
#include "html/conf_ha_json.h"
#include "html/conf_ui_json.h"
#include "html/conf_cloud_json.h"
#include "html/firmware_html.h"
#include "html/neas_logotype_white_svg.h"
#include "html/wifi_high_light_svg.h"
#include "html/wifi_medium_light_svg.h"
#include "html/wifi_low_light_svg.h"
#include "html/wifi_off_light_svg.h"

#if defined(ESP32)
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <esp32/clk.h>
#endif


#if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include "esp32/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/rtc.h"
#endif

#if defined(AMS_REMOTE_DEBUG)
AmsWebServer::AmsWebServer(uint8_t* buf, RemoteDebug* Debug, HwTools* hw, ResetDataContainer* rdc) {
#else
AmsWebServer::AmsWebServer(uint8_t* buf, Stream* Debug, HwTools* hw, ResetDataContainer* rdc) {
#endif
	this->debugger = Debug;
	this->hw = hw;
	this->buf = (char*) buf;
	this->rdc = rdc;
	if(rdc->magic != 0x4a) {
		rdc->last_cause = 0;
		rdc->cause = 0;
		rdc->magic = 0x4a;
	} else {
		rdc->last_cause = rdc->cause;
		rdc->cause = 0;
	}
}

void AmsWebServer::setup(AmsConfiguration* config, GpioConfig* gpioConfig, AmsData* meterState, AmsDataStorage* ds, EnergyAccounting* ea, RealtimePlot* rtp, AmsFirmwareUpdater* updater) {
    this->config = config;
	this->gpioConfig = gpioConfig;
	this->meterState = meterState;
	this->ds = ds;
	this->ea = ea;
	this->rtp = rtp;
	this->updater = updater;

	String context;
	config->getWebConfig(webConfig);
	stripNonAscii((uint8_t*) webConfig.context, 32);
	if(strlen(webConfig.context) > 0) {
		context = "/" + String(webConfig.context);
		context.replace(" ", "");
		if(context.length() == 1) {
			context = "";
		} else {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::INFO))
			#endif
			debugger->printf_P(PSTR("Using context path: '%s'\n"), context.c_str());
		}
	}

	if(context.isEmpty()) {
		server.on(F("/"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	} else {
		server.on(F("/"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this));
		server.on(context, HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
		server.on(context + F("/"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	}
	snprintf_P(buf, 32, PSTR("%s/index-%s.js"), context.c_str(), FirmwareVersion::VersionString);
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::indexJs, this));
	snprintf_P(buf, 32, PSTR("%s/index-%s.css"), context.c_str(), FirmwareVersion::VersionString);
	server.on(buf, HTTP_GET, std::bind(&AmsWebServer::indexCss, this));

	server.on(context + F("/configuration"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/priceconfig"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/status"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/consent"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/vendor"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/setup"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/mqtt-ca"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/mqtt-cert"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/mqtt-key"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/edit-day"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	server.on(context + F("/edit-month"), HTTP_GET, std::bind(&AmsWebServer::indexHtml, this));
	
	server.on(context + F("/favicon.svg"), HTTP_GET, std::bind(&AmsWebServer::faviconSvg, this)); 
	server.on(context + F("/logo.svg"), HTTP_GET, std::bind(&AmsWebServer::logoSvg, this)); 
	server.on(context + F("/neas_logotype_white.svg"), HTTP_GET, std::bind(&AmsWebServer::neasLogoSvg, this));
	server.on(context + F("/wifi-high-light.svg"), HTTP_GET, std::bind(&AmsWebServer::wifiHighLightSvg, this));
	server.on(context + F("/wifi-medium-light.svg"), HTTP_GET, std::bind(&AmsWebServer::wifiMediumLightSvg, this));
	server.on(context + F("/wifi-low-light.svg"), HTTP_GET, std::bind(&AmsWebServer::wifiLowLightSvg, this));
	server.on(context + F("/wifi-off.svg"), HTTP_GET, std::bind(&AmsWebServer::wifiOffSvg, this));

	server.on(context + F("/sysinfo.json"), HTTP_GET, std::bind(&AmsWebServer::sysinfoJson, this));
	server.on(context + F("/data.json"), HTTP_GET, std::bind(&AmsWebServer::dataJson, this));
	server.on(context + F("/dayplot.json"), HTTP_GET, std::bind(&AmsWebServer::dayplotJson, this));
	server.on(context + F("/monthplot.json"), HTTP_GET, std::bind(&AmsWebServer::monthplotJson, this));
	server.on(context + F("/energyprice.json"), HTTP_GET, std::bind(&AmsWebServer::energyPriceJson, this));
	server.on(context + F("/temperature.json"), HTTP_GET, std::bind(&AmsWebServer::temperatureJson, this));
	server.on(context + F("/tariff.json"), HTTP_GET, std::bind(&AmsWebServer::tariffJson, this));
	server.on(context + F("/realtime.json"), HTTP_GET, std::bind(&AmsWebServer::realtimeJson, this));
	server.on(context + F("/priceconfig.json"), HTTP_GET, std::bind(&AmsWebServer::priceConfigJson, this));
	server.on(context + F("/translations.json"), HTTP_GET, std::bind(&AmsWebServer::translationsJson, this));
	server.on(context + F("/cloudkey.json"), HTTP_GET, std::bind(&AmsWebServer::cloudkeyJson, this));

	server.on(context + F("/wifiscan.json"), HTTP_GET, std::bind(&AmsWebServer::wifiScan, this));

	server.on(context + F("/configuration.json"), HTTP_GET, std::bind(&AmsWebServer::configurationJson, this));
	server.on(context + F("/save"), HTTP_POST, std::bind(&AmsWebServer::handleSave, this));
	server.on(context + F("/reboot"), HTTP_POST, std::bind(&AmsWebServer::reboot, this));
	server.on(context + F("/upgrade"), HTTP_POST, std::bind(&AmsWebServer::upgrade, this));
	server.on(context + F("/firmware"), HTTP_GET, std::bind(&AmsWebServer::firmwareHtml, this));
	server.on(context + F("/firmware"), HTTP_POST, std::bind(&AmsWebServer::firmwarePost, this), std::bind(&AmsWebServer::firmwareUpload, this));
	server.on(context + F("/is-alive"), HTTP_GET, std::bind(&AmsWebServer::isAliveCheck, this));

	server.on(context + F("/reset"), HTTP_POST, std::bind(&AmsWebServer::factoryResetPost, this));

	server.on(context + F("/robots.txt"), HTTP_GET, std::bind(&AmsWebServer::robotstxt, this));

	server.on(context + F("/mqtt-ca"), HTTP_POST, std::bind(&AmsWebServer::mqttCaDelete, this), std::bind(&AmsWebServer::mqttCaUpload, this));
	server.on(context + F("/mqtt-cert"), HTTP_POST, std::bind(&AmsWebServer::mqttCertDelete, this), std::bind(&AmsWebServer::mqttCertUpload, this));
	server.on(context + F("/mqtt-key"), HTTP_POST, std::bind(&AmsWebServer::mqttKeyDelete, this), std::bind(&AmsWebServer::mqttKeyUpload, this));

	server.on(context + F("/configfile"), HTTP_POST, std::bind(&AmsWebServer::configFilePost, this), std::bind(&AmsWebServer::configFileUpload, this));
	server.on(context + F("/configfile.cfg"), HTTP_GET, std::bind(&AmsWebServer::configFileDownload, this));

	server.on(context + F("/dayplot"), HTTP_POST, std::bind(&AmsWebServer::modifyDayPlot, this));
	server.on(context + F("/monthplot"), HTTP_POST, std::bind(&AmsWebServer::modifyMonthPlot, this));

	server.on(context + F("/sysinfo.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/data.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/dayplot.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/monthplot.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/energyprice.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/temperature.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/tariff.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/realtime.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/priceconfig.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/translations.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/cloudkey.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));
	server.on(context + F("/configuration.json"), HTTP_OPTIONS, std::bind(&AmsWebServer::optionsGet, this));

	/* These trigger captive portal. Only problem is that after you have "signed in", the portal is closed and the user has no idea how to reach the device
	server.on(context + F("/generate_204"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Android captive portal check: http://connectivitycheck.gstatic.com/generate_204
	server.on(context + F("/ncsi.txt"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Microsoft connectivity check: http://www.msftncsi.com/ncsi.txt 
	server.on(context + F("/fwlink"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Microsoft connectivity check
	server.on(context + F("/library/test/success.html"), HTTP_GET, std::bind(&AmsWebServer::redirectToMain, this)); // Apple connectivity check: http://www.apple.com/library/test/success.html
	*/

	server.on("/ssdp/schema.xml", HTTP_GET, std::bind(&AmsWebServer::ssdpSchema, this));

	server.onNotFound(std::bind(&AmsWebServer::notFound, this));
	
	#if defined(ESP32)
	const char * headerkeys[] = {HEADER_AUTHORIZATION, HEADER_ORIGIN, HEADER_REFERER, HEADER_ACCESS_CONTROL_REQUEST_PRIVATE_NETWORK} ;
    server.collectHeaders(headerkeys, 4);
	#else
    server.collectHeaders(HEADER_AUTHORIZATION, HEADER_ORIGIN, HEADER_REFERER, HEADER_ACCESS_CONTROL_REQUEST_PRIVATE_NETWORK);
	#endif
	server.begin(); // Web server start

	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);
	mqttEnabled = strlen(mqttConfig.host) > 0;
}

#if defined(_CLOUDCONNECTOR_H)
void AmsWebServer::setCloud(CloudConnector* cloud) {
	this->cloud = cloud;
}
#endif

void AmsWebServer::setTimezone(Timezone* tz) {
	this->tz = tz;
}

void AmsWebServer::setMqttEnabled(bool enabled) {
	mqttEnabled = enabled;
}
void AmsWebServer::setMqttHandler(AmsMqttHandler* mqttHandler) {
	this->mqttHandler = mqttHandler;
}

void AmsWebServer::setConnectionHandler(ConnectionHandler* ch) {
	this->ch = ch;
}

void AmsWebServer::setPriceService(PriceService* ps) {
	this->ps = ps;
}

void AmsWebServer::setMeterConfig(uint8_t distributionSystem, uint16_t mainFuse, uint16_t productionCapacity) {
	maxPwr = 0;
	this->distributionSystem = distributionSystem;
	this->mainFuse = mainFuse;
	this->productionCapacity = productionCapacity;
}

void AmsWebServer::loop() {
	server.handleClient();

	if(maxPwr == 0 && meterState->getListType() > 1 && mainFuse > 0 && distributionSystem > 0) {
		int voltage = distributionSystem == 2 ? 400 : 230;
		if(meterState->isThreePhase()) {
			maxPwr = mainFuse * sqrt(3) * voltage;
		} else if(meterState->isTwoPhase()) {
			maxPwr = mainFuse * voltage;
		} else {
			maxPwr = mainFuse * 230;
		}
	}
}

bool AmsWebServer::checkSecurity(byte level, bool send401) {
	bool access = WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA || webConfig.security < level;
	if(!access && webConfig.security >= level && server.hasHeader(HEADER_AUTHORIZATION)) {
		String expectedAuth = String(webConfig.username) + ":" + String(webConfig.password);

		String providedPwd = server.header(HEADER_AUTHORIZATION);
		providedPwd.replace(F("Basic "), F(""));

		#if defined(ESP8266)
		String expectedBase64 = base64::encode(expectedAuth, false);
		#elif defined(ESP32)
		String expectedBase64 = base64::encode(expectedAuth);
		#endif

		access = providedPwd.equals(expectedBase64);
		if(!access) {
			#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("Unsuccessful login: '%s'\n"), providedPwd.c_str());
		}
	}

	if(!access && send401) {
		server.sendHeader(HEADER_AUTHENTICATE, AUTHENTICATE_BASIC);
		server.setContentLength(0);
		server.send(401, MIME_HTML, "");
	}
	return access;
}

void AmsWebServer::notFound() {
	#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::WARNING))
#endif
debugger->printf_P(PSTR("URI '%s' was not found\n"), server.uri().c_str());

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.send_P(404, MIME_HTML, PSTR("Not found"));
}

void AmsWebServer::faviconSvg() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1MO);
	server.send_P(200, "image/svg+xml", FAVICON_SVG);
}

void AmsWebServer::logoSvg() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1MO);
	String svg = String(FAVICON_SVG);
	svg.replace("045c7c", "f3f4f6");
	server.send(200, "image/svg+xml", svg.c_str());
}

void AmsWebServer::neasLogoSvg() {
    server.send_P(200, "image/svg+xml", NEAS_LOGOTYPE_WHITE_SVG, NEAS_LOGOTYPE_WHITE_SVG_LEN);
}

void AmsWebServer::wifiHighLightSvg() {
    server.send_P(200, "image/svg+xml", WIFI_HIGH_LIGHT_SVG, WIFI_HIGH_LIGHT_SVG_LEN);
}

void AmsWebServer::wifiMediumLightSvg() {
    server.send_P(200, "image/svg+xml", WIFI_MEDIUM_LIGHT_SVG, WIFI_MEDIUM_LIGHT_SVG_LEN);
}

void AmsWebServer::wifiLowLightSvg() {
    server.send_P(200, "image/svg+xml", WIFI_LOW_LIGHT_SVG, WIFI_LOW_LIGHT_SVG_LEN);
}

void AmsWebServer::wifiOffSvg() {
    server.send_P(200, "image/svg+xml", WIFI_OFF_LIGHT_SVG, WIFI_OFF_LIGHT_SVG_LEN);
}

void AmsWebServer::sysinfoJson() {
	if(!checkSecurity(2, true))
		return;

	SystemConfig sys;
	config->getSystemConfig(sys);

	uint32_t chipId;
	#if defined(ESP32)
		chipId = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
	#else
		chipId = ESP.getChipId();
	#endif
	String chipIdStr = String(chipId, HEX);

	String hostname;
	if(sys.userConfigured) {
		NetworkConfig networkConfig;
		config->getNetworkConfig(networkConfig);
		hostname = String(networkConfig.hostname);
	} else {
		hostname = "ams-"+chipIdStr;
	}

	IPAddress localIp;
	IPAddress subnet;
	IPAddress gateway;
	IPAddress dns1;
	IPAddress dns2;
	#if defined(ESP32)
	IPv6Address ipv6;
	IPv6Address dns1v6;
	IPv6Address dns2v6;
	#endif

	if(ch == NULL) {
		localIp = WiFi.softAPIP();
		subnet = IPAddress(255,255,255,0);
		gateway = WiFi.subnetMask();
		dns1 = WiFi.dnsIP(0);
		dns2 = WiFi.dnsIP(1);
	} else {
		localIp = ch->getIP();
		subnet = ch->getSubnetMask();
		gateway = ch->getGateway();
		dns1 = ch->getDns(0);
		dns2 = ch->getDns(1);
		#if defined(ESP32)
		ipv6 = ch->getIPv6();
		dns1v6 = ch->getDNSv6(0);
		dns2v6 = ch->getDNSv6(1);
		#endif
	}

    char macStr[18] = { 0 };
    char apMacStr[18] = { 0 };

	uint8_t mac[6];
	uint8_t apmac[6];

	#if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
    wifi_get_macaddr(SOFTAP_IF, apmac);
	uint8_t cpu_freq = system_get_cpu_freq();
	#elif defined(ESP32)
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_AP, apmac);
	uint32_t cpu_freq = esp_clk_cpu_freq() / 1000000;
	#endif

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(apMacStr, "%02X:%02X:%02X:%02X:%02X:%02X", apmac[0], apmac[1], apmac[2], apmac[3], apmac[4], apmac[5]);

	UiConfig ui;
	config->getUiConfig(ui);

	UpgradeInformation upinfo;
	updater->getUpgradeInformation(upinfo);

	String meterModel = meterState->getMeterModel();
	if(!meterModel.isEmpty())
		meterModel.replace(F("\\"), F("\\\\"));

	String meterId = meterState->getMeterId();
	if(!meterId.isEmpty())
		meterId.replace(F("\\"), F("\\\\"));

	time_t now = time(nullptr);
	String features = "";
	#if defined(AMS_REMOTE_DEBUG)
	if(!features.isEmpty()) features += ",";
	features += "\"rdebug\"";
	#endif
	#if defined(AMS_KMP)
	if(!features.isEmpty()) features += ",";
	features += "\"kmp\"";
	#endif
	#if defined(AMS_CLOUD)
	if(!features.isEmpty()) features += ",";
	features += "\"cloud\"";
	#endif
	#if defined(ZMART_CHARGE)
	if(!features.isEmpty()) features += ",";
	features += "\"zc\"";
	#endif

	int size = snprintf_P(buf, BufferSize, SYSINFO_JSON,
		FirmwareVersion::VersionString,
		#if defined(CONFIG_IDF_TARGET_ESP32S2)
		"esp32s2",
		#elif defined(CONFIG_IDF_TARGET_ESP32S3)
		"esp32s3",
		#elif defined(CONFIG_IDF_TARGET_ESP32C3)
		"esp32c3",
		#elif defined(CONFIG_FREERTOS_UNICORE)
		"esp32solo",
		#elif defined(ESP32)
		"esp32",
		#elif defined(ESP8266)
		"esp8266",
		#endif
		chipIdStr.c_str(),
		cpu_freq,
		macStr,
		apMacStr,
		sys.boardType,
		sys.vendorConfigured ? "true" : "false",
		sys.userConfigured ? "true" : "false",
		sys.dataCollectionConsent,
		hostname.c_str(),
		performRestart ? "true" : "false",
		updater->getProgress() > 0.0 && upinfo.errorCode == 0 ? "true" : "false",
		#if defined(ESP8266)
		localIp.isSet() ? localIp.toString().c_str() : "",
		subnet.isSet() ? subnet.toString().c_str() : "",
		gateway.isSet() ? gateway.toString().c_str() : "",
		dns1.isSet() ? dns1.toString().c_str() : "",
		dns2.isSet() ? dns2.toString().c_str() : "",
		"",
		"",
		"",
		#else
		localIp != INADDR_NONE ? localIp.toString().c_str() : "",
		subnet != INADDR_NONE ? subnet.toString().c_str() : "",
		gateway != INADDR_NONE ? gateway.toString().c_str() : "",
		dns1 != INADDR_NONE ? dns1.toString().c_str() : "",
		dns2 != INADDR_NONE ? dns2.toString().c_str() : "",
		ipv6 == IPv6Address() ? "" : ipv6.toString().c_str(),
		dns1v6 == IPv6Address() ? "" : dns1v6.toString().c_str(),
		dns2v6 == IPv6Address() ? "" : dns2v6.toString().c_str(),
		#endif
		sys.boardType > 240 && sys.boardType < 250 ? "true" : "false",
		meterState->getMeterType(),
		meterModel.c_str(),
		meterId.c_str(),
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
		ui.language,
		webConfig.security,
		webConfig.context,
		#if defined(ESP32)
		rtc_get_reset_reason(0),
		rdc->last_cause,
		#else
		ESP.getResetInfoPtr()->reason,
		ESP.getResetInfoPtr()->exccause,
		#endif
		upinfo.errorCode,
		upinfo.fromVersion,
		upinfo.toVersion,
		updater->getNextVersion(),
		updater->getProgress(),
		ea->getUseLastMonth(),
		ea->getCostLastMonth(),
		ea->getProducedLastMonth(),
		ea->getIncomeLastMonth(),
		(uint16_t) (tz == NULL ? 0 : (tz->toLocal(now)-now)/3600),
		features.c_str()
	);

	stripNonAscii((uint8_t*) buf, size+1);

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	if(performRestart) {
		server.handleClient();
		delay(250);

		if(ds != NULL) {
			ds->save();
		}
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::INFO))
		#endif
		debugger->printf_P(PSTR("Rebooting\n"));
		debugger->flush();
		delay(1000);
		rdc->cause = 1;
		ESP.restart();
		performRestart = false;
	}
}

void AmsWebServer::dataJson() {
	uint64_t millis = millis64();

	if(!checkSecurity(2, true))
		return;

	float vcc = hw->getVcc();
	int rssi = hw->getWifiRssi();

	uint8_t espStatus;
	#if defined(ESP8266)
	if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
		espStatus = 1;
	} else if(vcc > 2.8 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 2.7 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#elif defined(ESP32)
	if(vcc < 2.0) { // Voltage not correct, ESP would not run on this voltage
		espStatus = 1;
	} else if(vcc > 3.1 && vcc < 3.5) {
		espStatus = 1;
	} else if(vcc > 3.0 && vcc < 3.6) {
		espStatus = 2;
	} else {
		espStatus = 3;
	}
	#endif

	uint8_t hanStatus;
	if(meterState->getLastError() != 0) {
		hanStatus = 3;
	} else if(meterState->getLastUpdateMillis() == 0 && millis < 30000) {
		hanStatus = 0;
	} else if(millis - meterState->getLastUpdateMillis() < 15000) {
		hanStatus = 1;
	} else if(millis - meterState->getLastUpdateMillis() < 30000) {
		hanStatus = 2;
	} else {
		hanStatus = 3;
	}

	uint8_t wifiStatus;
	if(rssi > -75) {
		wifiStatus = 1;
	} else if(rssi > -95) {
		wifiStatus = 2;
	} else {
		wifiStatus = 3;
	}

	uint8_t mqttStatus;
	if(!mqttEnabled) {
		mqttStatus = 0;
	} else if(mqttHandler != NULL && mqttHandler->connected()) {
		mqttStatus = 1;
	} else if(mqttHandler == NULL || mqttHandler->lastError() == 0) {
		mqttStatus = 2;
	} else {
		mqttStatus = 3;
	}

	float price = ea->getPriceForHour(PRICE_DIRECTION_IMPORT, 0);
	float exportPrice = ea->getPriceForHour(PRICE_DIRECTION_EXPORT, 0);

	String peaks = "";
	for(uint8_t i = 1; i <= ea->getConfig()->hours; i++) {
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(ea->getPeak(i).value / 100.0);
	}

	time_t now = time(nullptr);

	snprintf_P(buf, BufferSize, DATA_JSON,
		maxPwr == 0 ? meterState->isThreePhase() ? 20000 : 10000 : maxPwr,
		productionCapacity,
		mainFuse == 0 ? 40 : mainFuse,
		meterState->getActiveImportPower(),
		meterState->getActiveExportPower(),
		((int32_t) meterState->getActiveImportPower()) - meterState->getActiveExportPower(),
		meterState->getReactiveImportPower(),
		meterState->getReactiveExportPower(),
		meterState->getActiveImportCounter(),
		meterState->getActiveExportCounter(),
		meterState->getReactiveImportCounter(),
		meterState->getReactiveExportCounter(),
		meterState->getPowerFactor(),

		meterState->getL1Voltage(),
		meterState->getL1Current(),
		meterState->getL1ActiveImportPower(),
		meterState->getL1ActiveExportPower(),
		meterState->getL1PowerFactor(),

		meterState->getL2Voltage(),
		meterState->getL2Current(),
		meterState->getL2ActiveImportPower(),
		meterState->getL2ActiveExportPower(),
		meterState->getL2PowerFactor(),
		meterState->isL2currentMissing() ? "true" : "false",

		meterState->getL3Voltage(),
		meterState->getL3Current(),
		meterState->getL3ActiveImportPower(),
		meterState->getL3ActiveExportPower(),
		meterState->getL3PowerFactor(),

		vcc,
		rssi,
		hw->getTemperature(),
		(uint32_t) (millis / 1000),
		ESP.getFreeHeap(),
		espStatus,
		hanStatus,
		wifiStatus,
		mqttStatus,
		mqttHandler == NULL ? 0 : (int) mqttHandler->lastError(),
		price == PRICE_NO_VALUE ? "null" : String(price, abs(price) > 1.0 ? 2 : 4).c_str(),
		exportPrice == PRICE_NO_VALUE ? "null" : String(exportPrice, abs(exportPrice) > 1.0 ? 2 : 4).c_str(),
		meterState->getMeterType(),
		distributionSystem,
		ea->getMonthMax(),
		peaks.c_str(),
		ea->getCurrentThreshold(),
		ea->getUseThisHour(),
		ea->getCostThisHour(),
		ea->getProducedThisHour(),
		ea->getIncomeThisHour(),
		ea->getUseToday(),
		ea->getCostToday(),
		ea->getProducedToday(),
		ea->getIncomeToday(),
		ea->getUseThisMonth(),
		ea->getCostThisMonth(),
		ea->getProducedThisMonth(),
		ea->getIncomeThisMonth(),
		price == PRICE_NO_VALUE ? "false" : "true",
		priceRegion.c_str(),
		priceCurrency.c_str(),
		meterState->getLastError(),
		ps == NULL ? 0 : ps->getLastError(),
		(uint32_t) now,
		checkSecurity(1, false) ? "true" : "false"
	);

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::dayplotJson() {
	if(!checkSecurity(2))
		return;

	if(ds == NULL) {
		notFound();
	} else {
		uint16_t pos = snprintf_P(buf, BufferSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 0; i < 24; i++) {
			pos += snprintf_P(buf+pos, BufferSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getHourImport(i) / 1000.0, i, ds->getHourExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, BufferSize-pos, PSTR("}"));

		addConditionalCloudHeaders();
		server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
		server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
		server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

		server.setContentLength(strlen(buf));
		server.send(200, MIME_JSON, buf);
	}
}

void AmsWebServer::monthplotJson() {
	if(!checkSecurity(2))
		return;

	if(ds == NULL) {
		notFound();
	} else {
		uint16_t pos = snprintf_P(buf, BufferSize, PSTR("{\"unit\":\"kwh\""));
		for(uint8_t i = 1; i < 32; i++) {
			pos += snprintf_P(buf+pos, BufferSize-pos, PSTR(",\"i%02d\":%.3f,\"e%02d\":%.3f"), i, ds->getDayImport(i) / 1000.0, i, ds->getDayExport(i) / 1000.0);
		}
		snprintf_P(buf+pos, BufferSize-pos, PSTR("}"));

		addConditionalCloudHeaders();
		server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
		server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
		server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

		server.setContentLength(strlen(buf));
		server.send(200, MIME_JSON, buf);
	}
}

void AmsWebServer::energyPriceJson() {
	if(!checkSecurity(2))
		return;

	float prices[36];
	for(int i = 0; i < 36; i++) {
		prices[i] = ps == NULL ? PRICE_NO_VALUE : ps->getValueForHour(PRICE_DIRECTION_IMPORT, i);
	}

	uint16_t pos = snprintf_P(buf, BufferSize, PSTR("{\"currency\":\"%s\",\"source\":\"%s\""),
		ps == NULL ? "" : ps->getCurrency(),
		ps == NULL ? "" : ps->getSource()
	);

    for(uint8_t i = 0;i < 36; i++) {
        if(prices[i] == PRICE_NO_VALUE) {
            pos += snprintf_P(buf+pos, BufferSize-pos, PSTR(",\"%02d\":null"), i);
        } else {
            pos += snprintf_P(buf+pos, BufferSize-pos, PSTR(",\"%02d\":%.4f"), i, prices[i]);
        }
    }
	snprintf_P(buf+pos, BufferSize-pos, PSTR("}"));

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::temperatureJson() {
	if(!checkSecurity(2))
		return;

	int count = hw->getTempSensorCount();
	snprintf_P(buf, 16, PSTR("{\"c\":%d,\"s\":["), count);

	for(int i = 0; i < count; i++) {
		TempSensorData* data = hw->getTempSensorData(i);
		if(data == NULL) continue;

		char* pos = buf+strlen(buf);
		snprintf_P(pos, 72, PSTR("{\"i\":%d,\"a\":\"%s\",\"n\":\"%s\",\"c\":%d,\"v\":%.1f},"), 
			i,
			toHex(data->address, 8).c_str(),
			"",
			1,
			data->lastRead
		);
		yield();
	}
	char* pos = buf+strlen(buf);
	snprintf_P(count == 0 ? pos : pos-1, 8, PSTR("]}"));

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::indexHtml() {
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	if(!checkSecurity(2))
		return;

	String context;
	config->getWebConfig(webConfig);
	stripNonAscii((uint8_t*) webConfig.context, 32);
	if(strlen(webConfig.context) > 0) {
		context = String(webConfig.context);
		context.replace(" ", "");
		if(!context.isEmpty()) {
			context = "/" + context + "/";
		}
	}

	if(context.isEmpty()) {
		server.setContentLength(INDEX_HTML_LEN);
		server.send_P(200, MIME_HTML, INDEX_HTML);
	} else {
		String body = String(INDEX_HTML);
		body.replace("href=\"/\"", "href=\"" + context + "\"");
		server.setContentLength(body.length());
		server.send(200, MIME_HTML, body);
	}

}

void AmsWebServer::indexCss() {
	if(!checkSecurity(2))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1YR);
	server.sendHeader(HEADER_CONTENT_ENCODING, CONTENT_ENCODING_GZIP);
	server.send_P(200, MIME_CSS, INDEX_CSS, INDEX_CSS_LEN);
}

void AmsWebServer::indexJs() {
	if(!checkSecurity(2))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1YR);
	server.sendHeader(HEADER_CONTENT_ENCODING, CONTENT_ENCODING_GZIP);
	server.send_P(200, MIME_JS, INDEX_JS, INDEX_JS_LEN);
}

void AmsWebServer::configurationJson() {
	if(!checkSecurity(1))
		return;
		

	MeterConfig meterConfig;
	config->getMeterConfig(meterConfig);
	
	bool multEnable = false;
	if(meterConfig.wattageMultiplier != 1.0 && meterConfig.wattageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.voltageMultiplier != 1.0 && meterConfig.voltageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.amperageMultiplier != 1.0 && meterConfig.amperageMultiplier != 0.0)
		multEnable = true;
	if(meterConfig.accumulatedMultiplier != 1.0 && meterConfig.accumulatedMultiplier != 0.0)
		multEnable = true;

	SystemConfig sysConfig;
	config->getSystemConfig(sysConfig);
	NtpConfig ntpConfig;
	config->getNtpConfig(ntpConfig);
	NetworkConfig networkConfig;
	config->getNetworkConfig(networkConfig);

	bool encen = false;
	for(uint8_t i = 0; i < 16; i++) {
		if(meterConfig.encryptionKey[i] > 0) {
			encen = true;
		}
	}

	EnergyAccountingConfig* eac = ea->getConfig();
	MqttConfig mqttConfig;
	config->getMqttConfig(mqttConfig);

	PriceServiceConfig price;
	config->getPriceServiceConfig(price);
	DebugConfig debugConfig;
	config->getDebugConfig(debugConfig);
	DomoticzConfig domo;
	config->getDomoticzConfig(domo);
	UiConfig ui;
	config->getUiConfig(ui);
	HomeAssistantConfig haconf;
	config->getHomeAssistantConfig(haconf);
	CloudConfig cloud;
	config->getCloudConfig(cloud);
	ZmartChargeConfig zcc;
	config->getZmartChargeConfig(zcc);
	stripNonAscii((uint8_t*) zcc.token, 21);

	bool qsc = false;
	bool qsr = false;
	bool qsk = false;

	if(LittleFS.begin()) {
		qsc = LittleFS.exists(FILE_MQTT_CA);
		qsr = LittleFS.exists(FILE_MQTT_CERT);
		qsk = LittleFS.exists(FILE_MQTT_KEY);
	}

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send_P(200, MIME_JSON, PSTR("{\"version\":\""));
	server.sendContent_P(FirmwareVersion::VersionString);
	server.sendContent_P(PSTR("\","));
	snprintf_P(buf, BufferSize, CONF_GENERAL_JSON,
		ntpConfig.timezone,
		networkConfig.hostname,
		webConfig.security,
		webConfig.username,
		strlen(webConfig.password) > 0 ? "***" : "",
		webConfig.context
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_METER_JSON,
		meterConfig.source,
		meterConfig.parser,
		meterConfig.baud,
		meterConfig.parity,
		meterConfig.invert ? "true" : "false",
		meterConfig.bufferSize * 64,
		meterConfig.distributionSystem,
		meterConfig.mainFuse,
		meterConfig.productionCapacity,
		encen ? "true" : "false",
		toHex(meterConfig.encryptionKey, 16).c_str(),
		toHex(meterConfig.authenticationKey, 16).c_str(),
		multEnable ? "true" : "false",
		meterConfig.wattageMultiplier == 0.0 ? 1.0 : meterConfig.wattageMultiplier / 1000.0,
		meterConfig.voltageMultiplier == 0.0 ? 1.0 : meterConfig.voltageMultiplier / 1000.0,
		meterConfig.amperageMultiplier == 0.0 ? 1.0 : meterConfig.amperageMultiplier / 1000.0,
		meterConfig.accumulatedMultiplier == 0.0 ? 1.0 : meterConfig.accumulatedMultiplier / 1000.0
	);
	server.sendContent(buf);

	snprintf_P(buf, BufferSize, CONF_THRESHOLDS_JSON,
		eac->thresholds[0],
		eac->thresholds[1],
		eac->thresholds[2],
		eac->thresholds[3],
		eac->thresholds[4],
		eac->thresholds[5],
		eac->thresholds[6],
		eac->thresholds[7],
		eac->thresholds[8],
		eac->thresholds[9],
		eac->hours
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_WIFI_JSON,
		networkConfig.ssid,
		strlen(networkConfig.psk) > 0 ? "***" : "",
		networkConfig.power / 10.0,
		networkConfig.sleep,
		networkConfig.use11b ? "true" : "false"
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_NET_JSON,
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
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_MQTT_JSON,
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
		mqttConfig.keepalive
	);
	server.sendContent(buf);

	snprintf_P(buf, BufferSize, CONF_PRICE_JSON,
		price.enabled ? "true" : "false",
		price.entsoeToken,
		price.area,
		price.currency
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_DEBUG_JSON,
		debugConfig.serial ? "true" : "false",
		debugConfig.telnet ? "true" : "false",
		debugConfig.level
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_GPIO_JSON,
		meterConfig.rxPin == 0xff ? "null" : String(meterConfig.rxPin, 10).c_str(),
		meterConfig.rxPinPullup ? "true" : "false",
		meterConfig.txPin == 0xff ? "null" : String(meterConfig.txPin, 10).c_str(),
		gpioConfig->apPin == 0xff ? "null" : String(gpioConfig->apPin, 10).c_str(),
		gpioConfig->ledPin == 0xff ? "null" : String(gpioConfig->ledPin, 10).c_str(),
		gpioConfig->ledInverted ? "true" : "false",
		gpioConfig->ledPinRed == 0xff ? "null" : String(gpioConfig->ledPinRed, 10).c_str(),
		gpioConfig->ledPinGreen == 0xff ? "null" : String(gpioConfig->ledPinGreen, 10).c_str(),
		gpioConfig->ledPinBlue == 0xff ? "null" : String(gpioConfig->ledPinBlue, 10).c_str(),
		gpioConfig->ledRgbInverted ? "true" : "false",
		gpioConfig->ledDisablePin == 0xff ? "null" : String(gpioConfig->ledDisablePin, 10).c_str(),
		gpioConfig->ledBehaviour,
		gpioConfig->tempSensorPin == 0xff ? "null" : String(gpioConfig->tempSensorPin, 10).c_str(),
		gpioConfig->tempAnalogSensorPin == 0xff ? "null" : String(gpioConfig->tempAnalogSensorPin, 10).c_str(),
		gpioConfig->vccPin == 0xff ? "null" : String(gpioConfig->vccPin, 10).c_str(),
		gpioConfig->vccOffset / 100.0,
		gpioConfig->vccMultiplier / 1000.0,
		gpioConfig->vccResistorVcc,
		gpioConfig->vccResistorGnd,
		gpioConfig->vccBootLimit / 10.0
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_UI_JSON,
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
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_DOMOTICZ_JSON,
		domo.elidx,
		domo.cl1idx,
		domo.vl1idx,
		domo.vl2idx,
		domo.vl3idx
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_HA_JSON,
		haconf.discoveryPrefix,
		haconf.discoveryHostname,
		haconf.discoveryNameTag
	);
	server.sendContent(buf);
	snprintf_P(buf, BufferSize, CONF_CLOUD_JSON,
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
	server.sendContent(buf);
	server.sendContent_P(PSTR("}"));
}

void AmsWebServer::priceConfigJson() {
	if(!checkSecurity(1))
		return;

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send_P(200, MIME_JSON, PSTR("{\"o\":["));
	if(ps != NULL) {
		std::vector<PriceConfig> pc = ps->getPriceConfig();
		if(pc.size() > 0) {
			for(uint8_t i = 0; i < pc.size(); i++) {
				PriceConfig& p = pc.at(i);

				String days;
				for(uint8_t d = 0; d < 7; d++) {
					if((p.days >> d) & 0x1 == 0x1) {
						days += String(d, 10) + ",";
					}
				}
				days = days.substring(0, days.length()-1);

				String hours;
				for(uint8_t h = 0; h < 24; h++) {
					if((p.hours >> h) & 0x1 == 0x1) {
						hours += String(h, 10) + ",";
					}
				}
				hours = hours.substring(0, hours.length()-1);

				snprintf_P(buf, BufferSize, CONF_PRICE_ROW_JSON,
					p.type,
					p.name,
					p.direction,
					days.c_str(),
					hours.c_str(),
					p.value / 10000.0,
					p.start_month,
					p.start_dayofmonth,
					p.end_month,
					p.end_dayofmonth,
					i == pc.size()-1 ? "" : ","
				);
				server.sendContent(buf);
			}
		}
	}
	snprintf_P(buf, BufferSize, PSTR("]}"));
	server.sendContent(buf);
}

void AmsWebServer::translationsJson() {
	if(!LittleFS.begin()) {
		server.send_P(500, MIME_PLAIN, PSTR("500: Filesystem unavailable"));
		return;
	}

	String lang = server.arg("lang");
	if(lang.isEmpty()) {
		UiConfig ui;
		if(config->getUiConfig(ui)) {
			lang = String(ui.language);
		}
	}

	snprintf_P(buf, BufferSize, PSTR("/translations-%s.json"), lang.c_str());
	if(!LittleFS.exists(buf)) {
		notFound();
		return;
	}

	addConditionalCloudHeaders();
//	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_1DA);
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	File file = LittleFS.open(buf, "r");
	server.setContentLength(file.size());

	server.send(200, MIME_JSON);
	while(file.available() > 0) {
		int len = file.readBytes(buf, BufferSize);
		server.sendContent(buf, len);
	}
	file.close();
}

void AmsWebServer::cloudkeyJson() {
	if(!checkSecurity(1))
		return;
	#if defined(_CLOUDCONNECTOR_H)
		if(cloud == NULL)
			notFound();

		String seed = cloud->generateSeed();

		snprintf_P(buf, BufferSize, PSTR("{\"seed\":\"%s\"}"), seed.c_str());

		server.setContentLength(strlen(buf));
		server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
		server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
		server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
		server.send(200, MIME_JSON, buf);
	#else
		notFound();
	#endif
}

void AmsWebServer::handleSave() {
	if(!checkSecurity(1))
		return;

	SystemConfig sys;
	config->getSystemConfig(sys);

	bool success = true;
	if(server.hasArg(F("v")) && server.arg(F("v")) == F("true")) {
		int boardType = server.arg(F("vb")).toInt();
		int hanPin = server.arg(F("vh")).toInt();

		MeterConfig meterConfig;
		config->getMeterConfig(meterConfig);
		config->clearGpio(*gpioConfig);
		hw->applyBoardConfig(boardType, *gpioConfig, meterConfig, hanPin);
		if(success) {
			config->setGpioConfig(*gpioConfig);
			config->setMeterConfig(meterConfig);

			sys.boardType = success ? boardType : 0xFF;
			sys.vendorConfigured = success;
			config->setSystemConfig(sys);
		}
	}

	if(server.hasArg(F("s")) && server.arg(F("s")) == F("true")) {
		MeterConfig meterConfig;
		config->getMeterConfig(meterConfig);

		config->clear();

		NetworkConfig network;
		config->clearNetworkConfig(network);

		strcpy(network.ssid, server.arg(F("ss")).c_str());

		String psk = server.arg(F("sp"));
		if(!psk.equals("***")) {
			strcpy(network.psk, psk.c_str());
		}
		network.mode = server.arg(F("sc")).toInt();
		if(network.mode > 3 || network.mode == 0) network.mode = 1; // WiFi Client

		if(network.mode == 3 || strlen(network.ssid) > 0) {
			if(server.hasArg(F("sm")) && server.arg(F("sm")) == "static") {
				strcpy(network.ip, server.arg(F("si")).c_str());
				strcpy(network.gateway, server.arg(F("sg")).c_str());
				strcpy(network.subnet, server.arg(F("su")).c_str());
				strcpy(network.dns1, server.arg(F("sd")).c_str());
			}

			if(server.hasArg(F("sh")) && !server.arg(F("sh")).isEmpty()) {
				strcpy(network.hostname, server.arg(F("sh")).c_str());
				network.mdns = true;
			} else {
				network.mdns = false;
			}
			
			switch(sys.boardType) {
				case 6: // Pow-P1
					meterConfig.baud = 115200;
					meterConfig.parity = 3; // 8N1
					meterConfig.bufferSize = 8;
					break;
				case 3: // Pow-K UART0
				case 5: // Pow-K+
				case 2: // spenceme
				case 8: // dbeinder: HAN mosquito
				case 50: // Generic ESP32-S2
				case 51: // Wemos S2 mini
				case 70: // Generic ESP32-C3
				case 71: // ESP32-C3-DevKitM-1
				case 80: // Generic ESP32-S3
					network.sleep = 1; // Modem sleep
					break;
				case 4: // Pow-U UART0
				case 7: // Pow-U+
					network.sleep = 2; // Light sleep
					break;
			}
			#if defined(ESP8266)
				meterConfig.bufferSize = 1;
			#endif
			config->setNetworkConfig(network);
			config->setMeterConfig(meterConfig);
			
			sys.userConfigured = success;
			sys.dataCollectionConsent = 0;
			config->setSystemConfig(sys);

			performRestart = true;
		}
	} else if(server.hasArg(F("sf")) && !server.arg(F("sf")).isEmpty()) {
		sys.dataCollectionConsent = server.hasArg(F("sf")) && (server.arg(F("sf")) == F("true") || server.arg(F("sf")) == F("1")) ? 1 : 2;
		config->setSystemConfig(sys);
	}

	if(server.hasArg(F("m")) && server.arg(F("m")) == F("true")) {
		MeterConfig meterConfig;
		config->getMeterConfig(meterConfig);
		meterConfig.source = server.arg(F("mo")).toInt();
		meterConfig.parser = server.arg(F("ma")).toInt();
		meterConfig.baud = server.arg(F("mb")).toInt();
		meterConfig.parity = server.arg(F("mp")).toInt();
		meterConfig.invert = server.hasArg(F("mi")) && server.arg(F("mi")) == F("true");
		meterConfig.distributionSystem = server.arg(F("md")).toInt();
		meterConfig.mainFuse = server.arg(F("mf")).toInt();
		meterConfig.productionCapacity = server.arg(F("mr")).toInt();
		meterConfig.bufferSize = min((double) 64, ceil((server.arg(F("ms")).toInt()) / 64));
		maxPwr = 0;

		if(server.hasArg(F("me")) && server.arg(F("me")) == F("true")) {
			String encryptionKeyHex = server.arg(F("mek"));
			if(!encryptionKeyHex.isEmpty()) {
				encryptionKeyHex.replace(F("0x"), F(""));
				fromHex(meterConfig.encryptionKey, encryptionKeyHex, 16);
			} else {
				memset(meterConfig.encryptionKey, 0, 16);
			}

			String authenticationKeyHex = server.arg(F("mea"));
			if(!authenticationKeyHex.isEmpty()) {
				authenticationKeyHex.replace(F("0x"), F(""));
				fromHex(meterConfig.authenticationKey, authenticationKeyHex, 16);
			} else {
				memset(meterConfig.authenticationKey, 0, 16);
			}
		} else {
			memset(meterConfig.encryptionKey, 0, 16);
			memset(meterConfig.authenticationKey, 0, 16);
		}

		meterConfig.wattageMultiplier = server.arg(F("mmw")).toFloat() * 1000;
		meterConfig.voltageMultiplier = server.arg(F("mmv")).toFloat() * 1000;
		meterConfig.amperageMultiplier = server.arg(F("mma")).toFloat() * 1000;
		meterConfig.accumulatedMultiplier = server.arg(F("mmc")).toFloat() * 1000;
		config->setMeterConfig(meterConfig);
	}

	if(server.hasArg(F("w")) && server.arg(F("w")) == F("true")) {
		long mode = server.arg(F("nc")).toInt();
		if(mode > 0 && mode < 3) {
			NetworkConfig network;
			config->getNetworkConfig(network);
			network.mode = mode;
			strcpy(network.ssid, server.arg(F("ws")).c_str());
			String psk = server.arg(F("wp"));
			if(!psk.equals("***")) {
				strcpy(network.psk, psk.c_str());
			}
			network.power = server.arg(F("ww")).toFloat() * 10;
			network.sleep = server.arg(F("wz")).toInt();
			network.use11b = server.hasArg(F("wb")) && server.arg(F("wb")) == F("true");

			if(server.hasArg(F("nm"))) {
				if(server.arg(F("nm")) == "static") {
					strcpy(network.ip, server.arg(F("ni")).c_str());
					strcpy(network.gateway, server.arg(F("ng")).c_str());
					strcpy(network.subnet, server.arg(F("ns")).c_str());
					strcpy(network.dns1, server.arg(F("nd1")).c_str());
					strcpy(network.dns2, server.arg(F("nd2")).c_str());
				} else if(server.arg(F("nm")) == "dhcp") {
					strcpy(network.ip, "");
					strcpy(network.gateway, "");
					strcpy(network.subnet, "");
					strcpy(network.dns1, "");
					strcpy(network.dns2, "");
				}
			}
			network.ipv6 = server.hasArg(F("nx")) && server.arg(F("nx")) == F("true");
			network.mdns = server.hasArg(F("nd")) && server.arg(F("nd")) == F("true");
			config->setNetworkConfig(network);
		} 

	}

	if(server.hasArg(F("ntp")) && server.arg(F("ntp")) == F("true")) {
		NtpConfig ntp;
		config->getNtpConfig(ntp);
		ntp.enable = true;
		ntp.dhcp = server.hasArg(F("ntpd")) && server.arg(F("ntpd")) == F("true");
		strcpy(ntp.server, server.arg(F("ntph")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("q")) && server.arg(F("q")) == F("true")) {
		MqttConfig mqtt;
		config->getMqttConfig(mqtt);
		if(server.hasArg(F("qh")) && !server.arg(F("qh")).isEmpty()) {
			strcpy(mqtt.host, server.arg(F("qh")).c_str());
			strcpy(mqtt.clientId, server.arg(F("qc")).c_str());
			strcpy(mqtt.publishTopic, server.arg(F("qb")).c_str());
			strcpy(mqtt.subscribeTopic, server.arg(F("qr")).c_str());
			strcpy(mqtt.username, server.arg(F("qu")).c_str());
			String pass = server.arg(F("qa"));
			if(!pass.equals("***")) {
				strcpy(mqtt.password, pass.c_str());
			}
			mqtt.payloadFormat = server.arg(F("qm")).toInt();
			#if defined(ESP8266)
			mqtt.ssl = false;
			#else
			mqtt.ssl = server.arg(F("qs")) == F("true");
			#endif

			mqtt.port = server.arg(F("qp")).toInt();
			if(mqtt.port == 0) {
				mqtt.port = mqtt.ssl ? 8883 : 1883;
			}

			mqtt.stateUpdate = server.arg(F("qt")).toInt() == 1;
			mqtt.stateUpdateInterval = server.arg(F("qd")).toInt();
			mqtt.timeout = server.arg(F("qi")).toInt();
			mqtt.keepalive = server.arg(F("qk")).toInt();
		} else {
			config->clearMqtt(mqtt);
		}
		config->setMqttConfig(mqtt);
	}

	if(server.hasArg(F("o")) && server.arg(F("o")) == F("true")) {
		DomoticzConfig domo {
			static_cast<uint16_t>(server.arg(F("oe")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou1")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou2")).toInt()),
			static_cast<uint16_t>(server.arg(F("ou3")).toInt()),
			static_cast<uint16_t>(server.arg(F("oc")).toInt())
		};
		config->setDomoticzConfig(domo);
	}

	if(server.hasArg(F("h")) && server.arg(F("h")) == F("true")) {
		HomeAssistantConfig haconf;
		config->getHomeAssistantConfig(haconf);
		strcpy(haconf.discoveryPrefix, server.arg(F("ht")).c_str());
		strcpy(haconf.discoveryHostname, server.arg(F("hh")).c_str());
		strcpy(haconf.discoveryNameTag, server.arg(F("hn")).c_str());
		config->setHomeAssistantConfig(haconf);
	}

	if(server.hasArg(F("g")) && server.arg(F("g")) == F("true")) {
		webConfig.security = server.arg(F("gs")).toInt();
		if(webConfig.security > 0) {
			strcpy(webConfig.username, server.arg(F("gu")).c_str());
			String pass = server.arg(F("gp"));
			if(!pass.equals("***")) {
				strcpy(webConfig.password, pass.c_str());
			}
			#if defined(AMS_REMOTE_DEBUG)
			debugger->setPassword(webConfig.password);
			#endif
		} else {
			memset(webConfig.username, 0, 37);
			memset(webConfig.password, 0, 37);
			#if defined(AMS_REMOTE_DEBUG)
			debugger->setPassword(F(""));
			#endif
		}
		strcpy(webConfig.context, server.arg(F("gc")).c_str());
		config->setWebConfig(webConfig);

		NetworkConfig network;
		config->getNetworkConfig(network);
		if(server.hasArg(F("gh")) && !server.arg(F("gh")).isEmpty()) {
			strcpy(network.hostname, server.arg(F("gh")).c_str());
		}
		config->setNetworkConfig(network);

		NtpConfig ntp;
		config->getNtpConfig(ntp);
		strcpy(ntp.timezone, server.arg(F("gt")).c_str());
		config->setNtpConfig(ntp);
	}

	if(server.hasArg(F("i")) && server.arg(F("i")) == F("true")) {
		MeterConfig meterConfig;
		config->getMeterConfig(meterConfig);
		meterConfig.rxPin = server.hasArg(F("ihp")) && !server.arg(F("ihp")).isEmpty() ? server.arg(F("ihp")).toInt() : 3;
		meterConfig.rxPinPullup = server.hasArg(F("ihu")) && server.arg(F("ihu")) == F("true");
		meterConfig.txPin = server.hasArg(F("iht")) && !server.arg(F("iht")).isEmpty() ? server.arg(F("iht")).toInt() : 1;
		config->setMeterConfig(meterConfig);

		gpioConfig->ledPin = server.hasArg(F("ilp")) && !server.arg(F("ilp")).isEmpty() ? server.arg(F("ilp")).toInt() : 0xFF;
		gpioConfig->ledInverted = server.hasArg(F("ili")) && server.arg(F("ili")) == F("true");
		gpioConfig->ledPinRed = server.hasArg(F("irr")) && !server.arg(F("irr")).isEmpty() ? server.arg(F("irr")).toInt() : 0xFF;
		gpioConfig->ledPinGreen = server.hasArg(F("irg")) && !server.arg(F("irg")).isEmpty() ? server.arg(F("irg")).toInt() : 0xFF;
		gpioConfig->ledPinBlue = server.hasArg(F("irb")) && !server.arg(F("irb")).isEmpty() ? server.arg(F("irb")).toInt() : 0xFF;
		gpioConfig->ledRgbInverted = server.hasArg(F("iri")) && server.arg(F("iri")) == F("true");
		gpioConfig->apPin = server.hasArg(F("ia")) && !server.arg(F("ia")).isEmpty() ? server.arg(F("ia")).toInt() : 0xFF;
		gpioConfig->tempSensorPin = server.hasArg(F("itd")) && !server.arg(F("itd")).isEmpty() ?server.arg(F("itd")).toInt() : 0xFF;
		gpioConfig->tempAnalogSensorPin = server.hasArg(F("ita")) && !server.arg(F("ita")).isEmpty() ?server.arg(F("ita")).toInt() : 0xFF;
		gpioConfig->vccPin = server.hasArg(F("ivp")) && !server.arg(F("ivp")).isEmpty() ? server.arg(F("ivp")).toInt() : 0xFF;
		gpioConfig->vccResistorGnd = server.hasArg(F("ivdg")) && !server.arg(F("ivdg")).isEmpty() ? server.arg(F("ivdg")).toInt() : 0;
		gpioConfig->vccResistorVcc = server.hasArg(F("ivdv")) && !server.arg(F("ivdv")).isEmpty() ? server.arg(F("ivdv")).toInt() : 0;
		gpioConfig->ledDisablePin =  server.hasArg(F("idd")) && !server.arg(F("idd")).isEmpty() ? server.arg(F("idd")).toInt() : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("idb"))) {
		gpioConfig->ledBehaviour =  server.hasArg(F("idb")) && !server.arg(F("idb")).isEmpty() ? server.arg(F("idb")).toInt() : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("iv")) && server.arg(F("iv")) == F("true")) {
		gpioConfig->vccOffset = server.hasArg(F("ivo")) && !server.arg(F("ivo")).isEmpty() ? server.arg(F("ivo")).toFloat() * 100 : 0;
		gpioConfig->vccMultiplier = server.hasArg(F("ivm")) && !server.arg(F("ivm")).isEmpty() ? server.arg(F("ivm")).toFloat() * 1000 : 1000;
		gpioConfig->vccBootLimit = server.hasArg(F("ivb")) && !server.arg(F("ivb")).isEmpty() ? server.arg(F("ivb")).toFloat() * 10 : 0;
		config->setGpioConfig(*gpioConfig);
	}

	if(server.hasArg(F("d")) && server.arg(F("d")) == F("true")) {
		DebugConfig debug;
		config->getDebugConfig(debug);
		bool active = debug.serial || debug.telnet;

		debug.telnet = server.hasArg(F("dt")) && server.arg(F("dt")) == F("true");
		debug.serial = server.hasArg(F("ds")) && server.arg(F("ds")) == F("true");
		debug.level = server.arg(F("dl")).toInt();

		#if defined(AMS_REMOTE_DEBUG)
		if(debug.telnet || debug.serial) {
			if(webConfig.security > 0) {
				debugger->setPassword(webConfig.password);
			} else {
				debugger->setPassword(F(""));
			}
			debugger->setSerialEnabled(debug.serial);
			NetworkConfig network;
			if(config->getNetworkConfig(network) && strlen(network.hostname) > 0) {
				debugger->begin(network.hostname, (uint8_t) debug.level);
				if(!debug.telnet) {
					debugger->stop();
				}
			}
		} else if(active) {
			performRestart = true;
		}
		#endif
		config->setDebugConfig(debug);
	}

	if(server.hasArg(F("u")) && server.arg(F("u")) == F("true")) {
		UiConfig ui;
		config->getUiConfig(ui);
		ui.showImport = server.arg(F("ui")).toInt();
		ui.showExport = server.arg(F("ue")).toInt();
		ui.showVoltage = server.arg(F("uv")).toInt();
		ui.showAmperage = server.arg(F("ua")).toInt();
		ui.showReactive = server.arg(F("ur")).toInt();
		ui.showRealtime = server.arg(F("uc")).toInt();
		ui.showPeaks = server.arg(F("ut")).toInt();
		ui.showPricePlot = server.arg(F("up")).toInt();
		ui.showDayPlot = server.arg(F("ud")).toInt();
		ui.showMonthPlot = server.arg(F("um")).toInt();
		ui.showTemperaturePlot = server.arg(F("us")).toInt();
		ui.showRealtimePlot = server.arg(F("ul")).toInt();
		ui.showPerPhasePower = server.arg(F("uh")).toInt();
		ui.showPowerFactor = server.arg(F("uf")).toInt();
		ui.darkMode = server.arg(F("uk")).toInt();
		strcpy(ui.language, server.arg(F("ulang")).c_str());
		config->setUiConfig(ui);
	}

	if(server.hasArg(F("p")) && server.arg(F("p")) == F("true")) {
		priceRegion = server.arg(F("pr"));

		PriceServiceConfig price;
		price.enabled = server.hasArg(F("pe")) && server.arg(F("pe")) == F("true");
		strcpy(price.entsoeToken, server.arg(F("pt")).c_str());
		strcpy(price.area, priceRegion.c_str());
		strcpy(price.currency, server.arg(F("pc")).c_str());
		config->setPriceServiceConfig(price);
	}

	if(server.hasArg(F("t")) && server.arg(F("t")) == F("true")) {
		EnergyAccountingConfig eac;
		eac.thresholds[0] = server.arg(F("t0")).toInt();
		eac.thresholds[1] = server.arg(F("t1")).toInt();
		eac.thresholds[2] = server.arg(F("t2")).toInt();
		eac.thresholds[3] = server.arg(F("t3")).toInt();
		eac.thresholds[4] = server.arg(F("t4")).toInt();
		eac.thresholds[5] = server.arg(F("t5")).toInt();
		eac.thresholds[6] = server.arg(F("t6")).toInt();
		eac.thresholds[7] = server.arg(F("t7")).toInt();
		eac.thresholds[8] = server.arg(F("t8")).toInt();
		eac.hours = server.arg(F("th")).toInt();
		config->setEnergyAccountingConfig(eac);
	}

	if(server.hasArg(F("c")) && server.arg(F("c")) == F("true")) {
		sys.energyspeedometer = server.hasArg(F("ces")) && server.arg(F("ces")) == F("true") ? 7 : 0;
		config->setSystemConfig(sys);

		CloudConfig cloud;
		config->getCloudConfig(cloud);
		cloud.enabled = server.hasArg(F("ce")) && server.arg(F("ce")) == F("true");
		cloud.proto = server.arg(F("cp")).toInt();
		config->setCloudConfig(cloud);

		ZmartChargeConfig zcc;
		config->getZmartChargeConfig(zcc);
		zcc.enabled = server.hasArg(F("cze")) && server.arg(F("cze")) == F("true");
		String token = server.arg(F("czt"));
		strcpy(zcc.token, token.c_str());
		if(server.hasArg(F("czu")) && server.arg(F("czu")).startsWith(F("https"))) {
			strcpy(zcc.baseUrl, server.arg(F("czu")).c_str());
		}
		config->setZmartChargeConfig(zcc);
	}

	if(server.hasArg(F("r")) && server.arg(F("r")) == F("true")) {
		if(ps != NULL) {
			uint8_t count = server.arg(F("rc")).toInt();
			for(uint8_t i = 0; i < count; i++) {
				PriceConfig pc;
				snprintf_P(buf, BufferSize, PSTR("rt%d"), i);
				pc.type = server.arg(buf).toInt();
				snprintf_P(buf, BufferSize, PSTR("rd%d"), i);
				pc.direction = server.arg(buf).toInt();
				snprintf_P(buf, BufferSize, PSTR("rv%d"), i);
				pc.value = server.arg(buf).toFloat() * 10000;
				snprintf_P(buf, BufferSize, PSTR("rn%d"), i);
				String name = server.arg(buf);
				strcpy(pc.name, name.c_str());

				int d = 0;
				pc.days = 0x00;
				snprintf_P(buf, BufferSize, PSTR("ra%d"), i);
				String days = server.arg(buf);
				char * pch = strtok ((char*) days.c_str(),",");
				while (pch != NULL && d < 7) {
					int day = String(pch).toInt();
					pc.days |= (0x1 << day);
					pch = strtok (NULL, ",");
					d++;
				}

				int h = 0;
				pc.hours = 0x00000000;
				snprintf_P(buf, BufferSize, PSTR("rh%d"), i);
				String hours = server.arg(buf);
				pch = strtok ((char*) hours.c_str(),",");
				while (pch != NULL && h < 24) {
					int hour = String(pch).toInt();
					pc.hours |= (0x1 << (hour));
					pch = strtok (NULL, ",");
					h++;
				}

				snprintf_P(buf, BufferSize, PSTR("rsm%d"), i);
				pc.start_month = server.arg(buf).toInt();

				snprintf_P(buf, BufferSize, PSTR("rsd%d"), i);
				pc.start_dayofmonth = server.arg(buf).toInt();

				snprintf_P(buf, BufferSize, PSTR("rem%d"), i);
				pc.end_month = server.arg(buf).toInt();

				snprintf_P(buf, BufferSize, PSTR("red%d"), i);
				pc.end_dayofmonth = server.arg(buf).toInt();

				ps->setPriceConfig(i, pc);
			}
			ps->cropPriceConfig(count);
			ps->save();
			#if defined(_CLOUDCONNECTOR_H)
			if(cloud != NULL) {
				cloud->forcePriceUpdate();
			}
			#endif
		} else {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::WARNING))
			#endif
			debugger->printf_P(PSTR("Price service missing...\n"));
		}
	}

	#if defined(AMS_REMOTE_DEBUG)
	if (debugger->isActive(RemoteDebug::INFO))
	#endif
	debugger->printf_P(PSTR("Saving configuration now...\n"));

	// If vendor page and clear all config is selected
	if(server.hasArg(F("v")) && server.arg(F("v")) == F("true") && server.hasArg(F("vr")) && server.arg(F("vr")) == F("true")) {
		config->clear();
	} else if(config->save()) {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::INFO))
		#endif
		debugger->printf_P(PSTR("Successfully saved.\n"));
		if(config->isNetworkConfigChanged() || config->isWebChanged() || performRestart) {
			performRestart = true;
		} else {
			hw->setup(&sys, gpioConfig);
		}
	} else {
		success = false;
	}

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		success ? "true" : "false",
		"",
		performRestart ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	if(performRestart) {
		server.handleClient();
		delay(250);

		if(ds != NULL) {
			ds->save();
		}
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::INFO))
		#endif
		debugger->printf_P(PSTR("Rebooting\n"));
		debugger->flush();
		delay(1000);
		rdc->cause = 2;
		performRestart = false;
		ESP.restart();
	}
}

void AmsWebServer::reboot() {
	if(!checkSecurity(1))
		return;

	server.send(200, MIME_JSON, "{\"reboot\":true}");

	server.handleClient();
	delay(250);

	#if defined(AMS_REMOTE_DEBUG)
if (debugger->isActive(RemoteDebug::INFO))
#endif
debugger->printf_P(PSTR("Rebooting\n"));
	debugger->flush();
	delay(1000);	rdc->cause = 3;

	rdc->cause = 3;
	performRestart = false;
	ESP.restart();
}

void AmsWebServer::upgrade() {
	if(!checkSecurity(1))
		return;

	SystemConfig sys;
	config->getSystemConfig(sys);

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		sys.dataCollectionConsent == 1 ? "true" : "false",
		"",
		sys.dataCollectionConsent == 1 ? "true" : "false"
	);

	addConditionalCloudHeaders();
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	if(sys.dataCollectionConsent == 1) {
		server.handleClient();
		delay(250);

		String version = server.arg(F("expected_version"));
		updater->setTargetVersion(version.c_str());
	}
}

void AmsWebServer::firmwareHtml() {
	if(!checkSecurity(1))
		return;

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(FIRMWARE_HTML_LEN);
	server.send_P(200, MIME_HTML, FIRMWARE_HTML);
}

void AmsWebServer::firmwarePost() {
	if(!checkSecurity(1))
		return;
	
	if(performRestart) {
		server.send(200);
	} else {
		server.sendHeader(HEADER_LOCATION,F("/firmware"));
		server.send(303);
	}
}


void AmsWebServer::firmwareUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = server.upload();
	if(upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
		if(filename.isEmpty()) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("No file, falling back to post\n"));
			return;
		}
        if(!filename.endsWith(F(".bin"))) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("Invalid file extension\n"));
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Invalid file extension",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
			return;
		}
		if(!updater->startFirmwareUpload(upload.totalSize, "new")) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("An error has occurred while starting firmware upload\n"));
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Unable to start firmware upgrade",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
			return;
		}
		#if defined(ESP32)
			esp_task_wdt_delete(NULL);
			esp_task_wdt_deinit();
		#elif defined(ESP8266)
			ESP.wdtDisable();
		#endif
    }
	
	if(upload.currentSize > 0) {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::DEBUG))
		#endif
		debugger->printf_P(PSTR("Writing chunk: %lu bytes\n"), upload.currentSize);
		if(!updater->addFirmwareUploadChunk(upload.buf, upload.currentSize)) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("An error has occurred while writing firmware to flash\n"));
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Unable to write to flash",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
			return;
		}
    }
	
	if(upload.status == UPLOAD_FILE_END) {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::DEBUG))
		#endif
		debugger->printf_P(PSTR("Upload complete\n"));

		if(updater->completeFirmwareUpload(upload.totalSize)) {
			performRestart = true;
			server.sendHeader(HEADER_LOCATION,F("/"));
			server.send(302);
		} else {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("An error has occurred while activating new firmware\n"));
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Unable to activate new firmware",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
			return;
		}
    }
}

HTTPUpload& AmsWebServer::uploadFile(const char* path) {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) {
		if(uploading) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("Upload already in progress\n"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Upload already in progress!</h1></body></html>"));
		} else if (!LittleFS.begin()) {
			#if defined(AMS_REMOTE_DEBUG)
			if (debugger->isActive(RemoteDebug::ERROR))
			#endif
			debugger->printf_P(PSTR("An Error has occurred while mounting LittleFS\n"));
			server.send_P(500, MIME_HTML, PSTR("<html><body><h1>Unable to mount LittleFS!</h1></body></html>"));
		} else {
			uploading = true;
			if(LittleFS.exists(path)) {
				LittleFS.remove(path);
			}
		    file = LittleFS.open(path, "w");
            size_t written = file.write(upload.buf, upload.currentSize);
	    } 
    } else if(upload.status == UPLOAD_FILE_WRITE) {
        if(file) {
            size_t written = file.write(upload.buf, upload.currentSize);
			delay(1);
			if(written != upload.currentSize) {
				file.flush();
				file.close();
				LittleFS.remove(path);

				#if defined(AMS_REMOTE_DEBUG)
				if (debugger->isActive(RemoteDebug::ERROR))
				#endif
				debugger->printf_P(PSTR("An Error has occurred while writing file\n"));
				snprintf_P(buf, BufferSize, RESPONSE_JSON,
					"false",
					"File size does not match",
					"false"
				);
				server.setContentLength(strlen(buf));
				server.send(500, MIME_JSON, buf);
			}
		}
    } else if(upload.status == UPLOAD_FILE_END) {
        if(file) {
			file.flush();
            file.close();
        } else {
			debugger->printf_P(PSTR("File was not valid in the end... Write error: %d, \n"), file.getWriteError());
			snprintf_P(buf, BufferSize, RESPONSE_JSON,
				"false",
				"Upload ended, but file is missing",
				"false"
			);
			server.setContentLength(strlen(buf));
			server.send(500, MIME_JSON, buf);
        }
    }
	return upload;
}

void AmsWebServer::isAliveCheck() {
	server.sendHeader(HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, F("*"));
	server.send(200);
}

void AmsWebServer::factoryResetPost() {
	if(!checkSecurity(1))
		return;

	bool success = false;
	if(server.hasArg(F("perform")) && server.arg(F("perform")) == F("true")) {
		LittleFS.format();
		config->clear();

		success = true;
	}

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		success ? "true" : "false",
		"",
		"true"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);

	server.handleClient();
	delay(250);

	#if defined(AMS_REMOTE_DEBUG)
	if (debugger->isActive(RemoteDebug::INFO))
	#endif
	debugger->printf_P(PSTR("Rebooting\n"));
	debugger->flush();
	delay(1000);
	rdc->cause = 5;
	ESP.restart();
}

void AmsWebServer::robotstxt() {
	server.send_P(200, MIME_HTML, PSTR("User-agent: *\nDisallow: /\n"));
}

void AmsWebServer::mqttCaUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CA);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);

		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttCaDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_CA);
		server.send(200);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::mqttCertUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_CERT);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttCertDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_CERT);
		server.send(200);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::mqttKeyUpload() {
	if(!checkSecurity(1))
		return;

	uploadFile(FILE_MQTT_KEY);
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_END) {
		server.sendHeader(HEADER_LOCATION,F("/configuration"));
		server.send(303);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	}
}

void AmsWebServer::mqttKeyDelete() {
	if(!checkSecurity(1))
		return;

	if(!uploading) { // Not an upload
		deleteFile(FILE_MQTT_KEY);
		server.send(200);
		MqttConfig mqttConfig;
		if(config->getMqttConfig(mqttConfig) && mqttConfig.ssl) {
			config->setMqttChanged();
		}
	} else {
		uploading = false;
		server.send(200);
	}
}

void AmsWebServer::deleteFile(const char* path) {
	if(LittleFS.begin()) {
		LittleFS.remove(path);
	}
}

void AmsWebServer::tariffJson() {
	if(!checkSecurity(2))
		return;

	EnergyAccountingConfig* eac = ea->getConfig();

	String peaks;
    for(uint8_t x = 0;x < min((uint8_t) 5, eac->hours); x++) {
		EnergyAccountingPeak peak = ea->getPeak(x+1);
		int len = snprintf_P(buf, BufferSize, PSTR("{\"d\":%d,\"v\":%.2f}"),
			peak.day,
			peak.value / 100.0
		);
		buf[len] = '\0';
		if(!peaks.isEmpty()) peaks += ",";
		peaks += String(buf);
	}

	snprintf_P(buf, BufferSize, TARIFF_JSON,
		eac->thresholds[0],
		eac->thresholds[1],
		eac->thresholds[2],
		eac->thresholds[3],
		eac->thresholds[4],
		eac->thresholds[5],
		eac->thresholds[6],
		eac->thresholds[7],
		eac->thresholds[8],
		eac->thresholds[9],
		peaks.c_str(),
		ea->getCurrentThreshold(),
		ea->getMonthMax()
	);

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::realtimeJson() {
	if(rtp == NULL) {
		server.send_P(500, MIME_PLAIN, PSTR("500: Not available"));
		return;
	}

	addConditionalCloudHeaders();
	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	uint16_t offset = 0;
	if(server.hasArg(F("offset"))) {
		offset = server.arg(F("offset")).toInt();
	}

	uint16_t size = 60;
	if(server.hasArg(F("size"))) {
		size = server.arg(F("size")).toInt();
	}
	
	if(size > rtp->getSize()) {
		size = rtp->getSize();
	}
	if(offset > rtp->getSize()) {
		offset = rtp->getSize();
	}

	uint16_t pos = snprintf_P(buf, BufferSize, PSTR("{\"offset\":%d,\"size\":%d,\"total\":%d,\"data\":["), offset, size, rtp->getSize());
	bool first = true;
	for(uint16_t i = 0; i < size; i++) {
		pos += snprintf_P(buf+pos, BufferSize-pos, PSTR("%s%d"), first ? "" : ",", rtp->getValue(offset+i));
		first = false;
		delay(1);
	}
	pos += snprintf_P(buf+pos, BufferSize-pos, PSTR("]}"));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::setPriceSettings(String region, String currency) {
	this->priceRegion = region;
	this->priceCurrency = currency;
}

void AmsWebServer::configFileDownload() {
	if(!checkSecurity(1))
		return;

	bool includeSecrets = server.hasArg(F("ic")) && server.arg(F("ic")) == F("true");
	bool includeWifi = server.hasArg(F("iw")) && server.arg(F("iw")) == F("true");
	bool includeMqtt = server.hasArg(F("im")) && server.arg(F("im")) == F("true");
	bool includeWeb = server.hasArg(F("ie")) && server.arg(F("ie")) == F("true");
	bool includeMeter = server.hasArg(F("it")) && server.arg(F("it")) == F("true");
	bool includeGpio = server.hasArg(F("ig")) && server.arg(F("ig")) == F("true");
	bool includeNtp = server.hasArg(F("in")) && server.arg(F("in")) == F("true");
	bool includePrice = server.hasArg(F("is")) && server.arg(F("is")) == F("true");
	bool includeThresholds = server.hasArg(F("ih")) && server.arg(F("ih")) == F("true");

	SystemConfig sys;
	config->getSystemConfig(sys);

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.sendHeader(F("Content-Disposition"), F("attachment; filename=configfile.cfg"));
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	server.send_P(200, MIME_PLAIN, PSTR("amsconfig\n"));
	server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("version %s\n"), FirmwareVersion::VersionString));
	server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("boardType %d\n"), sys.boardType));
	
	if(includeWifi) {
		NetworkConfig network;
		config->getNetworkConfig(network);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("netmode %d\n"), network.mode));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("hostname %s\n"), network.hostname));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ssid %s\n"), network.ssid));
		if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("psk %s\n"), network.psk));
		if(strlen(network.ip) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ip %s\n"), network.ip));
			if(strlen(network.gateway) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gateway %s\n"), network.gateway));
			if(strlen(network.subnet) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("subnet %s\n"), network.subnet));
			if(strlen(network.dns1) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dns1 %s\n"), network.dns1));
			if(strlen(network.dns2) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dns2 %s\n"), network.dns2));
		}
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mdns %d\n"), network.mdns ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("use11b %d\n"), network.use11b ? 1 : 0));
	}
	
	if(includeMqtt) {
		MqttConfig mqtt;
		config->getMqttConfig(mqtt);
		if(strlen(mqtt.host) > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttHost %s\n"), mqtt.host));
			if(mqtt.port > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPort %d\n"), mqtt.port));
			if(strlen(mqtt.clientId) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttClientId %s\n"), mqtt.clientId));
			if(strlen(mqtt.publishTopic) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPublishTopic %s\n"), mqtt.publishTopic));
			if(strlen(mqtt.subscribeTopic) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttSubscribeTopic %s\n"), mqtt.subscribeTopic));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttUsername %s\n"), mqtt.username));
			if(includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPassword %s\n"), mqtt.password));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttPayloadFormat %d\n"), mqtt.payloadFormat));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("mqttSsl %d\n"), mqtt.ssl ? 1 : 0));

			if(mqtt.payloadFormat == 3) {
				DomoticzConfig domo;
				config->getDomoticzConfig(domo);
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzElidx %d\n"), domo.elidx));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl1idx %d\n"), domo.vl1idx));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl2idx %d\n"), domo.vl2idx));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzVl3idx %d\n"), domo.vl3idx));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("domoticzCl1idx %d\n"), domo.cl1idx));
			} else if(mqtt.payloadFormat == 4) {
				HomeAssistantConfig haconf;
				config->getHomeAssistantConfig(haconf);
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("homeAssistantDiscoveryPrefix %s\n"), haconf.discoveryPrefix));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("homeAssistantDiscoveryHostname %s\n"), haconf.discoveryHostname));
				server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("homeAssistantDiscoveryNameTag %s\n"), haconf.discoveryNameTag));
			}
		}
	}
	
	if(includeWeb && includeSecrets) {
		WebConfig web;
		config->getWebConfig(web);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webSecurity %d\n"), web.security));
		if(web.security > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webUsername %s\n"), web.username));
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("webPassword %s\n"), web.password));
		}
	}
	
	if(includeMeter) {
		MeterConfig meter;
		config->getMeterConfig(meter);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterBaud %d\n"), meter.baud));
		char parity[4] = "";
		switch(meter.parity) {
			case 2:
				strcpy_P(parity, PSTR("7N1"));
				break;
			case 3:
				strcpy_P(parity, PSTR("8N1"));
				break;
			case 7:
				strcpy_P(parity, PSTR("8N2"));
				break;
			case 10:
				strcpy_P(parity, PSTR("7E1"));
				break;
			case 11:
				strcpy_P(parity, PSTR("8E1"));
				break;
		}
		if(strlen(parity) > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterParity %s\n"), parity));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterInvert %d\n"), meter.invert ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterDistributionSystem %d\n"), meter.distributionSystem));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterMainFuse %d\n"), meter.mainFuse));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterProductionCapacity %d\n"), meter.productionCapacity));
		if(includeSecrets) {
			if(meter.encryptionKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterEncryptionKey %s\n"), toHex(meter.encryptionKey, 16).c_str()));
			if(meter.authenticationKey[0] != 0x00) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterAuthenticationKey %s\n"), toHex(meter.authenticationKey, 16).c_str()));
		}
		if(meter.wattageMultiplier != 1.0 && meter.wattageMultiplier != 0.0)
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterWattageMultiplier %.3f\n"), meter.wattageMultiplier / 1000.0));
		if(meter.voltageMultiplier != 1.0 && meter.voltageMultiplier != 0.0)
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterVoltageMultiplier %.3f\n"), meter.voltageMultiplier / 1000.0));
		if(meter.amperageMultiplier != 1.0 && meter.amperageMultiplier != 0.0)
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterAmperageMultiplier %.3f\n"), meter.amperageMultiplier / 1000.0));
		if(meter.accumulatedMultiplier != 1.0 && meter.accumulatedMultiplier != 0.0)
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("meterAccumulatedMultiplier %.3f\n"), meter.accumulatedMultiplier / 1000.0));
	}
	
	if(includeGpio) {
		MeterConfig meter;
		config->getMeterConfig(meter);
		GpioConfig gpio;
		config->getGpioConfig(gpio);
		if(meter.rxPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioHanPin %d\n"), meter.rxPin));
		if(meter.rxPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioHanPinPullup %d\n"), meter.rxPinPullup ? 1 : 0));
		if(gpio.apPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioApPin %d\n"), gpio.apPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPin %d\n"), gpio.ledPin));
		if(gpio.ledPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedInverted %d\n"), gpio.ledInverted ? 1 : 0));
		if(gpio.ledPinRed != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinRed %d\n"), gpio.ledPinRed));
		if(gpio.ledPinGreen != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinGreen %d\n"), gpio.ledPinGreen));
		if(gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedPinBlue %d\n"), gpio.ledPinBlue));
		if(gpio.ledPinRed != 0xFF || gpio.ledPinGreen != 0xFF || gpio.ledPinBlue != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioLedRgbInverted %d\n"), gpio.ledRgbInverted ? 1 : 0));
		if(gpio.tempSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioTempSensorPin %d\n"), gpio.tempSensorPin));
		if(gpio.tempAnalogSensorPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioTempAnalogSensorPin %d\n"), gpio.tempAnalogSensorPin));
		if(gpio.vccPin != 0xFF) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccPin %d\n"), gpio.vccPin));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccOffset %.2f\n"), gpio.vccOffset / 100.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccMultiplier %.3f\n"), gpio.vccMultiplier / 1000.0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccBootLimit %.1f\n"), gpio.vccBootLimit / 10.0));
		if(gpio.vccPin != 0xFF && gpio.vccResistorGnd != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccResistorGnd %d\n"), gpio.vccResistorGnd));
		if(gpio.vccPin != 0xFF && gpio.vccResistorVcc != 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("gpioVccResistorVcc %d\n"), gpio.vccResistorVcc));
	}

	if(includeNtp) {
		NtpConfig ntp;
		config->getNtpConfig(ntp);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpEnable %d\n"), ntp.enable ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpDhcp %d\n"), ntp.dhcp ? 1 : 0));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpTimezone %s\n"), ntp.timezone));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("ntpServer %s\n"), ntp.server));
	}

	if(includePrice) {
		PriceServiceConfig price;
		config->getPriceServiceConfig(price);
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("priceEnabled %d\n"), price.enabled ? 1 : 0));
		if(strlen(price.entsoeToken) == 36 && includeSecrets) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("priceEntsoeToken %s\n"), price.entsoeToken));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("priceArea %s\n"), price.area));
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("priceCurrency %s\n"), price.currency));
		if(ps != NULL) {
			uint8_t i = 0;
			std::vector<PriceConfig> pc = ps->getPriceConfig();
			if(pc.size() > 0) {
				for(uint8_t i = 0; i < pc.size(); i++) {
					PriceConfig& p = pc.at(i);
					char direction[7] = "";
					switch(p.direction) {
						case PRICE_DIRECTION_IMPORT:
							strcpy_P(direction, PSTR("import"));
							break;
						case PRICE_DIRECTION_EXPORT:
							strcpy_P(direction, PSTR("export"));
							break;
						case PRICE_DIRECTION_BOTH:
							strcpy_P(direction, PSTR("both"));
							break;
						default:
							strcpy_P(direction, PSTR("--"));
					}
					char type[9] = "";
					switch(p.type) {
						case PRICE_TYPE_FIXED:
							strcpy_P(type, PSTR("fixed"));
							break;
						case PRICE_TYPE_ADD:
							strcpy_P(type, PSTR("add"));
							break;
						case PRICE_TYPE_PCT:
							strcpy_P(type, PSTR("percent"));
							break;
						case PRICE_TYPE_SUBTRACT:
							strcpy_P(type, PSTR("subtract"));
							break;
						default:
							strcpy_P(direction, PSTR("--"));
					}
					char days[3*7] = "";
					if(p.days == 0x7F) {
						strcpy_P(days, PSTR("all"));
					} else {
						if((p.days >> 0) & 0x01) strcat_P(days, PSTR("mo,"));
						if((p.days >> 1) & 0x01) strcat_P(days, PSTR("tu,"));
						if((p.days >> 2) & 0x01) strcat_P(days, PSTR("we,"));
						if((p.days >> 3) & 0x01) strcat_P(days, PSTR("th,"));
						if((p.days >> 4) & 0x01) strcat_P(days, PSTR("fr,"));
						if((p.days >> 5) & 0x01) strcat_P(days, PSTR("sa,"));
						if((p.days >> 6) & 0x01) strcat_P(days, PSTR("su,"));
						if(strlen(days) > 0) days[strlen(days)-1] = '\0';
					}

					char hours[3*24] = "";
					if(p.hours == 0xFFFFFF) {
						strcpy_P(hours, PSTR("all"));
					} else {
						for(uint8_t i = 0; i < 24; i++) {
							if((p.hours >> i) & 0x01) {
								char h[4];
								snprintf_P(h, 4, PSTR("%02d,"), i);
								strcat(hours, h);
							}
						}
						if(strlen(hours) > 0) hours[strlen(hours)-1] = '\0';
					}

					server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("priceModifier %i \"%s\" %s %s %.4f %s %s %02d-%02d %02d-%02d\n"), 
						i, 
						p.name, 
						direction,
						type,
						p.value / 10000.0,
						days,
						hours,
						p.start_dayofmonth,
						p.start_month,
						p.end_dayofmonth,
						p.end_month
					));
				}
			}
		}
	}

	if(includeThresholds) {
		EnergyAccountingConfig eac;
		config->getEnergyAccountingConfig(eac);

		if(eac.thresholds[9] > 0) server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("thresholds %d %d %d %d %d %d %d %d %d %d %d\n"), 
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
		));
	}


	if(ds != NULL) {
		DayDataPoints day = ds->getDayData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("dayplot %d %lu %.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			day.version,
			(int32_t) day.lastMeterReadTime,
			day.activeImport / 1000.0,
			day.accuracy,
			ds->getHourImport(0),
			ds->getHourImport(1),
			ds->getHourImport(2),
			ds->getHourImport(3),
			ds->getHourImport(4),
			ds->getHourImport(5),
			ds->getHourImport(6),
			ds->getHourImport(7),
			ds->getHourImport(8),
			ds->getHourImport(9),
			ds->getHourImport(10),
			ds->getHourImport(11),
			ds->getHourImport(12),
			ds->getHourImport(13),
			ds->getHourImport(14),
			ds->getHourImport(15),
			ds->getHourImport(16),
			ds->getHourImport(17),
			ds->getHourImport(18),
			ds->getHourImport(19),
			ds->getHourImport(20),
			ds->getHourImport(21),
			ds->getHourImport(22),
			ds->getHourImport(23)
		));
		if(day.activeExport > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR(" %.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"), 
				day.activeExport / 1000.0,
				ds->getHourExport(0),
				ds->getHourExport(1),
				ds->getHourExport(2),
				ds->getHourExport(3),
				ds->getHourExport(4),
				ds->getHourExport(5),
				ds->getHourExport(6),
				ds->getHourExport(7),
				ds->getHourExport(8),
				ds->getHourExport(9),
				ds->getHourExport(10),
				ds->getHourExport(11),
				ds->getHourExport(12),
				ds->getHourExport(13),
				ds->getHourExport(14),
				ds->getHourExport(15),
				ds->getHourExport(16),
				ds->getHourExport(17),
				ds->getHourExport(18),
				ds->getHourExport(19),
				ds->getHourExport(20),
				ds->getHourExport(21),
				ds->getHourExport(22),
				ds->getHourExport(23)
			));
		} else {
			server.sendContent(F("\n"));
		}

		MonthDataPoints month = ds->getMonthData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("monthplot %d %lu %.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"), 
			month.version,
			(int32_t) month.lastMeterReadTime,
			month.activeImport / 1000.0,
			month.accuracy,
			ds->getDayImport(1),
			ds->getDayImport(2),
			ds->getDayImport(3),
			ds->getDayImport(4),
			ds->getDayImport(5),
			ds->getDayImport(6),
			ds->getDayImport(7),
			ds->getDayImport(8),
			ds->getDayImport(9),
			ds->getDayImport(10),
			ds->getDayImport(11),
			ds->getDayImport(12),
			ds->getDayImport(13),
			ds->getDayImport(14),
			ds->getDayImport(15),
			ds->getDayImport(16),
			ds->getDayImport(17),
			ds->getDayImport(18),
			ds->getDayImport(19),
			ds->getDayImport(20),
			ds->getDayImport(21),
			ds->getDayImport(22),
			ds->getDayImport(23),
			ds->getDayImport(24),
			ds->getDayImport(25),
			ds->getDayImport(26),
			ds->getDayImport(27),
			ds->getDayImport(28),
			ds->getDayImport(29),
			ds->getDayImport(30),
			ds->getDayImport(31)
		));
		if(month.activeExport > 0) {
			server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR(" %.3f %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n"), 
				month.activeExport / 1000.0,
				ds->getDayExport(1),
				ds->getDayExport(2),
				ds->getDayExport(3),
				ds->getDayExport(4),
				ds->getDayExport(5),
				ds->getDayExport(6),
				ds->getDayExport(7),
				ds->getDayExport(8),
				ds->getDayExport(9),
				ds->getDayExport(10),
				ds->getDayExport(11),
				ds->getDayExport(12),
				ds->getDayExport(13),
				ds->getDayExport(14),
				ds->getDayExport(15),
				ds->getDayExport(16),
				ds->getDayExport(17),
				ds->getDayExport(18),
				ds->getDayExport(19),
				ds->getDayExport(20),
				ds->getDayExport(21),
				ds->getDayExport(22),
				ds->getDayExport(23),
				ds->getDayExport(24),
				ds->getDayExport(25),
				ds->getDayExport(26),
				ds->getDayExport(27),
				ds->getDayExport(28),
				ds->getDayExport(29),
				ds->getDayExport(30),
				ds->getDayExport(31)
			));
		} else {
			server.sendContent(F("\n"));
		}
	}

	if(ea != NULL) {
		EnergyAccountingConfig eac;
		config->getEnergyAccountingConfig(eac);
		EnergyAccountingData ead = ea->getData();
		server.sendContent(buf, snprintf_P(buf, BufferSize, PSTR("energyaccounting %d %d %.2f %.2f %.2f %.2f %.2f %.2f %d %.2f %d %.2f %d %.2f %d %.2f %d %.2f %.2f %.2f"), 
			ead.version,
			ead.month,
			ea->getCostYesterday(),
			ea->getCostThisMonth(),
			ea->getCostLastMonth(),
			ea->getIncomeYesterday(),
			ea->getIncomeThisMonth(),
			ea->getIncomeLastMonth(),
			ead.peaks[0].day,
			ead.peaks[0].value / 100.0,
			ead.peaks[1].day,
			ead.peaks[1].value / 100.0,
			ead.peaks[2].day,
			ead.peaks[2].value / 100.0,
			ead.peaks[3].day,
			ead.peaks[3].value / 100.0,
			ead.peaks[4].day,
			ead.peaks[4].value / 100.0,
			ea->getUseLastMonth(),
			ea->getProducedLastMonth()
		));
		server.sendContent_P(PSTR("\n"));
	}
}

void AmsWebServer::configFilePost() {
	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		"true",
		"",
		performRestart ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::configFileUpload() {
	if(!checkSecurity(1))
		return;

	HTTPUpload& upload = uploadFile(FILE_CFG);
    if(upload.status == UPLOAD_FILE_END) {
		performRestart = true;
		snprintf_P(buf, BufferSize, RESPONSE_JSON,
			"true",
			"",
			performRestart ? "true" : "false"
		);
		server.setContentLength(strlen(buf));
		server.send(200, MIME_JSON, buf);
	}
}

void AmsWebServer::redirectToMain() {
	String context;
	config->getWebConfig(webConfig);
	stripNonAscii((uint8_t*) webConfig.context, 32);
	if(strlen(webConfig.context) > 0) {
		context = String(webConfig.context);
	}

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);
	server.sendHeader(HEADER_LOCATION, "/" + context);
	server.send(302);
}

void AmsWebServer::ssdpSchema() {
	SSDP.schema(server.client());
}

void AmsWebServer::modifyDayPlot() {
	if(!checkSecurity(1))
		return;

	for(uint8_t i = 0; i < 24; i++) {
		snprintf_P(buf, BufferSize, PSTR("i%02d"), i);
		if(server.hasArg(buf)) {
			ds->setHourImport(i, server.arg(buf).toDouble() * 1000);
		}
		snprintf_P(buf, BufferSize, PSTR("e%02d"), i);
		if(server.hasArg(buf)) {
			ds->setHourExport(i, server.arg(buf).toDouble() * 1000);
		}
	}
	bool ret = ds->save();

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		"true",
		"",
		ret ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::modifyMonthPlot() {
	if(!checkSecurity(1))
		return;

	for(uint8_t i = 1; i <= 31; i++) {
		snprintf_P(buf, BufferSize, PSTR("i%02d"), i);
		if(server.hasArg(buf)) {
			ds->setDayImport(i, server.arg(buf).toDouble() * 1000);
		}
		snprintf_P(buf, BufferSize, PSTR("e%02d"), i);
		if(server.hasArg(buf)) {
			ds->setDayExport(i, server.arg(buf).toDouble() * 1000);
		}
	}
	bool ret = ds->save();

	snprintf_P(buf, BufferSize, RESPONSE_JSON,
		"true",
		"",
		ret ? "true" : "false"
	);
	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}

void AmsWebServer::addConditionalCloudHeaders() {
	server.sendHeader(HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, ORIGIN_AMSLESER_CLOUD);

	String req = server.header(HEADER_ACCESS_CONTROL_REQUEST_PRIVATE_NETWORK);
	if(!req.equalsIgnoreCase(F("true"))) {
		return;
	}

	String ref;
	if(server.hasHeader(HEADER_ORIGIN)) {
		ref = server.header(HEADER_ORIGIN);
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::DEBUG))
		#endif
		debugger->printf_P(PSTR("Origin: %s\n"), ref.c_str());
	} else if(server.hasHeader(HEADER_REFERER)) {
		ref = server.header(HEADER_REFERER);
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::DEBUG))
		#endif
		debugger->printf_P(PSTR("Referer: %s\n"), ref.c_str());
	} else {
		#if defined(AMS_REMOTE_DEBUG)
		if (debugger->isActive(RemoteDebug::DEBUG))
		#endif
		debugger->printf_P(PSTR("No Origin or Referer\n"));
	}
	if(!ref.startsWith(ORIGIN_AMSLESER_CLOUD)) {
		return;
	}

	NetworkConfig networkConfig;
	config->getNetworkConfig(networkConfig);

    char macStr[18] = { 0 };
    char apMacStr[18] = { 0 };

	uint8_t mac[6];

	#if defined(ESP8266)
    wifi_get_macaddr(STATION_IF, mac);
	#elif defined(ESP32)
    esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
	#endif

    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	server.sendHeader(HEADER_ACCESS_CONTROL_ALLOW_PRIVATE_NETWORK, F("true"));
	server.sendHeader(F("Private-Network-Access-Name"), networkConfig.hostname);
	server.sendHeader(F("Private-Network-Access-ID"), macStr);
}

void AmsWebServer::optionsGet() {
	addConditionalCloudHeaders();
	server.sendHeader(F("Allow"), F("GET"));
	server.send(200);
}

void AmsWebServer::wifiScan() {
	if(!checkSecurity(1))
		return;

	int16_t count = WiFi.scanNetworks();
	int pos = snprintf_P(buf, BufferSize, PSTR("{\"c\":%d,\"n\":["), count);
	count = min(count, (int16_t) 25); // Max 25 so that we don't overflow the buffer size
	for (int16_t i = 0; i < count; i++) {
		uint8_t* bssid = WiFi.BSSID(i);
		String ssid = WiFi.SSID(i);
		int32_t rssi = WiFi.RSSI(i);
		int32_t chan = WiFi.channel(i);
		#if defined(ESP32)
			wifi_auth_mode_t enc = WiFi.encryptionType(i);
			String encStr;
			switch (enc) {
				case WIFI_AUTH_OPEN:            encStr = "open"; break;
				case WIFI_AUTH_WEP:             encStr = "WEP"; break;
				case WIFI_AUTH_WPA_PSK:         encStr = "WPA"; break;
				case WIFI_AUTH_WPA2_PSK:        encStr = "WPA2"; break;
				case WIFI_AUTH_WPA_WPA2_PSK:    encStr = "WPA+WPA2"; break;
				case WIFI_AUTH_WPA2_ENTERPRISE: encStr = "WPA2-EAP"; break;
				case WIFI_AUTH_WPA3_PSK:        encStr = "WPA3"; break;
				case WIFI_AUTH_WPA2_WPA3_PSK:   encStr = "WPA2+WPA3"; break;
				case WIFI_AUTH_WAPI_PSK:        encStr = "WAPI"; break;
				default:                        encStr = "unknown";
			}
		#else
			uint8_t enc = WiFi.encryptionType(i);
			String encStr;
			switch (enc) {
				case ENC_TYPE_WEP:  encStr = "WEP"; break;
				case ENC_TYPE_TKIP: encStr = "WPA"; break;
				case ENC_TYPE_CCMP: encStr = "WPA2"; break;
				case ENC_TYPE_NONE: encStr = "open"; break;
				default:            encStr = "unknown";
			}
		#endif

	    char bssidStr[18] = { 0 };
	    sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
		pos += snprintf_P(buf+pos, BufferSize-pos, PSTR("{\"b\":\"%s\",\"s\":\"%s\",\"r\":%d,\"c\":%d,\"e\":\"%s\"}%s"), bssidStr, ssid.c_str(), rssi, chan, encStr, i == count-1 ? "" : ",");
	}
	pos += snprintf_P(buf+pos, BufferSize-pos, PSTR("]}"));
	WiFi.scanDelete();

	server.sendHeader(HEADER_CACHE_CONTROL, CACHE_CONTROL_NO_CACHE);
	server.sendHeader(HEADER_PRAGMA, PRAGMA_NO_CACHE);
	server.sendHeader(HEADER_EXPIRES, EXPIRES_OFF);

	server.setContentLength(strlen(buf));
	server.send(200, MIME_JSON, buf);
}