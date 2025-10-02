/**
 * @copyright Utilitech AS 2023
 * License: Fair Source 5
 * 
 * @brief Program for ESP32 and ESP8266 to receive data from AMS electric meters and send to MQTT
 * 
 * @details This program was created to receive data from AMS electric meters via M-Bus, decode 
 * and send to a MQTT broker. The data packet structure supported by this software is specific 
 * to Norwegian meters, but may also support data from electricity providers in other countries. 
 */

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
ADC_MODE(ADC_VCC);
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESP32SSDP.h>
#include <esp_task_wdt.h>
#include <lwip/dns.h>
#endif
#if defined(AMS_CLOUD)
#include "CloudConnector.h"
#endif
#if defined(ZMART_CHARGE)
#include "ZmartChargeCloudConnector.h"
#endif

#define WDT_TIMEOUT 120
#if defined(SLOW_PROC_TRIGGER_MS)
	#warning "Using predefined slow process trigger"
#else
	#define SLOW_PROC_TRIGGER_MS 1000
#endif

#define METER_SOURCE_NONE 0
#define METER_SOURCE_GPIO 1
#define METER_SOURCE_MQTT 2
#define METER_SOURCE_ESPNOW 3

#define METER_PARSER_PASSIVE 0
#define METER_PARSER_PULSE 2
#define METER_PARSER_KAMSTRUP 9

#define METER_ERROR_UNKNOWN_DATA 89
#define METER_ERROR_NO_DATA 90
#define METER_ERROR_BREAK 91
#define METER_ERROR_BUFFER 92
#define METER_ERROR_FIFO 93
#define METER_ERROR_FRAME 94
#define METER_ERROR_PARITY 95
#define METER_ERROR_RX 96
#define METER_ERROR_EXCEPTION 98
#define METER_ERROR_AUTODETECT 99

#include "LittleFS.h"

#include "FirmwareVersion.h"
#include "AmsStorage.h"
#include "AmsDataStorage.h"
#include "EnergyAccounting.h"
#include <MQTT.h>
#include <DNSServer.h>
#include <lwip/apps/sntp.h>

#include "hexutils.h"
#include "HwTools.h"

#include "ConnectionHandler.h"
#include "WiFiClientConnectionHandler.h"
#include "WiFiAccessPointConnectionHandler.h"
#include "EthernetConnectionHandler.h"
#include "PriceService.h"
#include "RealtimePlot.h"
#include "AmsWebServer.h"
#include "AmsConfiguration.h"

#include "AmsMqttHandler.h"
#include "JsonMqttHandler.h"
#include "RawMqttHandler.h"
#include "DomoticzMqttHandler.h"
#include "HomeAssistantMqttHandler.h"
#include "PassthroughMqttHandler.h"

#include "MeterCommunicator.h"
#include "PassiveMeterCommunicator.h"
#if defined(AMS_KMP)
#include "KmpCommunicator.h"
#endif
#include "PulseMeterCommunicator.h"

#include "Uptime.h"

#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"

#define debugV_P(x, ...)	if (Debug.isActive(Debug.VERBOSE)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugD_P(x, ...)	if (Debug.isActive(Debug.DEBUG))	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugI_P(x, ...)	if (Debug.isActive(Debug.INFO)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugW_P(x, ...)	if (Debug.isActive(Debug.WARNING)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugE_P(x, ...)	if (Debug.isActive(Debug.ERROR)) 	{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
#define debugA_P(x, ...)	if (Debug.isActive(Debug.ANY)) 		{Debug.printf_P(x, ##__VA_ARGS__);Debug.println();}
RemoteDebug Debug;
#else
#define debugV_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
#define debugD_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
#define debugI_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
#define debugW_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
#define debugE_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
#define debugA_P(x, ...)	{Serial.printf_P(x, ##__VA_ARGS__);Serial.println();}
HardwareSerial Debug = Serial;
#endif

#define BUF_SIZE_COMMON (2048)

#include "Timezones.h"

#include "AmsFirmwareUpdater.h"

uint8_t commonBuffer[BUF_SIZE_COMMON];

HwTools hw;

DNSServer* dnsServer = NULL;

AmsConfiguration config;

PriceService* ps = NULL;

Timezone* tz = NULL;

ConnectionHandler* ch = NULL;

#if defined(ESP32)
__NOINIT_ATTR ResetDataContainer rdc;
#else
ResetDataContainer rdc;
#endif
AmsWebServer ws(commonBuffer, &Debug, &hw, &rdc);

bool mqttEnabled = false;
AmsMqttHandler* mqttHandler = NULL;

#if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
JsonMqttHandler* energySpeedometer = NULL;
MqttConfig energySpeedometerConfig = {
	"mqtt.sandtime.energy",
	8883,
	"",
	"amsleser",
	"",
	#if defined(ENERGY_SPEEDOMETER_USER)
	ENERGY_SPEEDOMETER_USER,
	#else
	"",
	#endif
	#if defined(ENERGY_SPEEDOMETER_PASS)
	ENERGY_SPEEDOMETER_PASS,
	#else
	"",
	#endif
	0,
	true
};
#endif

Stream *hanSerial;
HardwareSerial *hwSerial = NULL;
uint8_t rxBufferErrors = 0;

SystemConfig sysConfig;
GpioConfig gpioConfig;

MeterConfig meterConfig;
AmsData meterState;
bool ntpEnabled = false;

bool mdnsEnabled = false;

AmsFirmwareUpdater updater(&Debug, &hw, &meterState);

AmsDataStorage ds(&Debug);
#if defined(_CLOUDCONNECTOR_H)
CloudConnector *cloud = NULL;
#endif
#if defined(ZMART_CHARGE)
ZmartChargeCloudConnector *zcloud = NULL;
#endif
#if defined(ESP32)
__NOINIT_ATTR EnergyAccountingRealtimeData rtd;
#else
EnergyAccountingRealtimeData rtd;
#endif
EnergyAccounting ea(&Debug, &rtd);

RealtimePlot rtp;

MeterCommunicator* mc = NULL;
PassiveMeterCommunicator* passiveMc = NULL;
#if defined(AMS_KMP)
KmpCommunicator* kmpMc = NULL;
#endif
PulseMeterCommunicator* pulseMc = NULL;


bool networkConnected = false;
bool setupMode = false;

void configFileParse();
void connectToNetwork();
void toggleSetupMode();
void postConnect();
void MQTT_connect();
void handleNtpChange();
void handleDataSuccess(AmsData* data);
void handleTemperature(unsigned long now);
void handleSystem(unsigned long now);
void handleButton(unsigned long now);
void handlePriceService(unsigned long now);
void handleClear(unsigned long now);
void handleUiLanguage();
void handleEnergyAccountingChanged();
bool handleVoltageCheck();
bool readHanPort();
void errorBlink();

uint8_t pulses = 0;
void onPulse();

#if defined(ESP32)
uint8_t dnsState = 0;
ip_addr_t dns0;
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
	if(setupMode) return; // None of this necessary in setup mode
	if(ch != NULL) ch->eventHandler(event, info);
	switch(event) {
		case ARDUINO_EVENT_WIFI_STA_CONNECTED: {
			dnsState = 0;
			if(ch != NULL) {
				NetworkConfig conf;
				ch->getCurrentConfig(conf);
				if(conf.ipv6) {
					dnsState = 2; // Never reset if IPv6 is enabled
					debugI_P(PSTR("IPv6 enabled, not monitoring DNS poisoning"));
				}
			}
			break;
		}
		case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
			if(dnsState == 0) {
				const ip_addr_t* dns = dns_getserver(0);
				memcpy(&dns0, dns, sizeof(dns0));

				IPAddress res;
				int ret = WiFi.hostByName("hub.amsleser.no", res);
				if(ret == 0) {
					dnsState = 2;
					debugI_P(PSTR("No DNS, probably a closed network"));
				} else if(dnsState == 0) {
					debugI_P(PSTR("DNS is present and working, monitoring DNS poisoning"));
					dnsState = 1;
				}
			}
			break;
		}
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
			if(WiFi.getMode() == WIFI_STA) {
				wifi_err_reason_t reason = (wifi_err_reason_t) info.wifi_sta_disconnected.reason;
				switch(reason) {
					case WIFI_REASON_AUTH_FAIL:
					case WIFI_REASON_NO_AP_FOUND:
						if(sysConfig.dataCollectionConsent == 0) {
							debugI_P(PSTR("Unable to connect to configured AP, swapping to AP mode"));
							toggleSetupMode();
						}
						break;
				}
			}
			break;
		}
		case ARDUINO_EVENT_SC_FOUND_CHANNEL:
			debugI_P(PSTR("SmartConfig found channel"));
			break;
		case ARDUINO_EVENT_SC_GOT_SSID_PSWD:
			debugI_P(PSTR("SmartConfig got config"));
			break;
	}
}

void rxerr(int err) {
	if(passiveMc != NULL) {
		passiveMc->rxerr(err);
	}
}
#endif


