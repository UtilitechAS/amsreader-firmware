#include "PassthroughMqttHandler.h"

bool PassthroughMqttHandler::publish(AmsData* data, AmsData* previousState, EnergyAccounting* ea, EntsoeApi* eapi) {
    return false;
}

bool PassthroughMqttHandler::publishTemperatures(AmsConfiguration*, HwTools*) {
    return false;
}

bool PassthroughMqttHandler::publishPrices(EntsoeApi*) {
    return false;
}

bool PassthroughMqttHandler::publishSystem(HwTools* hw, EntsoeApi* eapi, EnergyAccounting* ea) {
    return false;
}

bool PassthroughMqttHandler::publishRaw(String data) {
    bool ret = mqtt.publish(mqttConfig.publishTopic, data);
    loop();
    delay(10);
    return ret;
}

uint8_t PassthroughMqttHandler::getFormat() {
    return 255;
}

void PassthroughMqttHandler::onMessage(String &topic, String &payload) {
}
