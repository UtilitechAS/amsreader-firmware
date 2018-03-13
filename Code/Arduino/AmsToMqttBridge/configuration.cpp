// 
// 
// 

#include "configuration.h"

bool configuration::hasConfig() 
{
	bool has = false;
	EEPROM.begin(EEPROM_SIZE);
	has = EEPROM.read(EEPROM_CONFIG_ADDRESS) == EEPROM_CHECK_SUM;
	EEPROM.end();
	return has;
}

bool configuration::save()
{
	int address = EEPROM_CONFIG_ADDRESS;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(address, EEPROM_CHECK_SUM);
	address++;

	address += saveString(address, ssid);
	address += saveString(address, ssidPassword);
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

	bool vRet = EEPROM.commit();
	EEPROM.end();

	return vRet;
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

int configuration::readInt(int pAddress, int *pValue)
{
	int lower = EEPROM.read(pAddress);
	int higher = EEPROM.read(pAddress + 1);
	*pValue = lower + (higher << 8);
	return 2;
}
int configuration::saveInt(int pAddress, int pValue)
{
	byte lowByte = pValue & 0xFF;
	byte highByte = ((pValue >> 8) & 0xFF);

	EEPROM.write(pAddress, lowByte);
	EEPROM.write(pAddress + 1, highByte);

	return 2;
}

int configuration::readBool(int pAddress, bool *pValue)
{
	byte y = EEPROM.read(pAddress);
	*pValue = (bool)y;
	//Serial.printf("Read bool as %#x [%s]\r\n", y, (*pValue ? "true" : "false"));
	return 1;
}

int configuration::saveBool(int pAddress, bool pValue)
{
	byte y = (byte)pValue;
	//Serial.printf("Writing bool as %#x [%s]\r\n", y, (pValue ? "true" : "false"));
	EEPROM.write(pAddress, y);
	return 1;
}
void configuration::print(Stream& serial)
{

	/*
	char* ssid;
	char* ssidPassword;
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

	byte vLength = EEPROM.read(pAddress + address);
	address++;

	//Serial.print("Found length of string: ");
	//Serial.println(vLength);

	char* buffer = new char[vLength];
	for (int i = 0; i<vLength; i++)
	{
		buffer[i] = EEPROM.read(pAddress + address++);
	}
	*pString = buffer;

	//Serial.print("Read string from EEPROM: [");
	//Serial.print(*pString);
	//Serial.println("]");

	return address;
}
int configuration::saveString(int pAddress, char* pString)
{
	int address = 0;
	int vLength = strlen(pString) + 1;
	//Serial.print("Storing length of string: ");
	//Serial.println(vLength);
	EEPROM.put(pAddress + address, vLength);
	address++;

	//Serial.print("Storing string: ");
	//Serial.println(pString);
	for (int i = 0; i<vLength; i++)
	{
		EEPROM.put(pAddress + address, pString[i]);
		address++;
	}

	return address;
}