void setup() {
	Serial.begin(115200);

	config.hasConfig(); // Need to run this to make sure all configuration have been migrated before we load GPIO config

	if(!config.getGpioConfig(gpioConfig)) {
		config.clearGpio(gpioConfig);
	}
	if(config.getSystemConfig(sysConfig)) {
		config.getMeterConfig(meterConfig);
		if(sysConfig.boardType < 20) {
			config.clearGpio(gpioConfig, false);
			hw.applyBoardConfig(sysConfig.boardType, gpioConfig, meterConfig, meterConfig.rxPin);
			config.setMeterConfig(meterConfig);
			config.setGpioConfig(gpioConfig);
		}
	} else {
		config.clearMeter(meterConfig);
		sysConfig.boardType = 0;
		sysConfig.vendorConfigured = false;
		sysConfig.userConfigured = false;
		sysConfig.dataCollectionConsent = false;
	}

	delay(1);
	hw.setup(&sysConfig, &gpioConfig);

	if(gpioConfig.apPin >= 0) {
		pinMode(gpioConfig.apPin, INPUT_PULLUP);

		if(!hw.ledOn(LED_GREEN)) {
			hw.ledOn(LED_INTERNAL);
		}
		delay(1000);
		if(digitalRead(gpioConfig.apPin) == LOW) {
			if(!hw.ledOn(LED_RED)) {
				hw.ledBlink(LED_INTERNAL, 4);
			}
			delay(2000);
			if(digitalRead(gpioConfig.apPin) == LOW) {
				if(!hw.ledOff(LED_GREEN)) {
					hw.ledOn(LED_INTERNAL);
				}
				delay(2000);
				if(digitalRead(gpioConfig.apPin) == HIGH) {
					config.clear();
					if(!hw.ledBlink(LED_RED, 6)) {
						hw.ledBlink(LED_INTERNAL, 6);
					}
				}
			}
		}
	}

	hw.ledBlink(LED_INTERNAL, 1);
	hw.ledBlink(LED_RED, 1);
	hw.ledBlink(LED_YELLOW, 1);
	hw.ledBlink(LED_GREEN, 1);
	hw.ledBlink(LED_BLUE, 1);

	PriceServiceConfig price;
	if(config.getPriceServiceConfig(price)) {
		ps = new PriceService(&Debug);
		ps->setup(price);
		ws.setPriceService(ps);
	}
	ws.setPriceSettings(price.area, price.currency);
	ea.setCurrency(price.currency);
	bool shared = false;
	Serial.flush();
	Serial.end();
	if(meterConfig.rxPin == 3) {
		shared = true;
		#if defined(ESP8266)
			SerialConfig serialConfig;
		#elif defined(ESP32)
			uint32_t serialConfig;
		#endif
		switch(meterConfig.parity) {
			case 2:
				serialConfig = SERIAL_7N1;
				break;
			case 3:
				serialConfig = SERIAL_8N1;
				break;
			case 7:
				serialConfig = SERIAL_8N2;
				break;
			case 10:
				serialConfig = SERIAL_7E1;
				break;
			default:
				serialConfig = SERIAL_8E1;
				break;
		}
		#if defined(ESP32)
			#if ARDUINO_USB_CDC_ON_BOOT
				Serial0.begin(meterConfig.baud == 0 ? 2400 : meterConfig.baud, serialConfig, -1, -1, meterConfig.invert);
			#else
				Serial.begin(meterConfig.baud == 0 ? 2400 : meterConfig.baud, serialConfig, -1, -1, meterConfig.invert);
			#endif
		#else
			Serial.begin(meterConfig.baud == 0 ? 2400 : meterConfig.baud, serialConfig, SERIAL_FULL, 1, meterConfig.invert);
		#endif
	}

 	if(!shared) {
		Serial.begin(115200);
	}

	#if defined(AMS_REMOTE_DEBUG)
	Debug.setSerialEnabled(true);
	yield();
	#endif

	float vcc = hw.getVcc();

	debugI_P(PSTR("AMS reader %s started"), FirmwareVersion::VersionString);
	debugI_P(PSTR("Voltage: %.2fV"), vcc);

	float vccBootLimit = gpioConfig.vccBootLimit == 0 ? 0 : min(3.29, gpioConfig.vccBootLimit / 10.0); // Make sure it is never above 3.3v
	if(vccBootLimit > 2.5 && vccBootLimit < 3.3 && (gpioConfig.apPin == 0xFF || digitalRead(gpioConfig.apPin) == HIGH)) { // Skip if user is holding AP button while booting (HIGH = button is released)
		if (vcc < vccBootLimit) {
			{
				Debug.printf_P(PSTR("(setup) Voltage is too low (%.2f < %.2f), sleeping\n"), vcc, vccBootLimit);
				Serial.flush();
			}
			ESP.deepSleep(10000000);    //Deep sleep to allow output cap to charge up
		}  
	}

	if(!hw.ledOn(LED_GREEN)) {
		hw.ledOn(LED_INTERNAL);
	}
	#if defined(ESP32)
	if(updater.relocateOrRepartitionIfNecessary()) {
		ESP.restart();
		return;
	}
	#endif
	hw.ledOff(LED_GREEN);
	hw.ledOff(LED_INTERNAL);

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	#if defined(ESP32)
	WiFi.onEvent(WiFiEvent);
	#endif

	UpgradeInformation upinfo;
	if(config.getUpgradeInformation(upinfo)) {
		updater.setUpgradeInformation(upinfo);
	}
	yield();

	bool hasFs = false;
#if defined(ESP32)
	WiFi.onEvent(WiFiEvent);
	debugD_P(PSTR("ESP32 LittleFS"));
	hasFs = LittleFS.begin();
	if(!hasFs) {
		debugD_P(PSTR(" formatting"));
		hasFs = LittleFS.begin(true);
	}
	debugD_P(PSTR(" size: %lu, used: %lu"), LittleFS.totalBytes(), LittleFS.usedBytes());
#else
	debugD_P(PSTR("ESP8266 LittleFS"));
	hasFs = LittleFS.begin();
#endif
	yield();


	if(hasFs) {
		#if defined(ESP8266)
			LittleFS.gc();
			if(!LittleFS.check()) {
				debugW_P(PSTR("LittleFS filesystem error"));
				if(!LittleFS.format()) {
					debugE_P(PSTR("Unable to format broken filesystem"));
				}
			}
		#endif

		if(LittleFS.exists(FILE_FIRMWARE_DELETE)) {
			LittleFS.remove(FILE_FIRMWARE_DELETE);
		} else if(LittleFS.exists(FILE_CFG)) {
			debugI_P(PSTR("Found config"));
			configFileParse();
			debugI_P(PSTR("Config update complete, restarting"));
			Debug.flush();
			delay(250);
			ESP.restart();
			return;
		}
	}
	yield();

	if(config.hasConfig()) {
		config.print(&Debug);
		connectToNetwork();
		handleNtpChange();
		ds.load();
	} else {
		debugI_P(PSTR("No configuration, booting AP"));
		toggleSetupMode();
	}

	EnergyAccountingConfig *eac = new EnergyAccountingConfig();
	if(!config.getEnergyAccountingConfig(*eac)) {
		config.clearEnergyAccountingConfig(*eac);
		config.setEnergyAccountingConfig(*eac);
		config.ackEnergyAccountingChange();
	}
	ea.setup(&ds, eac);
	ea.load();
	ea.setPriceService(ps);
	ws.setup(&config, &gpioConfig, &meterState, &ds, &ea, &rtp, &updater);

	UiConfig ui;
	if(config.getUiConfig(ui)) {
		if(strlen(ui.language) == 0) {
			strcpy(ui.language, "en");
			config.setUiConfig(ui);
		}
		snprintf_P((char*) commonBuffer, BUF_SIZE_COMMON, PSTR("/translations-%s.json"), ui.language);
		if(!LittleFS.exists((char*) commonBuffer)) {
			debugI_P(PSTR("Marking %s for download"), commonBuffer);
			config.setUiLanguageChanged();
		}
	}

	yield();

	#if defined(ESP32)
		esp_task_wdt_init(WDT_TIMEOUT, true);
 		esp_task_wdt_add(NULL);
    #elif defined(ESP8266)
		ESP.wdtEnable(WDT_TIMEOUT * 1000);
	#endif
}

int buttonTimer = 0;
bool buttonActive = false;
unsigned long longPressTime = 5000;
bool longPressActive = false;

unsigned long lastTemperatureRead = 0;
unsigned long lastSysupdate = 0;
uint64_t lastErrorBlink = 0; 
unsigned long lastVoltageCheck = 0;
int lastError = 0;

