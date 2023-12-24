#include "AmsMqttHandler.h"
#include "FirmwareVersion.h"
#include "AmsStorage.h"
#include "LittleFS.h"

void AmsMqttHandler::setCaVerification(bool caVerification) {
	this->caVerification = caVerification;
}

void AmsMqttHandler::setConfig(MqttConfig& mqttConfig) {
	this->mqttConfig = mqttConfig;
	this->mqttConfigChanged = true;
}

bool AmsMqttHandler::connect() {
	if(millis() - lastMqttRetry < 10000) {
		yield();
		return false;
	}
	lastMqttRetry = millis();

	time_t epoch = time(nullptr);
	
	WiFiClient *actualClient = NULL;

	if(mqttConfig.ssl) {
		if(epoch < FirmwareVersion::BuildEpoch) {
			if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("NTP not ready for MQTT SSL\n"));
			return false;
		}
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("MQTT SSL is configured (%dkb free heap)\n"), ESP.getFreeHeap());
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
			#if defined(ESP8266)
				mqttSecureClient->setBufferSizes(512, 512);
				if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("ESP8266 firmware does not have enough memory...\n"));
				return false;
			#endif
		}

		if(mqttConfigChanged) {
			if(caVerification && LittleFS.begin()) {
				File file;

				if(LittleFS.exists(FILE_MQTT_CA)) {
					if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT CA file (%dkb free heap)\n"), ESP.getFreeHeap());
					file = LittleFS.open(FILE_MQTT_CA, (char*) "r");
					#if defined(ESP8266)
						BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(file);
						mqttSecureClient->setTrustAnchors(serverTrustedCA);
					#elif defined(ESP32)
						if(mqttSecureClient->loadCACert(file, file.size())) {
							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("CA accepted\n"));
						} else {
							if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("CA was rejected\n"));
							return false;
						}
					#endif
					file.close();
				} else {
					if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("No CA, disabling validation\n"));
					mqttSecureClient->setInsecure();
				}

				#if defined(ESP8266)
					if(LittleFS.exists(FILE_MQTT_CERT) && LittleFS.exists(FILE_MQTT_KEY)) {
						if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT certificate file (%dkb free heap)\n"), ESP.getFreeHeap());
						file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
						BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
						file.close();

						if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT key file (%dkb free heap)\n"), ESP.getFreeHeap());
						file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
						BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
						file.close();

						if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Loading cert and key (%dkb free heap)\n"), ESP.getFreeHeap());
						mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
					}
				#endif

				#if defined(ESP32)
					if(LittleFS.exists(FILE_MQTT_CERT)) {
						if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT certificate file (%dkb free heap)\n"), ESP.getFreeHeap());
						file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
						mqttSecureClient->loadCertificate(file, file.size());
						file.close();
					}
				
					if(LittleFS.exists(FILE_MQTT_KEY)) {
						if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT key file (%dkb free heap)\n"), ESP.getFreeHeap());
						file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
						mqttSecureClient->loadPrivateKey(file, file.size());
						file.close();
					}
				#endif

				LittleFS.end();
			} else {
				if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("CA verification disabled\n"));
				mqttSecureClient->setInsecure();
			}
		}
		actualClient = mqttSecureClient;

		if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("MQTT SSL setup complete (%dkb free heap)\n"), ESP.getFreeHeap());
	} else {
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("No SSL, using client without SSL support\n"));
		if(mqttClient == NULL) {
			mqttClient = new WiFiClient();
		}
		actualClient = mqttClient;
	}

	mqttConfigChanged = false;
    if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Connecting to MQTT %s:%d\n"), mqttConfig.host, mqttConfig.port);
	mqtt.begin(mqttConfig.host, mqttConfig.port, *actualClient);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("Setting NTP time %lu for secure MQTT connection\n"), epoch);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt.connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Successfully connected to MQTT\n"));
		mqtt.onMessage(std::bind(&AmsMqttHandler::onMessage, this, std::placeholders::_1, std::placeholders::_2));
		if(strlen(mqttConfig.subscribeTopic) > 0) {
			if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("  Subscribing to [%s]\n"), mqttConfig.subscribeTopic);
			if(!mqtt.subscribe(mqttConfig.subscribeTopic)) {
				if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf_P(PSTR("  Unable to subscribe to to [%s]\n"), mqttConfig.subscribeTopic);
			}
		}
        return true;
	} else {
		if (debugger->isActive(RemoteDebug::ERROR)) {
			debugger->printf_P(PSTR("Failed to connect to MQTT: %d\n"), mqtt.lastError());
			#if defined(ESP8266)
				if(mqttSecureClient) {
					mqttSecureClient->getLastSSLError((char*) json, BufferSize);
					debugger->println((char*) json);
				}
			#endif
		}
        return false;
	}
}

void AmsMqttHandler::disconnect() {
    mqtt.disconnect();
    mqtt.loop();
    delay(10);
    yield();
}

lwmqtt_err_t AmsMqttHandler::lastError() {
    return mqtt.lastError();
}

bool AmsMqttHandler::connected() {
	return mqtt.connected();
}

bool AmsMqttHandler::loop() {
    bool ret = mqtt.loop();
    delay(10);
    yield();
	#if defined(ESP32)
		esp_task_wdt_reset();
	#elif defined(ESP8266)
		ESP.wdtFeed();
	#endif
    return ret;
}