/**
 * ANSI C12.18 optical meter communicator.
 *
 * License: Fair Source
 */
#include "C1218MeterCommunicator.h"

#if defined(ESP32)

#include "Uptime.h"
#include "crc.h"

static const uint8_t C1218_ACK = 0x06;
static const uint8_t C1218_NACK = 0x15;
static const uint8_t C1218_START = 0xEE;
static const uint8_t C1218_OK = 0x00;

#if defined(AMS_REMOTE_DEBUG)
C1218MeterCommunicator::C1218MeterCommunicator(RemoteDebug* debugger, AmsConfiguration* config)
#else
C1218MeterCommunicator::C1218MeterCommunicator(Stream* debugger, AmsConfiguration* config)
#endif
    : debugger(debugger), config(config) {}

C1218MeterCommunicator::~C1218MeterCommunicator() {
    closeSession();
    serial->end();
}

void C1218MeterCommunicator::configure(MeterConfig& meterConfig, Timezone*) {
    this->meterConfig = meterConfig;
    config->getC1218Config(c1218Config);
    resetSerial();
    initialized = true;
    sessionOpen = updated = bigEndian = toggle = false;
    failures = 0;
    lastError = 0;
    nextPoll = 0;
}

void C1218MeterCommunicator::resetSerial() {
    serial->end();
    serial->setRxBufferSize(256);
    serial->begin(9600, SERIAL_8N1, meterConfig.rxPin, meterConfig.txPin, meterConfig.invert);
    discardInput();
}

bool C1218MeterCommunicator::loop() {
    uint64_t now = millis64();
    if(!initialized || now < nextPoll) return false;
    nextPoll = now + 2000;

    if(sessionOpen && now - sessionStarted >= 300000) closeSession();
    if((!sessionOpen && !openSession()) || !poll()) {
        closeSession();
        lastError = 97;
        if(++failures >= 3) {
            resetSerial();
            failures = 0;
        }
        return false;
    }

    failures = 0;
    lastError = 0;
    updated = true;
    return true;
}

AmsData* C1218MeterCommunicator::getData(AmsData&) {
    if(!updated) return NULL;
    updated = false;
    uint64_t now = millis64();
    AmsData* data = new AmsData();

    data->apply(OBIS_ACTIVE_IMPORT, max((int32_t) 0, table28[0]), now);
    data->apply(OBIS_ACTIVE_EXPORT, max((int32_t) 0, table28[1]), now);
    data->apply(OBIS_REACTIVE_IMPORT, max((int32_t) 0, table28[2]), now);
    data->apply(OBIS_REACTIVE_EXPORT, max((int32_t) 0, table28[3]), now);
    data->apply(OBIS_CURRENT_L1, table28[4] / 1000.0, now);
    data->apply(OBIS_CURRENT_L2, table28[5] / 1000.0, now);
    data->apply(OBIS_CURRENT_L3, table28[6] / 1000.0, now);
    data->apply(OBIS_VOLTAGE_L1, table28[7] / 1000.0, now);
    data->apply(OBIS_VOLTAGE_L2, table28[8] / 1000.0, now);
    data->apply(OBIS_VOLTAGE_L3, table28[9] / 1000.0, now);
    if(table28Values >= 15) {
        data->apply(OBIS_POWER_FACTOR, table28[10] / 1000.0, now);
        data->apply(OBIS_POWER_FACTOR_L1, table28[10] / 1000.0, now);
        data->apply(OBIS_POWER_FACTOR_L2, table28[13] / 1000.0, now);
        data->apply(OBIS_POWER_FACTOR_L3, table28[14] / 1000.0, now);
    }
    data->apply(OBIS_ACTIVE_IMPORT_COUNT, table23[0] / 1000.0, now);
    data->apply(OBIS_ACTIVE_EXPORT_COUNT, table23[1] / 1000.0, now);
    return data;
}

int C1218MeterCommunicator::getLastError() { return lastError; }
bool C1218MeterCommunicator::isConfigChanged() { return false; }
void C1218MeterCommunicator::ackConfigChanged() {}
void C1218MeterCommunicator::getCurrentConfig(MeterConfig& config) { config = meterConfig; }
HardwareSerial* C1218MeterCommunicator::getHwSerial() { return serial; }