void loop() {
	unsigned long now = millis();
	unsigned long start = now;
	#if defined(AMS_REMOTE_DEBUG)
	Debug.handle();
	#endif
	unsigned long end = millis();
	if(end - start > SLOW_PROC_TRIGGER_MS) {
		debugW_P(PSTR("Used %dms to handle debug"), millis()-start);
	}

	handleButton(now);

	if(now > 10000 && now - lastErrorBlink > 3000) {
		errorBlink();
	}

	// Only do normal stuff if we're not booted as AP
	if (!setupMode) {
		if (ch != NULL && !ch->isConnected()) {
			if(networkConnected) {
				#if defined(AMS_REMOTE_DEBUG)
				Debug.stop();
    			#endif

				MDNS.end();
				if(mqttHandler != NULL) {
					mqttHandler->disconnect();
				}
			}
			networkConnected = false;
			connectToNetwork();
		} else {
			if(!networkConnected) {
				postConnect();
			}
			if(config.isNtpChanged()) {
				handleNtpChange();
			}
			#if defined ESP8266
			if(mdnsEnabled) {
				start = millis();
				MDNS.update();
				end = millis();
				if(end - start > SLOW_PROC_TRIGGER_MS) {
					debugW_P(PSTR("Used %dms to update mDNS"), millis()-start);
				}
			}
			#endif

			if (mqttEnabled || config.isMqttChanged()) {
				if(mqttHandler == NULL || !mqttHandler->connected() || config.isMqttChanged()) {
					if(mqttHandler != NULL && config.isMqttChanged()) {
						mqttHandler->disconnect();
					}
					MQTT_connect();
					config.ackMqttChange();
				}
			} else if(mqttHandler != NULL) {
				mqttHandler->disconnect();
			}

			#if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
			if(sysConfig.energyspeedometer == 7) {
				if(!meterState.getMeterId().isEmpty()) {
					if(energySpeedometer == NULL) {
						uint16_t chipId;
						#if defined(ESP32)
							chipId = ( ESP.getEfuseMac() >> 32 ) % 0xFFFFFFFF;
						#else
							chipId = ESP.getChipId();
						#endif
						strcpy(energySpeedometerConfig.clientId, (String("ams") + String(chipId, HEX)).c_str());
						energySpeedometer = new JsonMqttHandler(energySpeedometerConfig, &Debug, (char*) commonBuffer, &hw, &updater);
						energySpeedometer->setCaVerification(false);
					}
					if(!energySpeedometer->connected()) {
						lwmqtt_err_t err = energySpeedometer->lastError();
						if(err > 0)
							debugE_P(PSTR("Energyspeedometer connector reporting error (%d)"), err);
						energySpeedometer->connect();
						energySpeedometer->publishSystem(&hw, ps, &ea);
					}
					energySpeedometer->loop();
					delay(10);
				}
			} else if(energySpeedometer != NULL) {
				if(energySpeedometer->connected()) {
					energySpeedometer->disconnect();
					energySpeedometer->loop();
				} else {
					delete energySpeedometer;
					energySpeedometer = NULL;
				}
			}
			#endif

			try {
				handlePriceService(now);
			} catch(const std::exception& e) {
				debugE_P(PSTR("Exception in PriceService loop (%s)"), e.what());
			}
			start = millis();
			ws.loop();
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to handle web"), millis()-start);
			}

			if(mqttHandler != NULL) {
				start = millis();
				mqttHandler->loop();
				delay(10); // Needed to preserve power. After adding this, the voltage is super smooth on a HAN powered device
				end = millis();
				if(end - start > SLOW_PROC_TRIGGER_MS) {
					debugW_P(PSTR("Used %dms to handle mqtt"), millis()-start);
				}
			}

			#if defined(_CLOUDCONNECTOR_H)
			if(config.isCloudChanged()) {
				CloudConfig cc;
				if(config.getCloudConfig(cc) && cc.enabled) {
					if(cloud == NULL) {
						cloud = new CloudConnector(&Debug);
					}
					NtpConfig ntp;
					config.getNtpConfig(ntp);
					if(cloud->setup(cc, meterConfig, sysConfig, ntp, &hw, &rdc, ps)) {
						config.setCloudConfig(cc);
					}
					cloud->setConnectionHandler(ch);

					PriceServiceConfig price;
					config.getPriceServiceConfig(price);
					cloud->setPriceConfig(price);

					EnergyAccountingConfig *eac = ea.getConfig();
					cloud->setEnergyAccountingConfig(*eac);

					ws.setCloud(cloud);
				} else if(cloud != NULL) {
					delete cloud;
					cloud = NULL;
				}
				config.ackCloudConfig();
			}
			if(cloud != NULL) {
				cloud->update(meterState, ea);
			}
			#endif

			#if defined(ZMART_CHARGE)
			if(config.isZmartChargeConfigChanged()) {
				ZmartChargeConfig zcc;
				if(config.getZmartChargeConfig(zcc) && zcc.enabled) {
					if(zcloud == NULL) {
						zcloud = new ZmartChargeCloudConnector(&Debug, (char*) commonBuffer);
					}
					zcloud->setup(zcc.baseUrl, zcc.token);
				} else if(zcloud != NULL) {
					delete zcloud;
					zcloud = NULL;
				}
				config.ackZmartChargeConfig();
			}
			if(zcloud != NULL) {
				zcloud->update(meterState);
				if(zcloud->isConfigChanged()) {
					ZmartChargeConfig zcc;
					if(config.getZmartChargeConfig(zcc)) {
						const char* newBaseUrl = zcloud->getBaseUrl();
						memset(zcc.baseUrl, 0, 64);
						memcpy(zcc.baseUrl, newBaseUrl, strlen(newBaseUrl));
						config.setZmartChargeConfig(zcc);
						config.ackZmartChargeConfig();
					}
					zcloud->ackConfigChanged();
				}
			}
			#endif
			start = millis();
			handleUiLanguage();
			end = millis();
			if(end-start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to handle language update"), end-start);
			}
			start = millis();
			updater.loop();
			if(updater.isUpgradeInformationChanged()) {
				UpgradeInformation upinfo;
				updater.getUpgradeInformation(upinfo);
				config.setUpgradeInformation(upinfo);
				updater.ackUpgradeInformationChanged();
				if(mqttHandler != NULL)
					mqttHandler->publishFirmware();
				
				if(upinfo.errorCode == AMS_UPDATE_ERR_SUCCESS_SIGNAL) {
					debugW_P(PSTR("Rebooting to firmware version %s"), upinfo.toVersion);
					upinfo.errorCode == AMS_UPDATE_ERR_SUCCESS_CONFIRMED;
					config.setUpgradeInformation(upinfo);
					delay(1000);
					ESP.restart();
				}
			}
			end = millis();
			if(end-start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to handle firmware updater"), end-start);
			}
		}
		#if defined(ESP32)
		if(now - lastVoltageCheck > 1000) {
			start = millis();
			handleVoltageCheck();
			end = millis();
			lastVoltageCheck = now;
			if(end-start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to handle language update"), end-start);
			}
		}
		#endif
	} else {
		if(WiFi.smartConfigDone()) {
			debugI_P(PSTR("Smart config DONE!"));

			NetworkConfig network;
			config.getNetworkConfig(network);
			strcpy(network.ssid, WiFi.SSID().c_str());
			strcpy(network.psk, WiFi.psk().c_str());
			network.mode = 1;
			network.mdns = true;
			config.setNetworkConfig(network);

			SystemConfig sys;
			config.getSystemConfig(sys);
			sys.userConfigured = true;
			sys.dataCollectionConsent = 0;
			config.setSystemConfig(sys);
			config.save();

			delay(1000);
			ESP.restart();
		}
		if(dnsServer != NULL) {
			dnsServer->processNextRequest();
		}
		ws.loop();
	}

	if(config.isMeterChanged()) {
		config.getMeterConfig(meterConfig);
		if(meterConfig.source == METER_SOURCE_GPIO) {
			switch(meterConfig.parser) {
				case METER_PARSER_PASSIVE:
					if(pulseMc != NULL) {
						delete pulseMc;
						pulseMc = NULL;
					}
					#if defined(AMS_KMP)
					if(kmpMc != NULL) {
						delete(kmpMc);
						kmpMc = NULL;
					}
					#endif
					if(passiveMc == NULL) {
						passiveMc = new PassiveMeterCommunicator(&Debug);
					}
					passiveMc->configure(meterConfig, tz);
					hwSerial = passiveMc->getHwSerial();
					mc = passiveMc;
					break;
				case METER_PARSER_KAMSTRUP:
					if(pulseMc != NULL) {
						delete pulseMc;
						pulseMc = NULL;
					}
					if(passiveMc != NULL) {
						delete(passiveMc);
						passiveMc = NULL;
					}
					#if defined(AMS_KMP)
					if(kmpMc == NULL) {
						kmpMc = new KmpCommunicator(&Debug);
					}
					kmpMc->configure(meterConfig);
					hwSerial = kmpMc->getHwSerial();
					mc = kmpMc;
					#endif
					break;
				case METER_PARSER_PULSE:
					#if defined(AMS_KMP)
					if(kmpMc != NULL) {
						delete(kmpMc);
						kmpMc = NULL;
					}
					#endif
					if(passiveMc != NULL) {
						delete(passiveMc);
						passiveMc = NULL;
					}
					if(pulseMc == NULL) {
						pulseMc = new PulseMeterCommunicator(&Debug);
					}
					pulseMc->configure(meterConfig, tz);
					attachInterrupt(digitalPinToInterrupt(meterConfig.rxPin), onPulse, RISING);

					mc = pulseMc;
					break;
				default:
					debugE_P(PSTR("Unknown meter parser selected: %d"), meterConfig.parser);
			}
			#if defined(ESP32)
				if(hwSerial != NULL) {
					hwSerial->onReceiveError(rxerr);
				}
			#endif
		} else {
			debugE_P(PSTR("Unknown meter source selected: %d"), meterConfig.source);
		}
		ws.setMeterConfig(meterConfig.distributionSystem, meterConfig.mainFuse, meterConfig.productionCapacity);
		config.ackMeterChanged();
	}

	if(config.isEnergyAccountingChanged()) {
		handleEnergyAccountingChanged();
	}
	try {
		start = millis();
		if(readHanPort() || now - meterState.getLastUpdateMillis() > 30000) {
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to read HAN port (true)"), millis()-start);
			}
			handleTemperature(now);
			handleSystem(now);
			hw.setBootSuccessful(true);
		} else {
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to read HAN port (false)"), millis()-start);
			}
		}
		if(millis() - meterState.getLastUpdateMillis() > 1800000 && !ds.isHappy(time(nullptr))) {
			handleClear(now);
		}
	} catch(const std::exception& e) {
		debugE_P(PSTR("Exception in readHanPort (%s)"), e.what());
		meterState.setLastError(METER_ERROR_EXCEPTION);
	}

	delay(10); // Needed for auto modem sleep
	start = millis();
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
	yield();

	end = millis();
	if(end-start > SLOW_PROC_TRIGGER_MS) {
		debugW_P(PSTR("Used %dms to feed WDT"), end-start);
	}

	if(end-now > SLOW_PROC_TRIGGER_MS*2) {
		debugW_P(PSTR("loop() used %dms"), end-now);
	}
}

