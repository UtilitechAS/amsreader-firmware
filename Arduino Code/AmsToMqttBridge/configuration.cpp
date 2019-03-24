// 
// 
// 

#include "configuration.h"

bool configuration::hasConfig() 
{
	bool hasConfig = false;
	EEPROM.begin(EEPROM_SIZE);
	hasConfig = EEPROM.read(EEPROM_CONFIG_ADDRESS) == EEPROM_CHECK_SUM;
	EEPROM.end();
	return hasConfig;
}

bool configuration::save()
{
	int address = EEPROM_CONFIG_ADDRESS;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(address, EEPROM_CHECK_SUM);
	address++;

	address += saveString(address, ssid);
	address += saveString(address, ssidPassword);
	address += saveByte(address, meterType);
	address += saveString(address, mqtt);
	address += saveInt(address, mqttPort);
	address += saveString(address, mqttClientID);
	address += saveString(address, mqttPublishTopic);
	address += saveString(address, mqttSubscribeTopic);

	if (isSecure()) {
		address += saveBool(address, true);
		address += saveString(address, mqttUser);
		address += saveString(address, mqttPass);
	}
	else
		address += saveBool(address, false);

	bool success = EEPROM.commit();
	EEPROM.end();

	return success;
}


bool configuration::load()
{
	int address = EEPROM_CONFIG_ADDRESS;
	bool success = false;

	EEPROM.begin(EEPROM_SIZE);
	if (EEPROM.read(address) == EEPROM_CHECK_SUM)
	{
		address++;

		address += readString(address, &ssid);
		address += readString(address, &ssidPassword);
		address += readByte(address, &meterType);
		address += readString(address, &mqtt);
		address += readInt(address, &mqttPort);
		address += readString(address, &mqttClientID);
		address += readString(address, &mqttPublishTopic);
		address += readString(address, &mqttSubscribeTopic);

		bool secure = false;
		address += readBool(address, &secure);

		if (secure)
		{
			address += readString(address, &mqttUser);
			address += readString(address, &mqttPass);
		}
		else
		{
			mqttUser = 0;
			mqttPass = 0;
		}

		success = true;
	}
	else
	{
		ssid = (char*)String("").c_str();
		ssidPassword = (char*)String("").c_str();
		meterType = (byte)0;
		mqtt = (char*)String("").c_str();
		mqttClientID = (char*)String("").c_str();
		mqttPublishTopic = (char*)String("").c_str();
		mqttSubscribeTopic = (char*)String("").c_str();
		mqttUser = 0;
		mqttPass = 0;
		mqttPort = 1883;
	}
	EEPROM.end();
	return success;
}

bool configuration::isSecure()
{
	return (mqttUser != 0) && (String(mqttUser).length() > 0);
}

int configuration::readInt(int address, int *value)
{
	int lower = EEPROM.read(address);
	int higher = EEPROM.read(address + 1);
	*value = lower + (higher << 8);
	return 2;
}
int configuration::saveInt(int address, int value)
{
	byte lowByte = value & 0xFF;
	byte highByte = ((value >> 8) & 0xFF);

	EEPROM.write(address, lowByte);
	EEPROM.write(address + 1, highByte);

	return 2;
}

int configuration::readBool(int address, bool *value)
{
	byte y = EEPROM.read(address);
	*value = (bool)y;
	return 1;
}

int configuration::saveBool(int address, bool value)
{
	byte y = (byte)value;
	EEPROM.write(address, y);
	return 1;
}

int configuration::readByte(int address, byte *value)
{
	*value = EEPROM.read(address);
	return 1;
}

int configuration::saveByte(int address, byte value)
{
	EEPROM.write(address, value);
	return 1;
}
void configuration::print(Stream& serial)
{
	/*
	char* ssid;
	char* ssidPassword;
	byte meterType;
	char* mqtt;
	int mqttPort;
	char* mqttClientID;
	char* mqttPublishTopic;
	char* mqttSubscribeTopic;
	bool secure;
	char* mqttUser;
	char* mqttPass;
	*/

	serial.println("Configuration:");
	serial.println("-----------------------------------------------");
	serial.printf("ssid:                 %s\r\n", this->ssid);
	serial.printf("ssidPassword:         %s\r\n", this->ssidPassword);
	serial.printf("meterType:            %i\r\n", this->meterType);
	serial.printf("mqtt:                 %s\r\n", this->mqtt);
	serial.printf("mqttPort:             %i\r\n", this->mqttPort);
	serial.printf("mqttClientID:         %s\r\n", this->mqttClientID);
	serial.printf("mqttPublishTopic:     %s\r\n", this->mqttPublishTopic);
	serial.printf("mqttSubscribeTopic:   %s\r\n", this->mqttSubscribeTopic);

	if (this->isSecure())
	{
		serial.printf("SECURE MQTT CONNECTION:\r\n");
		serial.printf("mqttUser:             %s\r\n", this->mqttUser);
		serial.printf("mqttPass:             %s\r\n", this->mqttPass);
	}
	serial.println("-----------------------------------------------");
}

template <class T> int configuration::writeAnything(int ee, const T& value)
{
	const byte* p = (const byte*)(const void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		EEPROM.write(ee++, *p++);
	return i;
}

template <class T> int configuration::readAnything(int ee, T& value)
{
	byte* p = (byte*)(void*)&value;
	unsigned int i;
	for (i = 0; i < sizeof(value); i++)
		*p++ = EEPROM.read(ee++);
	return i;
}

int configuration::readString(int pAddress, char* pString[])
{
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
int configuration::saveString(int pAddress, char* pString)
{
	int address = 0;
	int length = strlen(pString) + 1;
	EEPROM.put(pAddress + address, length);
	address++;

	for (int i = 0; i < length; i++)
	{
		EEPROM.put(pAddress + address, pString[i]);
		address++;
	}

	return address;
}
