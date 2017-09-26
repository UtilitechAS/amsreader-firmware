#include "HanReader.h"

HanReader::HanReader()
{
  
}

void HanReader::setup(HardwareSerial *hanPort)
{
  // Initialize H/W serial port for MBus communication
  hanPort->begin(2400, SERIAL_8E1);
  while (!hanPort) {}
  bytesRead = 0;
  han = hanPort;
}

void HanReader::setup(HardwareSerial *hanPort, Stream *debugPort)
{
  setup(hanPort);
  debug = debugPort;
  if (debug) debug->println("MBUS serial setup complete");
}

bool HanReader::read()
{
  if (han->available())
  {
    byte newByte = han->read();
    if (reader.Read(newByte))
    {
      bytesRead = reader.GetRawData(buffer, 0, 512);
      list = (List)kaifa.GetListID(buffer, 0, bytesRead);
	  return true;
    }
  }

  return false;
}

List HanReader::getList()
{
	return list;
}

time_t HanReader::getPackageTime()
{
	return kaifa.GetPackageTime(buffer, 0, bytesRead);
}

int HanReader::getInt(List1_ObisObjects objectId) { return getInt((int)objectId); }
int HanReader::getInt(List2_ObisObjects objectId) { return getInt((int)objectId); }
int HanReader::getInt(List3_ObisObjects objectId) { return getInt((int)objectId); }
int HanReader::getInt(int objectId)
{
	return kaifa.GetInt(objectId, buffer, 0, bytesRead);
}

String HanReader::getString(List1_ObisObjects objectId) { return getString((int)objectId); }
String HanReader::getString(List2_ObisObjects objectId) { return getString((int)objectId); }
String HanReader::getString(List3_ObisObjects objectId) { return getString((int)objectId); }
String HanReader::getString(int objectId)
{
	return kaifa.GetString(objectId, buffer, 0, bytesRead);
}