void handleUiLanguage() {
	if(config.isUiLanguageChanged()) {
		debugD_P(PSTR("Language has changed"));
		if(LittleFS.begin()) {
			UiConfig ui;
			config.getUiConfig(ui);
			if(strlen(ui.language) == 0) {
				debugD_P(PSTR("No language set"));
				return;
			}
			snprintf_P((char*) commonBuffer, BUF_SIZE_COMMON, PSTR("http://hub.amsleser.no/hub/language/%s.json"),
				strlen(ui.language) > 0 ? ui.language : "en"
			);
			HTTPClient http;

			debugI_P(PSTR("Downloading %s"), commonBuffer);
			#if defined(ESP8266)
			WiFiClient client;
			client.setTimeout(5000);
			if(http.begin(client, (char*) commonBuffer)) {
			#elif defined(ESP32)
			if(http.begin((char*) commonBuffer)) {
			#endif
				int status = http.GET();

				#if defined(ESP32)
					esp_task_wdt_reset();
				#elif defined(ESP8266)
					ESP.wdtFeed();
				#endif

				if(status == HTTP_CODE_OK) {
					snprintf_P((char*) commonBuffer, BUF_SIZE_COMMON, PSTR("/translations-%s.json"), ui.language);
					File file = LittleFS.open((char*) commonBuffer, "w");
					size_t written = http.writeToStream(&file);
					file.close();
					if(written > 0) {
						debugD_P(PSTR("Success (%lu written)"), written);
					} else {
						debugW_P(PSTR("Failed to write language '%s' (%d written)"), ui.language, written);
					}
				} else {
					debugW_P(PSTR("Failed to download language '%s'"), ui.language);
				}
				http.end();
			}
		}

		config.ackUiLanguageChange();
	}
}

void handleClear(unsigned long now) {
	time_t ts = time(nullptr);
	tmElements_t tm;
	breakTime(ts, tm);
	if(tm.Minute == 0) {
		AmsData nullData;
		debugI_P(PSTR("Clearing data that have not been updated"));
		ds.update(&nullData, ts);
	}
}

void handleEnergyAccountingChanged() {
	EnergyAccountingConfig *eac = ea.getConfig();
	config.getEnergyAccountingConfig(*eac);
	ea.setup(&ds, eac);
	config.ackEnergyAccountingChange();
	#if defined(_CLOUDCONNECTOR_H)
	if(cloud != NULL) {
		cloud->setEnergyAccountingConfig(*eac);
	}
	#endif
}

char ntpServerName[64] = "";

void handleNtpChange() {
	NtpConfig ntp;
	if(config.getNtpConfig(ntp)) {
		tz = resolveTimezone(ntp.timezone);
		if(ntp.enable && strlen(ntp.server) > 0) {
			strcpy(ntpServerName, ntp.server);
		} else if(ntp.enable) {
			strcpy(ntpServerName, "pool.ntp.org");
		} else {
			memset(ntpServerName, 0, 64);
		}
		configTime(tz->toLocal(0), tz->toLocal(JULY1970)-JULY1970, ntpServerName, "", "");
		sntp_servermode_dhcp(ntp.enable && ntp.dhcp ? 1 : 0); // Not implemented on ESP32?
		ntpEnabled = ntp.enable;

		ws.setTimezone(tz);
		ds.setTimezone(tz);
		ea.setTimezone(tz);
		ps->setTimezone(tz);
	}

	config.ackNtpChange();
}

void handleSystem(unsigned long now) {
	if(config.isSystemConfigChanged()) {
		config.getSystemConfig(sysConfig);
		config.ackSystemConfigChanged();
	}

	unsigned long start, end;
	if(now - lastSysupdate > 60000) {
		start = millis();
		if(WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED) {
			if(mqttHandler != NULL) {
				mqttHandler->publishSystem(&hw, ps, &ea);
				mqttHandler->publishFirmware();
			}
			#if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
			if(energySpeedometer != NULL) {
				energySpeedometer->publishSystem(&hw, ps, &ea);
			}
			#endif
		}
		lastSysupdate = now;
		end = millis();
		if(end - start > SLOW_PROC_TRIGGER_MS) {
			debugW_P(PSTR("Used %dms to send system update to MQTT"), millis()-start);
		}

		#if defined(ESP32)
		if(dnsState == 1) {
			const ip_addr_t* dns = dns_getserver(0);
			if(memcmp(&dns0, dns, sizeof(dns0)) != 0) {
					dns_setserver(0, &dns0);
					debugI_P(PSTR("Had to reset DNS server"));
			}
		}
		#endif
	}
}

bool handleVoltageCheck() {
	if(!hw.isVoltageOptimal()) {
		if(WiFi.getMode() == WIFI_STA) {
			debugW_P(PSTR("Vcc dropped below limit, disconnecting WiFi for 5 seconds to preserve power"));
			ch->disconnect(5000);
		}
		return false;
	}
	return true;
}

void handleTemperature(unsigned long now) {
	unsigned long start, end;
	if(now - lastTemperatureRead > 15000) {
		start = millis();
		if(hw.updateTemperatures()) {
			lastTemperatureRead = now;

			if(mqttHandler != NULL && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED) {
				mqttHandler->publishTemperatures(&config, &hw);
			}
		}
		end = millis();
		if(end - start > SLOW_PROC_TRIGGER_MS) {
			debugW_P(PSTR("Used %dms to update temperature"), millis()-start);
		}
	}
}

void handlePriceService(unsigned long now) {
	unsigned long start, end;
	if(ps != NULL && ntpEnabled) {
		start = millis();
		if(ps->loop() && mqttHandler != NULL) {
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to update prices"), millis()-start);
			}

			start = millis();
			mqttHandler->publishPrices(ps);
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to publish prices to MQTT"), millis()-start);
			}
		} else {
			end = millis();
			if(end - start > SLOW_PROC_TRIGGER_MS) {
				debugW_P(PSTR("Used %dms to handle price API"), millis()-start);
			}
		}
	}
	
	if(config.isPriceServiceChanged()) {
		PriceServiceConfig price;
		if(config.getPriceServiceConfig(price) && price.enabled && strlen(price.area) > 0) {
			if(ps == NULL) {
				ps = new PriceService(&Debug);
				ea.setPriceService(ps);
				ws.setPriceService(ps);
				#if defined(_CLOUDCONNECTOR_H)
				if(cloud != NULL) {
					cloud->setPriceConfig(price);
				}
				#endif
			}
			ps->setup(price);
		} else if(ps != NULL) {
			delete ps;
			ps = NULL;
			ws.setPriceService(NULL);
		}
		ws.setPriceSettings(price.area, price.currency);
		config.ackPriceServiceChange();
		ea.setCurrency(price.currency);
	}
}

void handleButton(unsigned long now) {
	if(gpioConfig.apPin != 0xFF) {
		if (digitalRead(gpioConfig.apPin) == LOW) {
			if (buttonActive == false) {
				buttonActive = true;
				buttonTimer = now;
			}

			if ((now - buttonTimer > longPressTime) && (longPressActive == false)) {
				longPressActive = true;
				debugD_P(PSTR("Button was held, triggering setup mode"));
				toggleSetupMode();
			}
		} else {
			if (buttonActive == true) {
				if (longPressActive == true) {
					longPressActive = false;
				} else {
					// Single press action
					debugD_P(PSTR("Button was clicked, no action configured"));
				}
				buttonActive = false;
			}
		}
	}
}

