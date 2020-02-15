#include "HwTools.h"


double HwTools::getVcc() {
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
	return (((double) ESP.getVcc()) / 1024) * 1.1; // This board has a voltage divider on VCC, add 10%
#elif defined(ESP8266)
	return ((double) ESP.getVcc()) / 1024;
#endif
	return -1;
}

double HwTools::getTemperature() {

#if defined TEMP_SENSOR_PIN
    if(!tempSensorInit) {
        tempSensor->begin();
        delay(25);
        tempSensor->requestTemperatures();
        hasTempSensor = tempSensor->getTempCByIndex(0) != DEVICE_DISCONNECTED_C;
        tempSensorInit = true;
    }

    if(hasTempSensor) {
        tempSensor->requestTemperatures();
        return tempSensor->getTempCByIndex(0);
    } else {
        return DEVICE_DISCONNECTED_C;
    }
#endif
    return DEVICE_DISCONNECTED_C;
}