bool C1218MeterCommunicator::openSession() {
    discardInput();
    toggle = false;
    uint8_t response[MESSAGE_CAPACITY];
    size_t length;

    const uint8_t ident[] = {0x20};
    length = sizeof(response);
    if(!request(ident, sizeof(ident), response, length)) { fail(F("C12.18 IDENT failed")); return false; }

    const uint8_t negotiate[] = {0x61, 0x00, 0x40, 0x02, 0x06};
    length = sizeof(response);
    if(!request(negotiate, sizeof(negotiate), response, length)) { fail(F("C12.18 NEGOTIATE failed")); return false; }

    uint8_t logon[13];
    memset(logon, ' ', sizeof(logon));
    logon[0] = 0x50;
    logon[1] = c1218Config.userId >> 8;
    logon[2] = c1218Config.userId;
    memcpy(logon + 3, c1218Config.username, strnlen(c1218Config.username, 10));
    length = sizeof(response);
    if(!request(logon, sizeof(logon), response, length)) { fail(F("C12.18 LOGON failed")); return false; }

    uint8_t security[21] = {0x51};
    memcpy(security + 1, c1218Config.password, strnlen(c1218Config.password, 20));
    length = sizeof(response);
    if(!request(security, sizeof(security), response, length)) { fail(F("C12.18 SECURITY failed")); return false; }

    length = sizeof(response);
    if(!readTable(0, response, length) || length < 3) { fail(F("C12.18 table 0 read failed")); return false; }
    uint16_t tableLength = (response[0] << 8) | response[1];
    if(tableLength + 2 > length) { fail(F("C12.18 table 0 is incomplete")); return false; }
    bigEndian = (response[2] & 0x01) != 0;

    sessionOpen = true;
    sessionStarted = millis64();
    return true;
}

void C1218MeterCommunicator::closeSession() {
    if(!sessionOpen) return;
    uint8_t response[8];
    size_t length = sizeof(response);
    const uint8_t logoff[] = {0x52};
    request(logoff, sizeof(logoff), response, length);
    length = sizeof(response);
    const uint8_t terminate[] = {0x21};
    request(terminate, sizeof(terminate), response, length);
    sessionOpen = false;
}

bool C1218MeterCommunicator::poll() {
    uint8_t response[MESSAGE_CAPACITY];
    size_t length = sizeof(response);
    const size_t values = c1218Config.extendedTable28 ? 26 : 10;
    if(!readPartialTable(28, values * 4, response, length) || !parseTable(response, length, table28, values)) {
        fail(F("C12.18 table 28 read failed"));
        return false;
    }
    table28Values = values;

    length = sizeof(response);
    if(!readPartialTable(23, 8, response, length) || !parseTable(response, length, table23, 2)) {
        fail(F("C12.18 table 23 read failed"));
        return false;
    }
    return true;
}

bool C1218MeterCommunicator::readTable(uint16_t table, uint8_t* response, size_t& responseLength) {
    const uint8_t payload[] = {0x30, (uint8_t) (table >> 8), (uint8_t) table};
    return request(payload, sizeof(payload), response, responseLength);
}

bool C1218MeterCommunicator::readPartialTable(uint16_t table, uint16_t bytes, uint8_t* response, size_t& responseLength) {
    const uint8_t payload[] = {0x3F, (uint8_t) (table >> 8), (uint8_t) table, 0, 0, 0,
        (uint8_t) (bytes >> 8), (uint8_t) bytes};
    return request(payload, sizeof(payload), response, responseLength);
}

bool C1218MeterCommunicator::request(const uint8_t* payload, size_t length, uint8_t* response, size_t& responseLength) {
    if(!sendFrame(payload, length)) return false;
    if(!receiveMessage(response, responseLength) || responseLength < 1 || response[0] != C1218_OK) return false;
    memmove(response, response + 1, --responseLength);
    return true;
}