void errorBlink() {
	if(lastError == 3)
		lastError = 0;
	lastErrorBlink = millis64();
	if(setupMode) return;
	while(lastError < 3) {
		switch(lastError++) {
			case 0:
				if(lastErrorBlink - meterState.getLastUpdateMillis() > 30000) {
					debugW_P(PSTR("No HAN data received last 30s, single blink"));
					hw.ledBlink(LED_RED, 1); // If no message received from AMS in 30 sec, blink once
					if(meterState.getLastError() == 0) meterState.setLastError(METER_ERROR_NO_DATA);
					return;
				}
				break;
			case 1:
				if(mqttHandler != NULL && mqttHandler->lastError() != 0) {
					debugW_P(PSTR("MQTT connection not available, double blink"));
					hw.ledBlink(LED_RED, 2); // If MQTT error, blink twice
					return;
				}
				break;
			case 2:
				if(WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
					debugW_P(PSTR("WiFi not connected, tripe blink"));
					hw.ledBlink(LED_RED, 3); // If WiFi not connected, blink three times
					return;
				}
				break;
		}
	}
}

unsigned long lastConnectRetry = 0;

void connectToNetwork() {
	if(lastConnectRetry > 0 && (millis() - lastConnectRetry) < 10000) {
		delay(50);
		return;
	}
	lastConnectRetry = millis();
	if(!handleVoltageCheck()) {
		debugW_P(PSTR("Voltage is not high enough to reconnect"));
		return;
	}
	NetworkConfig network;
	if(config.getNetworkConfig(network)) {
		if(network.mode == 0 || network.mode > 3) network.mode = NETWORK_MODE_WIFI_CLIENT;
		if(ch != NULL && ch->getMode() != network.mode) {
			delete ch;
			ch = NULL;
		}
		switch(network.mode) {
			case NETWORK_MODE_WIFI_CLIENT:
				if(ch == NULL) {
					ch = new WiFiClientConnectionHandler(&Debug);
				}
				break;
			case NETWORK_MODE_WIFI_AP:
				if(ch == NULL) {
					ch = new WiFiAccessPointConnectionHandler(&Debug);
				}
				break;
			case NETWORK_MODE_ETH_CLIENT:
				if(ch == NULL) {
					ch = new EthernetConnectionHandler(&Debug);
				}
				break;
			default:
				setupMode = false;
				toggleSetupMode();
		}
		ch->connect(network, sysConfig);
		ws.setConnectionHandler(ch);
		#if defined(_CLOUDCONNECTOR_H)
		if(cloud != NULL)
			cloud->setConnectionHandler(ch);
		#endif
	} else {
		setupMode = false;
		toggleSetupMode();
	}
}

void toggleSetupMode() {
	if(!hw.ledOn(LED_YELLOW)) {
		hw.ledOn(LED_INTERNAL);
	}
	if(dnsServer != NULL) {
		dnsServer->stop();
	}
	WiFi.stopSmartConfig();
	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_OFF);
	delay(10);
	yield();

	if (!setupMode || !config.hasConfig()) {
		debugI_P(PSTR("Entering setup mode"));

		//wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, 0); // Disable default gw

		/* Example code to set captive portal option in DHCP
		auto& server = WiFi.softAPDhcpServer();
		server.onSendOptions([](const DhcpServer& server, auto& options) {
			// Captive Portal URI
			const IPAddress gateway = netif_ip4_addr(server.getNetif());
			const String captive = F("http://") + gateway.toString();
			options.add(114, captive.c_str(), captive.length());
		});
		*/

		WiFi.mode(WIFI_AP_STA);
		#if defined(ESP32) && defined(AMS2MQTT_SC_KEY)
		WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH_V2, AMS2MQTT_SC_KEY);
		#else
		WiFi.beginSmartConfig();
		#endif
		WiFi.softAP(PSTR("NEAS-WATTUP"));

		if(dnsServer == NULL) {
			dnsServer = new DNSServer();
		}
		dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
		dnsServer->start(53, PSTR("*"), WiFi.softAPIP());
		#if defined(DEBUG_MODE)
			Debug.setSerialEnabled(true);
			Debug.begin(F("192.168.4.1"), 23, RemoteDebug::VERBOSE);
		#endif
		setupMode = true;

		hw.setBootSuccessful(false);
		if(gpioConfig.ledDisablePin != 0xFF) {
			digitalWrite(gpioConfig.ledDisablePin, LOW);
		}
	} else {
		debugI_P(PSTR("Exiting setup mode"));
		if(dnsServer != NULL) {
			delete dnsServer;
			dnsServer = NULL;
		}
		connectToNetwork();
		setupMode = false;
		delay(500);
		if(!hw.ledOff(LED_YELLOW)) {
			hw.ledOff(LED_INTERNAL);
		}
		hw.setBootSuccessful(true);
	}
}

bool readHanPort() {
	if(mc == NULL) return false;
	if(pulseMc != NULL) {
		pulseMc->onPulse(pulses);
		pulses = 0;
		if(meterState.getListType() < 3) {
			time_t now = time(nullptr);
			if(now > FirmwareVersion::BuildEpoch) {
				ImpulseAmsData init = ImpulseAmsData(ds.getEstimatedImportCounter());
				meterState.apply(init);
			}
		}
	}
	if(!mc->loop()) {
		meterState.setLastError(mc->getLastError());
		return false;
	}
	if(mc->isConfigChanged()) {
		mc->getCurrentConfig(meterConfig);
		config.setMeterConfig(meterConfig);
		mc->ackConfigChanged();
	}
	meterState.setLastError(mc->getLastError());

	AmsData* data = mc->getData(meterState);

	if(data != NULL) {
		if(data->getListType() > 0) {
			handleDataSuccess(data);
		} else {
			meterState.setLastError(METER_ERROR_UNKNOWN_DATA);
		}
		delete data;
	}
	yield();
	return true;
}

void handleDataSuccess(AmsData* data) {
	if(!setupMode && !hw.ledBlink(LED_GREEN, 1))
		hw.ledBlink(LED_INTERNAL, 1);

	if(mqttHandler != NULL) {
		#if defined(ESP32)
			esp_task_wdt_reset();
		#elif defined(ESP8266)
			ESP.wdtFeed();
		#endif
		yield();
		if(mqttHandler->publish(data, &meterState, &ea, ps)) {
			delay(10);
		}
	}
	#if defined(ESP32) && defined(ENERGY_SPEEDOMETER_PASS)
	if(energySpeedometer != NULL && energySpeedometer->publish(&meterState, &meterState, &ea, ps)) {
		delay(10);
	}
	#endif

	time_t now = time(nullptr);
	time_t meterTime = data->getMeterTimestamp();
	if(now < FirmwareVersion::BuildEpoch && data->getListType() >= 3) {
		if(meterTime > FirmwareVersion::BuildEpoch) {
			debugI_P(PSTR("Using timestamp from meter"));
			now = meterTime;
		} else if(data->getPackageTimestamp() > FirmwareVersion::BuildEpoch) {
			debugI_P(PSTR("Using timestamp from meter (DLMS)"));
			now = data->getPackageTimestamp();
		}
		if(now > FirmwareVersion::BuildEpoch) {
			timeval tv { now, 0};
			settimeofday(&tv, nullptr);
		}
	}

	bool wasCounterEstimated = meterState.isCounterEstimated();
	meterState.apply(*data);
	rtp.update(meterState);

	time_t dataUpdateTime = now;
	if(abs(now - meterTime) < 300) {
		dataUpdateTime = meterTime;
	}

	tmElements_t tm, mtm;
	breakTime(now, tm);
	breakTime(meterTime, mtm);

	bool saveData = false;
	if(!ds.isHappy(dataUpdateTime) && dataUpdateTime > FirmwareVersion::BuildEpoch) { // Must use "isHappy()" in case day state gets reset and lastTimestamp is "now"
		debugD_P(PSTR("READY to update (internal clock %02d:%02d:%02d UTC, meter clock: %02d:%02d:%02d, list type %d, est: %d, using clock: %d)"), tm.Hour, tm.Minute, tm.Second, mtm.Hour, mtm.Minute, mtm.Second, data->getListType(), wasCounterEstimated, dataUpdateTime == now);
		tmElements_t dtm;
		breakTime(dataUpdateTime, dtm);
		if(dtm.Minute < 2 && data->getListType() >= 3) {
			debugD_P(PSTR("Updating data storage using actual data"));
			saveData = ds.update(data, dataUpdateTime);

			#if defined(_CLOUDCONNECTOR_H)
			if(saveData && cloud != NULL) cloud->forceUpdate();
			#endif
		} else if(dtm.Minute == 2) {
			debugW_P(PSTR("Did not receive necessary data for previous hour, clearing"));
			AmsData nullData;
			saveData = ds.update(&nullData, dataUpdateTime);
		}
		if(saveData) {
			debugI_P(PSTR("Saving data"));
			if(!ds.save()) {
				debugW_P(PSTR("Unable to save data storage"));
			}
		}
	} else {
		debugD_P(PSTR("NOT Ready to update (internal clock %02d:%02d:%02d UTC, meter clock: %02d:%02d:%02d, list type %d, est: %d)"), tm.Hour, tm.Minute, tm.Second, mtm.Hour, mtm.Minute, mtm.Second, data->getListType(), wasCounterEstimated);
	}

	if(ea.update(data)) {
		debugI_P(PSTR("Saving energy accounting"));
		if(!ea.save()) {
			debugW_P(PSTR("Unable to save energy accounting"));
		}
	}
}

