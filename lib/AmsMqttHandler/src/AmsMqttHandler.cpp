#include "AmsMqttHandler.h"
#include "FirmwareVersion.h"
#include "AmsStorage.h"
#include "LittleFS.h"

bool AmsMqttHandler::connect() {
	time_t epoch = time(nullptr);

	if(mqttConfig.ssl) {
		if(epoch < FirmwareVersion::BuildEpoch) {
			if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("NTP not ready for MQTT SSL"));
			return false;
		}
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("MQTT SSL is configured (%dkb free heap)"), ESP.getFreeHeap());
		if(mqttSecureClient == NULL) {
			mqttSecureClient = new WiFiClientSecure();
			#if defined(ESP8266)
				mqttSecureClient->setBufferSizes(512, 512);
				debugD_P(PSTR("ESP8266 firmware does not have enough memory..."));
				return;
			#endif
		
			if(LittleFS.begin()) {
				File file;

				if(LittleFS.exists(FILE_MQTT_CA)) {
					if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT CA file (%dkb free heap)"), ESP.getFreeHeap());
					file = LittleFS.open(FILE_MQTT_CA, (char*) "r");
					#if defined(ESP8266)
						BearSSL::X509List *serverTrustedCA = new BearSSL::X509List(file);
						mqttSecureClient->setTrustAnchors(serverTrustedCA);
					#elif defined(ESP32)
						if(mqttSecureClient->loadCACert(file, file.size())) {
							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("CA accepted"));
						} else {
							if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("CA was rejected"));
							delete mqttSecureClient;
							mqttSecureClient = NULL;
							return false;
						}
					#endif
					file.close();

					if(LittleFS.exists(FILE_MQTT_CERT) && LittleFS.exists(FILE_MQTT_KEY)) {
						#if defined(ESP8266)
							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT certificate file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							BearSSL::X509List *serverCertList = new BearSSL::X509List(file);
							file.close();

							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT key file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							BearSSL::PrivateKey *serverPrivKey = new BearSSL::PrivateKey(file);
							file.close();

							debugD_P(PSTR("Setting client certificates (%dkb free heap)"), ESP.getFreeHeap());
							mqttSecureClient->setClientRSACert(serverCertList, serverPrivKey);
						#elif defined(ESP32)
							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT certificate file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_CERT, (char*) "r");
							mqttSecureClient->loadCertificate(file, file.size());
							file.close();

							if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Found MQTT key file (%dkb free heap)"), ESP.getFreeHeap());
							file = LittleFS.open(FILE_MQTT_KEY, (char*) "r");
							mqttSecureClient->loadPrivateKey(file, file.size());
							file.close();
						#endif
					}
				} else {
					if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("No CA, disabling certificate validation"));
					mqttSecureClient->setInsecure();
				}
				mqttClient = mqttSecureClient;

				LittleFS.end();
				if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("MQTT SSL setup complete (%dkb free heap)"), ESP.getFreeHeap());
			}
		}
	}
	
	if(mqttClient == NULL) {
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("No SSL, using client without SSL support"));
		mqttClient = new WiFiClient();
	}

    if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Connecting to MQTT %s:%d"), mqttConfig.host, mqttConfig.port);
	
	mqtt.begin(mqttConfig.host, mqttConfig.port, *mqttClient);

	#if defined(ESP8266)
		if(mqttSecureClient) {
			time_t epoch = time(nullptr);
			debugD_P(PSTR("Setting NTP time %lu for secure MQTT connection"), epoch);
			mqttSecureClient->setX509Time(epoch);
		}
	#endif

	// Connect to a unsecure or secure MQTT server
	if ((strlen(mqttConfig.username) == 0 && mqtt.connect(mqttConfig.clientId)) ||
		(strlen(mqttConfig.username) > 0 && mqtt.connect(mqttConfig.clientId, mqttConfig.username, mqttConfig.password))) {
		if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("Successfully connected to MQTT!"));
        return true;
	} else {
		if (debugger->isActive(RemoteDebug::ERROR)) {
			debugger->printf_P(PSTR("Failed to connect to MQTT: %d"), mqtt.lastError());
			#if defined(ESP8266)
				if(mqttSecureClient) {
					mqttSecureClient->getLastSSLError((char*) commonBuffer, BUF_SIZE_COMMON);
					Debug.println((char*) commonBuffer);
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

	if(mqttClient != NULL) {
		mqttClient->stop();
		delete mqttClient;
		mqttClient = NULL;
		if(mqttSecureClient != NULL) {
			mqttSecureClient = NULL;
		}
	}
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