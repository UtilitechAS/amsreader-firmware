// config.h

#ifndef _CONFIGURATION_h
#define _CONFIGURATION_h

#include <EEPROM.h>


#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class configuration {
public:
	char* ssid;
	char* ssidPassword;
	char* mqtt;
	int mqttPort;
	char* mqttClientID;
	char* mqttPublishTopic;
	char* mqttSubscribeTopic;
	char* mqttUser;
	char* mqttPass;
	byte meterType;

	bool hasConfig();
	bool isSecure();
	bool save();
	bool load();

	void print(Stream& serial);
protected:

private:
	const int EEPROM_SIZE = 512;
	const byte EEPROM_CHECK_SUM = 71; // Used to check if config is stored. Change if structure changes
	const int EEPROM_CONFIG_ADDRESS = 0;

	int saveString(int pAddress, char* pString);
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