bool C1218MeterCommunicator::sendFrame(const uint8_t* payload, size_t length) {
    if(length + 8 > FRAME_CAPACITY) return false;
    uint8_t frame[FRAME_CAPACITY];
    frame[0] = C1218_START;
    frame[1] = 0;
    frame[2] = toggle ? 0x20 : 0;
    frame[3] = 0;
    frame[4] = length >> 8;
    frame[5] = length;
    memcpy(frame + 6, payload, length);
    uint16_t crc = crc16_x25(frame, length + 6);
    frame[length + 6] = crc >> 8;
    frame[length + 7] = crc;
    toggle = !toggle;

    for(uint8_t attempt = 0; attempt < 3; attempt++) {
        discardInput();
        serial->write(frame, length + 8);
        serial->flush();
        uint8_t reply = 0;
        if(waitByte(reply) && reply == C1218_ACK) return true;
        if(reply != C1218_NACK) delay(20);
    }
    return false;
}

bool C1218MeterCommunicator::receiveMessage(uint8_t* response, size_t& responseLength) {
    size_t capacity = responseLength;
    responseLength = 0;
    for(uint8_t packet = 0; packet < 16; packet++) {
        uint8_t payload[FRAME_CAPACITY], control, sequence;
        size_t length = sizeof(payload);
        if(!receiveFrame(payload, length, control, sequence) || responseLength + length > capacity) return false;
        memcpy(response + responseLength, payload, length);
        responseLength += length;
        if(!(control & 0x80) || sequence == 0) return true;
    }
    return false;
}

bool C1218MeterCommunicator::receiveFrame(uint8_t* payload, size_t& payloadLength, uint8_t& control, uint8_t& sequence) {
    uint8_t value;
    do {
        if(!waitByte(value)) return false;
    } while(value != C1218_START);

    uint8_t frame[FRAME_CAPACITY];
    frame[0] = value;
    if(!readExact(frame + 1, 5)) return false;
    control = frame[2];
    sequence = frame[3];
    uint16_t length = (frame[4] << 8) | frame[5];
    if(length > payloadLength || length + 8 > sizeof(frame)) { serial->write(C1218_NACK); return false; }
    if(!readExact(frame + 6, length + 2)) { serial->write(C1218_NACK); return false; }
    uint16_t crc = crc16_x25(frame, length + 6);
    if(frame[length + 6] != (uint8_t) (crc >> 8) || frame[length + 7] != (uint8_t) crc) {
        serial->write(C1218_NACK);
        return false;
    }
    memcpy(payload, frame + 6, length);
    payloadLength = length;
    serial->write(C1218_ACK);
    serial->flush();
    return true;
}

bool C1218MeterCommunicator::waitByte(uint8_t& value, uint32_t timeout) {
    uint32_t start = millis();
    while(millis() - start < timeout) {
        if(serial->available()) { value = serial->read(); return true; }
        yield();
        delay(1);
    }
    return false;
}

bool C1218MeterCommunicator::readExact(uint8_t* target, size_t length, uint32_t timeout) {
    size_t offset = 0;
    uint32_t start = millis();
    while(offset < length && millis() - start < timeout) {
        while(serial->available() && offset < length) target[offset++] = serial->read();
        if(offset < length) { yield(); delay(1); }
    }
    return offset == length;
}

void C1218MeterCommunicator::discardInput() {
    while(serial->available()) serial->read();
}

void C1218MeterCommunicator::fail(const __FlashStringHelper* message) {
    debugger->println(message);
}

int32_t C1218MeterCommunicator::readInt32(const uint8_t* value) const {
    uint32_t decoded = bigEndian
        ? ((uint32_t) value[0] << 24) | ((uint32_t) value[1] << 16) | ((uint32_t) value[2] << 8) | value[3]
        : value[0] | ((uint32_t) value[1] << 8) | ((uint32_t) value[2] << 16) | ((uint32_t) value[3] << 24);
    return (int32_t) decoded;
}

bool C1218MeterCommunicator::parseTable(const uint8_t* response, size_t responseLength, int32_t* values, size_t count) {
    if(responseLength < 2) return false;
    uint16_t length = (response[0] << 8) | response[1];
    if(length < count * 4 || responseLength < count * 4 + 2) return false;
    for(size_t i = 0; i < count; i++) values[i] = readInt32(response + 2 + i * 4);
    return true;
}

#endif