void postConnect() {
	networkConnected = true;

	NetworkConfig network;
	ch->getCurrentConfig(network);
	if(ch->isConfigChanged()) {
		config.setNetworkConfig(network);
	}
	WebConfig web;
	#if defined(AMS_REMOTE_DEBUG)
	if(config.getWebConfig(web) && web.security > 0) {
		Debug.setPassword(web.password);
	}
	DebugConfig debug;
	if(config.getDebugConfig(debug)) {
		Debug.begin(network.hostname, debug.serial || debug.telnet ? (uint8_t) debug.level : RemoteDebug::WARNING); // I don't know why, but ESP8266 stops working after a while if ERROR level is set
		if(!debug.telnet) {
			Debug.stop();
		}
	} else {
		Debug.stop();
	}
	#endif
	mdnsEnabled = false;
	if(strlen(network.hostname) > 0 && network.mdns) {
		debugD_P(PSTR("mDNS is enabled, using host: %s"), network.hostname);
		if(MDNS.begin(network.hostname)) {
			mdnsEnabled = true;
			MDNS.addService(F("http"), F("tcp"), 80);
		} else {
			debugE_P(PSTR("Failed to set up mDNS!"));
		}
	}

	MqttConfig mqttConfig;
	if(config.getMqttConfig(mqttConfig)) {
		mqttEnabled = strlen(mqttConfig.host) > 0;
		ws.setMqttEnabled(mqttEnabled);
	}

	sprintf_P((char*) commonBuffer, PSTR("AMS reader %s"), network.hostname);

	SSDP.setSchemaURL("ssdp/schema.xml");
	SSDP.setHTTPPort(80);
	SSDP.setName((char*) commonBuffer);
	//SSDP.setSerialNumber("0");
	SSDP.setURL("/");
	SSDP.setModelName("AMS reader");
	//SSDP.setModelNumber("929000226503");
	SSDP.setModelURL("https://amsleser.no");
	SSDP.setManufacturer("Utilitech AS");
	SSDP.setManufacturerURL("http://amsleser.no");
	SSDP.setDeviceType("rootdevice");
	sprintf_P((char*) commonBuffer, PSTR("amsreader/%s"), FirmwareVersion::VersionString);
	#if defined(ESP32)
	SSDP.setModelDescription("Device to read data from electric smart meters");
	SSDP.setServerName((char*) commonBuffer);
	//SSDP.setUUID("");
	SSDP.setIcons(  "<icon>"
					"<mimetype>image/svg+xml</mimetype>"
					"<height>48</height>"
					"<width>48</width>"
					"<depth>24</depth>"
					"<url>favicon.svg</url>"
					"</icon>");
	#endif
	SSDP.setInterval(300);
	SSDP.begin();
}


unsigned long lastMqttRetry = -20000;
void MQTT_connect() {
	if(millis() - lastMqttRetry < (config.isMqttChanged() ? 5000 : 30000)) {
		yield();
		return;
	}
	lastMqttRetry = millis();

	MqttConfig mqttConfig;
	if(!config.getMqttConfig(mqttConfig) || strlen(mqttConfig.host) == 0) {
		debugW_P(PSTR("No MQTT config"));
		ws.setMqttEnabled(false);
		mqttEnabled = false;
		return;
	}
	mqttEnabled = true;
	ws.setMqttEnabled(true);

	if(mqttHandler != NULL) {
		mqttHandler->disconnect();
		if(mqttHandler->getFormat() != mqttConfig.payloadFormat) {
			delete mqttHandler;
			mqttHandler = NULL;
		} else if(config.isMqttChanged()) {
			mqttHandler->setConfig(mqttConfig);
			switch(mqttConfig.payloadFormat) {
				case 3: {
					DomoticzConfig domo;
					config.getDomoticzConfig(domo);
					DomoticzMqttHandler* dmh = (DomoticzMqttHandler*) &mqttHandler;
					dmh->setDomoticzConfig(domo);
					break;
				}
				case 4: {
					HomeAssistantConfig haconf;
					config.getHomeAssistantConfig(haconf);
					HomeAssistantMqttHandler* hamh = (HomeAssistantMqttHandler*) &mqttHandler;
					hamh->setHomeAssistantConfig(haconf);
					break;
				}
			}
		}
	}

	if(mqttHandler == NULL) {
		switch(mqttConfig.payloadFormat) {
			case 0:
			case 5:
			case 6:
				mqttHandler = new JsonMqttHandler(mqttConfig, &Debug, (char*) commonBuffer, &hw, &updater);
				break;
			case 1:
			case 2:
				mqttHandler = new RawMqttHandler(mqttConfig, &Debug, (char*) commonBuffer, &updater);
				break;
			case 3:
				DomoticzConfig domo;
				config.getDomoticzConfig(domo);
				mqttHandler = new DomoticzMqttHandler(mqttConfig, &Debug, (char*) commonBuffer, domo, &updater);
				break;
			case 4:
				HomeAssistantConfig haconf;
				config.getHomeAssistantConfig(haconf);
				mqttHandler = new HomeAssistantMqttHandler(mqttConfig, &Debug, (char*) commonBuffer, sysConfig.boardType, haconf, &hw, &updater);
				break;
			case 255:
				mqttHandler = new PassthroughMqttHandler(mqttConfig, &Debug, (char*) commonBuffer, &updater);
				break;
		}
	}
	ws.setMqttHandler(mqttHandler);

	if(mqttHandler != NULL) {
		mqttHandler->connect();
		mqttHandler->publishSystem(&hw, ps, &ea);
		if(ps != NULL && ps->getValueForHour(PRICE_DIRECTION_IMPORT, 0) != PRICE_NO_VALUE) {
			mqttHandler->publishPrices(ps);
		}
	}
}

String getSplit(String input, int index) {
	char separator = ' ';
	String ret = input;
	ret.trim();
	for(int i = 0; i < index; i++) {
		int pos = ret.indexOf(separator);
		if(pos == -1) {
			return "";
		}
		ret = ret.substring(pos+1);
	}
	ret = ret.substring(0, ret.indexOf(separator));
	ret.trim();
	return ret;
}

void configFileParse() {
	debugD_P(PSTR("Parsing config file"));

	if(!LittleFS.exists(FILE_CFG)) {
		debugW_P(PSTR("Config file does not exist"));
		return;
	}

	File file = LittleFS.open(FILE_CFG, (char*) "r");

	bool lSys = false;
	bool lNetwork = false;
	bool lMqtt = false;
	bool lWeb = false;
	bool lMeter = false;
	bool lGpio = false;
	bool lDomo = false;
	bool lHa = false;
	bool lNtp = false;
	bool lPrice = false;
	bool lEac = false;
	bool sEa = false;
	bool sDs = false;

	ds.load();

	SystemConfig sys;
	NetworkConfig network;
	MqttConfig mqtt;
	WebConfig web;
	MeterConfig meter;
	GpioConfig gpio;
	DomoticzConfig domo;
	HomeAssistantConfig haconf;
	NtpConfig ntp;
	PriceServiceConfig price;
	EnergyAccountingConfig eac;
	uint8_t priceModifierCount = 0;

	size_t size;
	char* buf = (char*) commonBuffer;
	memset(buf, 0, 1024);
	while((size = file.readBytesUntil('\n', buf, 1024)) > 0) {
		for(uint16_t i = 0; i < size; i++) {
			if(buf[i] < 32 || buf[i] > 126) {
				memset(buf+i, 0, size-i);
				debugD_P(PSTR("Found non-ascii, shortening line from %d to %d"), size, i);
				size = i;
				break;
			}
		}
		if(strncmp_P(buf, PSTR("boardType "), 10) == 0) {
			if(!lSys) { config.getSystemConfig(sys); lSys = true; };
			if(sys.boardType == 0xFF) {
				sys.boardType = String(buf+10).toInt();
			}
		} else if(strncmp_P(buf, PSTR("netmode "), 8) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			network.mode = String(buf+8).toInt();
		} else if(strncmp_P(buf, PSTR("ssid "), 5) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.ssid, buf+5);
		} else if(strncmp_P(buf, PSTR("psk "), 4) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.psk, buf+4);
		} else if(strncmp_P(buf, PSTR("ip "), 3) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.ip, buf+3);
		} else if(strncmp_P(buf, PSTR("gateway "), 8) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.gateway, buf+8);
		} else if(strncmp_P(buf, PSTR("subnet "), 7) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.subnet, buf+7);
		} else if(strncmp_P(buf, PSTR("dns1 "), 5) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.dns1, buf+5);
		} else if(strncmp_P(buf, PSTR("dns2 "), 5) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.dns2, buf+5);
		} else if(strncmp_P(buf, PSTR("hostname "), 9) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			strcpy(network.hostname, buf+9);
		} else if(strncmp_P(buf, PSTR("use11b "), 7) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			network.use11b = String(buf+7).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("mdns "), 5) == 0) {
			if(!lNetwork) { config.getNetworkConfig(network); lNetwork = true; };
			network.mdns = String(buf+5).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("mqttHost "), 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.host, buf+9);
		} else if(strncmp_P(buf, PSTR("mqttPort "), 9) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.port = String(buf+9).toInt();
		} else if(strncmp_P(buf, PSTR("mqttClientId "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.clientId, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPublishTopic "), 17) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.publishTopic, buf+17);
		} else if(strncmp_P(buf, PSTR("mqttSubscribeTopic "), 19) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.subscribeTopic, buf+19);
		} else if(strncmp_P(buf, PSTR("mqttUsername "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.username, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPassword "), 13) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			strcpy(mqtt.password, buf+13);
		} else if(strncmp_P(buf, PSTR("mqttPayloadFormat "), 18) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.payloadFormat = String(buf+18).toInt();
		} else if(strncmp_P(buf, PSTR("mqttSsl "), 8) == 0) {
			if(!lMqtt) { config.getMqttConfig(mqtt); lMqtt = true; };
			mqtt.ssl = String(buf+8).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("webSecurity "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			web.security = String(buf+12).toInt();
		} else if(strncmp_P(buf, PSTR("webUsername "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			strcpy(web.username, buf+12);
		} else if(strncmp_P(buf, PSTR("webPassword "), 12) == 0) {
			if(!lWeb) { config.getWebConfig(web); lWeb = true; };
			strcpy(web.password, buf+12);
		} else if(strncmp_P(buf, PSTR("meterBaud "), 10) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.baud = String(buf+10).toInt();
		} else if(strncmp_P(buf, PSTR("meterParity "), 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.parity = 0;
			if(strncmp_P(buf+12, PSTR("7N1"), 3) == 0) meter.parity = 2;
			if(strncmp_P(buf+12, PSTR("8N1"), 3) == 0) meter.parity = 3;
			if(strncmp_P(buf+12, PSTR("8N2"), 3) == 0) meter.parity = 7;
			if(strncmp_P(buf+12, PSTR("7E1"), 3) == 0) meter.parity = 10;
			if(strncmp_P(buf+12, PSTR("8E1"), 3) == 0) meter.parity = 11;
		} else if(strncmp_P(buf, PSTR("meterInvert "), 12) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.invert = String(buf+12).toInt() == 1;;
		} else if(strncmp_P(buf, PSTR("meterDistributionSystem "), 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.distributionSystem = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("meterMainFuse "), 14) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.mainFuse = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("meterProductionCapacity "), 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.productionCapacity = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("meterEncryptionKey "), 19) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.encryptionKey, String(buf+19), 16);
		} else if(strncmp_P(buf, PSTR("meterAuthenticationKey "), 23) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			fromHex(meter.authenticationKey, String(buf+23), 16);
		} else if(strncmp_P(buf, PSTR("meterWattageMultiplier "), 23) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.wattageMultiplier = String(buf+23).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("meterVoltageMultiplier "), 23) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.voltageMultiplier = String(buf+23).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("meterAmperageMultiplier "), 24) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.amperageMultiplier = String(buf+24).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("meterAccumulatedMultiplier "), 27) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.accumulatedMultiplier = String(buf+27).toDouble() * 1000;
		} else if(strncmp_P(buf, PSTR("gpioHanPin "), 11) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.rxPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioHanPinPullup "), 17) == 0) {
			if(!lMeter) { config.getMeterConfig(meter); lMeter = true; };
			meter.rxPinPullup = String(buf+17).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("gpioApPin "), 10) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.apPin = String(buf+10).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPin "), 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedInverted "), 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledInverted = String(buf+16).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("gpioLedPinRed "), 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinRed = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPinGreen "), 16) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinGreen = String(buf+16).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedPinBlue "), 15) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledPinBlue = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("gpioLedRgbInverted "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.ledRgbInverted = String(buf+19).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("gpioTempSensorPin "), 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempSensorPin = String(buf+18).toInt();
		} else if(strncmp_P(buf, PSTR("gpioTempAnalogSensorPin "), 24) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.tempAnalogSensorPin = String(buf+24).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccPin "), 11) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccPin = String(buf+11).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccOffset "), 14) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccOffset = String(buf+14).toFloat() * 100;
		} else if(strncmp_P(buf, PSTR("gpioVccMultiplier "), 18) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccMultiplier = String(buf+18).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("gpioVccBootLimit "), 17) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccBootLimit = String(buf+17).toFloat() * 10;
		} else if(strncmp_P(buf, PSTR("gpioVccResistorGnd "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorGnd = String(buf+19).toInt();
		} else if(strncmp_P(buf, PSTR("gpioVccResistorVcc "), 19) == 0) {
			if(!lGpio) { config.getGpioConfig(gpio); lGpio = true; };
			gpio.vccResistorVcc = String(buf+19).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzElidx "), 14) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.elidx = String(buf+14).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl1idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl1idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl2idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl2idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzVl3idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.vl3idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("domoticzCl1idx "), 15) == 0) {
			if(!lDomo) { config.getDomoticzConfig(domo); lDomo = true; };
			domo.cl1idx = String(buf+15).toInt();
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryPrefix "), 29) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryPrefix, buf+29);
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryHostname "), 31) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryHostname, buf+31);
		} else if(strncmp_P(buf, PSTR("homeAssistantDiscoveryNameTag "), 30) == 0) {
			if(!lHa) { config.getHomeAssistantConfig(haconf); lHa = true; };
			strcpy(haconf.discoveryNameTag, buf+30);
		} else if(strncmp_P(buf, PSTR("ntpEnable "), 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.enable = String(buf+10).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("ntpDhcp "), 8) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			ntp.dhcp = String(buf+8).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("ntpServer "), 10) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			strcpy(ntp.server, buf+10);
		} else if(strncmp_P(buf, PSTR("ntpTimezone "), 12) == 0) {
			if(!lNtp) { config.getNtpConfig(ntp); lNtp = true; };
			strcpy(ntp.timezone, buf+12);
		} else if(strncmp_P(buf, PSTR("entsoeToken "), 12) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.entsoeToken, buf+12);
		} else if(strncmp_P(buf, PSTR("entsoeArea "), 11) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.area, buf+11);
		} else if(strncmp_P(buf, PSTR("entsoeCurrency "), 15) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.currency, buf+15);
		} else if(strncmp_P(buf, PSTR("entsoeMultiplier "), 17) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			price.unused1 = String(buf+17).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("entsoeFixedPrice "), 17) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			price.unused2 = String(buf+17).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("priceEnabled "), 13) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			price.enabled = String(buf+13).toInt() == 1;
		} else if(strncmp_P(buf, PSTR("priceEntsoeToken "), 17) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.entsoeToken, buf+17);
		} else if(strncmp_P(buf, PSTR("priceArea "), 10) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.area, buf+10);
		} else if(strncmp_P(buf, PSTR("priceCurrency "), 14) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			strcpy(price.currency, buf+14);
		} else if(strncmp_P(buf, PSTR("priceMultiplier "), 16) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			price.unused1 = String(buf+16).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("priceFixedPrice "), 16) == 0) {
			if(!lPrice) { config.getPriceServiceConfig(price); lPrice = true; };
			price.unused2 = String(buf+16).toFloat() * 1000;
		} else if(strncmp_P(buf, PSTR("priceModifier "), 14) == 0) {
			PriceConfig pc;
			memset(&pc, 0, sizeof(PriceConfig));

			String line = String(buf+14);

			uint8_t priceIndex = line.substring(0, line.indexOf(" ")).toInt();

			int nameStart = line.indexOf("\"");
			if(nameStart < 0) {
				debugW_P(PSTR("Price modifier without name start"));
				continue;
			}
			int nameEnd = line.indexOf("\"", nameStart+1);
			if(nameEnd < nameStart) {
				debugW_P(PSTR("Price modifier without name end"));
				continue;
			}
			String name = line.substring(nameStart+1, nameEnd);
			strcpy(pc.name, name.c_str());
			String rest = line.substring(nameEnd+1);

			String direction = getSplit(rest, 0);
			if(direction.equals("import")) {
				pc.direction = PRICE_DIRECTION_IMPORT;
			} else if(direction.equals("export")) {
				pc.direction = PRICE_DIRECTION_EXPORT;
			} else if(direction.equals("both")) {
				pc.direction = PRICE_DIRECTION_BOTH;
			} else {
				debugW_P(PSTR("Price modifier with unknown direction \"%s\""), direction.c_str());
				continue;
			}

			String type = getSplit(rest, 1);
			if(type.equals("fixed")) {
				pc.type = PRICE_TYPE_FIXED;
			} else if(type.equals("add")) {
				pc.type = PRICE_TYPE_ADD;
			} else if(type.equals("percent")) {
				pc.type = PRICE_TYPE_PCT;
			} else if(type.equals("subtract")) {
				pc.type = PRICE_TYPE_SUBTRACT;
			} else {
				debugW_P(PSTR("Price modifier unknown type"));
				continue;
			}

			pc.value = getSplit(rest, 2).toFloat() * 10000;

			String days = getSplit(rest, 3);
			if(days.equals("all")) {
				pc.days = 0x7F;
			} else {
				pc.days = 0;
				if(days.indexOf("mo") >= 0) pc.days |= 1 << 0;
				if(days.indexOf("tu") >= 0) pc.days |= 1 << 1;
				if(days.indexOf("we") >= 0) pc.days |= 1 << 2;
				if(days.indexOf("th") >= 0) pc.days |= 1 << 3;
				if(days.indexOf("fr") >= 0) pc.days |= 1 << 4;
				if(days.indexOf("sa") >= 0) pc.days |= 1 << 5;
				if(days.indexOf("su") >= 0) pc.days |= 1 << 6;
			}

			String hours = getSplit(rest, 4);
			if(hours.equals("all")) {
				pc.hours = 0xFFFFFF;
			} else {
				pc.hours = 0;
				for(uint8_t i = 0; i < 24; i++) {
					char h[4];
					snprintf_P(h, 4, PSTR("%02d"), i);
					if(hours.indexOf(String(h)) >= 0) pc.hours |= 1 << i;
				}
			}

			String start = getSplit(rest, 5);
			pc.start_dayofmonth = start.substring(0, start.indexOf("-")).toInt();
			pc.start_month = start.substring(start.indexOf("-")+1).toInt();

			String end = getSplit(rest, 6);
			pc.end_dayofmonth = end.substring(0, end.indexOf("-")).toInt();
			pc.end_month = end.substring(end.indexOf("-")+1).toInt();

			ps->setPriceConfig(priceIndex, pc);
			priceModifierCount = priceIndex+1;
		} else if(strncmp_P(buf, PSTR("thresholds "), 11) == 0) {
			if(!lEac) { config.getEnergyAccountingConfig(eac); lEac = true; };
			int i = 0;
			char * pch = strtok (buf+11," ");
			while (pch != NULL && i < 10) {
				eac.thresholds[i++] = String(pch).toInt();
				pch = strtok (NULL, " ");
			}
			eac.hours = String(pch).toInt();
		} else if(strncmp_P(buf, PSTR("dayplot "), 8) == 0) {
			int i = 0;
			DayDataPoints day = { 0 };
			char * pch = strtok (buf+8," ");
			while (pch != NULL) {
				double val = String(pch).toDouble();
				if(day.version < 5) {
					if(i == 0) {
						day.version = val;
					} else if(i == 1) {
						day.lastMeterReadTime = val;
					} else if(i == 2) {
						day.activeImport = val;
					} else if(i > 2 && i < 27) {
						day.hImport[i-3] = val / 10;
					} else if(i == 27) {
						day.activeExport = val;
					} else if(i > 27 && i < 52) {
						day.hExport[i-28] = val / 10;
					}
				} else {
					if(i == 1) {
						day.lastMeterReadTime = val;
					} else if(i == 2) {
						day.activeImport = day.version > 5 ? val * 1000 : val;
					} else if(i == 3) {
						day.accuracy = val;
					} else if(i > 3 && i < 28) {
						day.hImport[i-4] = val / pow(10, day.accuracy);
					} else if(i == 28) {
						day.activeExport = day.version > 5 ? val * 1000 : val;
					} else if(i > 28 && i < 53) {
						day.hExport[i-29] = val / pow(10, day.accuracy);
					}
				}

				pch = strtok (NULL, " ");
				i++;
			}
			ds.setDayData(day);
			sDs = true;
		} else if(strncmp_P(buf, PSTR("monthplot "), 10) == 0) {
			int i = 0;
			MonthDataPoints month = { 0 };
			char * pch = strtok (buf+10," ");
			while (pch != NULL) {
				double val = String(pch).toDouble();
				if(month.version < 6) {
					if(i == 0) {
						month.version = val;
					} else if(i == 1) {
						month.lastMeterReadTime = val;
					} else if(i == 2) {
						month.activeImport = val;
					} else if(i > 2 && i < 34) {
						month.dImport[i-3] = val / 10;
					} else if(i == 34) {
						month.activeExport = val;
					} else if(i > 34 && i < 66) {
						month.dExport[i-35] = val / 10;
					}
				} else {
					if(i == 1) {
						month.lastMeterReadTime = val;
					} else if(i == 2) {
						month.activeImport = month.version > 6 ? val * 1000 : val;
					} else if(i == 3) {
						month.accuracy = val;
					} else if(i > 3 && i < 35) {
						month.dImport[i-4] = val / pow(10, month.accuracy);
					} else if(i == 35) {
						month.activeExport = month.version > 6 ? val * 1000 : val;
					} else if(i > 35 && i < 67) {
						month.dExport[i-36] = val / pow(10, month.accuracy);
					}
				}
				pch = strtok (NULL, " ");
				i++;
			}
			ds.setMonthData(month);
			sDs = true;
		} else if(strncmp_P(buf, PSTR("energyaccounting "), 17) == 0) {
			uint8_t i = 0;
			EnergyAccountingData ead = { 0, 0, 
                0, 0, 0, // Cost
                0, 0, 0, // Income
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0 // Peak 5
            };
			uint8_t peak = 0;
			uint64_t totalImport = 0, totalExport = 0;
			char * pch = strtok (buf+17," ");
			while (pch != NULL) {
				if(ead.version < 5) {
					if(i == 0) {
						long val = String(pch).toInt();
						ead.version = val;
					} else if(i == 1) {
						long val = String(pch).toInt();
						ead.month = val;
					} else if(i == 2) {
						float val = String(pch).toFloat();
						if(val > 0.0) {
							ead.peaks[0] = { 1, (uint16_t) (val*100) };
						}
					} else if(i == 3) {
						float val = String(pch).toFloat();
						ead.costYesterday = val * 100;
					} else if(i == 4) {
						float val = String(pch).toFloat();
						ead.costThisMonth = val * 100;
					} else if(i == 5) {
						float val = String(pch).toFloat();
						ead.costLastMonth = val * 100;
					} else if(i >= 6 && i < 18) {
						uint8_t hour = i-6;					
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							float val = String(pch).toFloat();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					}
				} else {
					if(i == 1) {
						long val = String(pch).toInt();
						ead.month = val;
					} else if(i == 2) {
						float val = String(pch).toFloat();
						ead.costYesterday = val * 100;
					} else if(i == 3) {
						float val = String(pch).toFloat();
						ead.costThisMonth = val * 100;
					} else if(i == 4) {
						float val = String(pch).toFloat();
						ead.costLastMonth = val * 100;
					} else if(i == 5) {
						float val = String(pch).toFloat();
						ead.incomeYesterday= val * 100;
					} else if(i == 6) {
						float val = String(pch).toFloat();
						ead.incomeThisMonth = val * 100;
					} else if(i == 7) {
						float val = String(pch).toFloat();
						ead.incomeLastMonth = val * 100;
					} else if(i >= 8 && i < 18) {
						uint8_t hour = i-8;		
						{			
							long val = String(pch).toInt();
							ead.peaks[peak].day = val;
						} 
						pch = strtok (NULL, " ");
						i++;
						{
							float val = String(pch).toFloat();
							ead.peaks[peak].value = val * 100;
						}
						peak++;
					} else if(i == 18) {
						float val = String(pch).toFloat();
						totalImport = val * 1000;
					} else if(i == 19) {
						float val = String(pch).toFloat();
						totalExport = val * 1000;
					}
				}
				pch = strtok (NULL, " ");
				i++;
			}
            uint8_t accuracy = 0;
            uint64_t importUpdate = totalImport, exportUpdate = totalExport;
			while(importUpdate > UINT32_MAX || exportUpdate > UINT32_MAX) {
                accuracy++;
                importUpdate = totalImport / pow(10, accuracy);
                exportUpdate = totalExport / pow(10, accuracy);
            }
            ead.lastMonthImport = importUpdate;
            ead.lastMonthExport = exportUpdate;

			ead.version = 6;
			ea.setData(ead);
			sEa = true;
		}
		memset(buf, 0, 1024);
	}

	debugD_P(PSTR("Deleting config file"));
	file.close();
	if(!LittleFS.remove(FILE_CFG)) {
		debugW_P(PSTR("Unable to remove config file, formatting filesystem"));
		if(!sDs) {
			ds.load();
			sDs = true;
		}
		if(!sEa) {
			ea.load();
			sEa = true;
		}
		if(!LittleFS.format()) {
			debugE_P(PSTR("Unable to format broken filesystem"));
		}
	}

	debugI_P(PSTR("Saving configuration now..."));
	Serial.flush();
	if(lSys) config.setSystemConfig(sys);
	if(lNetwork) config.setNetworkConfig(network);
	if(lMqtt) config.setMqttConfig(mqtt);
	if(lWeb) config.setWebConfig(web);
	if(lMeter) config.setMeterConfig(meter);
	if(lGpio) config.setGpioConfig(gpio);
	if(lDomo) config.setDomoticzConfig(domo);
	if(lHa) config.setHomeAssistantConfig(haconf);
	if(lNtp) config.setNtpConfig(ntp);
	if(lPrice) config.setPriceServiceConfig(price);
	if(lEac) config.setEnergyAccountingConfig(eac);
	if(sDs) ds.save();
	if(sEa) ea.save();
	if(priceModifierCount > 0) ps->save();
	config.save();
	LittleFS.end();
}

void IRAM_ATTR onPulse() {
	pulses++;
}