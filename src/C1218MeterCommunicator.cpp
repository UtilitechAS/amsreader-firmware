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
static const uint8_t C1218_SNS = 0x02;

static uint32_t c1218Baud(uint8_t code) {
    static const uint32_t rates[] = {0, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 128000, 256000};
    return code < sizeof(rates) / sizeof(rates[0]) ? rates[code] : 0;
}

#if defined(AMS_REMOTE_DEBUG)
C1218MeterCommunicator::C1218MeterCommunicator(RemoteDebug* debugger, AmsConfiguration* config)
#else
C1218MeterCommunicator::C1218MeterCommunicator(Stream* debugger, AmsConfiguration* config)
#endif
    : debugger(debugger), config(config) {}

C1218MeterCommunicator::~C1218MeterCommunicator() {
    serial->end();
}

void C1218MeterCommunicator::configure(MeterConfig& meterConfig, Timezone*) {
    this->meterConfig = meterConfig;
    config->getC1218Config(c1218Config);
    serialBaud = 9600;
    resetSerial();
    initialized = true;
    sessionOpen = updated = bigEndian = toggle = false;
    sessionNegotiated = false;
    packetSize = DEFAULT_PACKET_SIZE;
    maxPackets = DEFAULT_MAX_PACKETS;
    nextExtendedTable = 0;
    readExtendedTable = false;
    failures = attempts = packets = 0;
    lastError = 0;
    nextPoll = 0;
    stage = WAIT_POLL;
    ioState = IO_IDLE;
    responseLength = rxLength = rxExpected = 0;
    intercharacterDeadline = 0;
    rejectedPackets = 0;
    hasLastRxPacket = false;
    expectedRxSequence = 0;
}

void C1218MeterCommunicator::resetSerial() {
    serial->end();
    serial->setRxBufferSize(256);
    serial->begin(serialBaud, SERIAL_8N1, meterConfig.rxPin, meterConfig.txPin, meterConfig.invert);
    discardInput();
    debugger->print(F("C12.18 UART configured: RX="));
    debugger->print(meterConfig.rxPin);
    debugger->print(F(", TX="));
    debugger->println(meterConfig.txPin);
}

bool C1218MeterCommunicator::loop() {
    uint64_t now = millis64();
    if(!initialized) return false;

    if(ioState != IO_IDLE) {
        RequestStatus status = serviceRequest(now);
        if(status == PENDING) return false;
        if(status == FAILED) {
            abortCycle(now);
            return false;
        }
        return finishStage(now);
    }

    if(stage == WAIT_POLL) {
        if(now < nextPoll) return false;
        stage = sessionOpen && now - sessionStarted >= 300000 ? LOGOFF : sessionOpen ? TABLE28 : IDENT;
    }
    startStage();
    return false;
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
    if(table28Values >= 10) {
        data->apply(OBIS_CURRENT_L1, table28[4] / 1000.0, now);
        data->apply(OBIS_CURRENT_L2, table28[5] / 1000.0, now);
        data->apply(OBIS_CURRENT_L3, table28[6] / 1000.0, now);
        data->apply(OBIS_VOLTAGE_L1, table28[7] / 1000.0, now);
        data->apply(OBIS_VOLTAGE_L2, table28[8] / 1000.0, now);
        data->apply(OBIS_VOLTAGE_L3, table28[9] / 1000.0, now);
    }
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

void C1218MeterCommunicator::startStage() {
    switch(stage) {
        case IDENT: {
            toggle = false;
            const uint8_t payload[] = {0x20};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case NEGOTIATE: {
            const uint8_t payload[] = {0x61, 0x00, 0x40, 0x02, 0x06};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case LOGON: {
            uint8_t payload[13];
            memset(payload, ' ', sizeof(payload));
            payload[0] = 0x50;
            payload[1] = c1218Config.userId >> 8;
            payload[2] = c1218Config.userId;
            memcpy(payload + 3, c1218Config.username, strnlen(c1218Config.username, 10));
            beginRequest(payload, sizeof(payload));
            break;
        }
        case SECURITY: {
            if(!c1218Config.password[0]) {
                stage = TABLE0;
                break;
            }
            uint8_t payload[21] = {0x51};
            memcpy(payload + 1, c1218Config.password, strnlen(c1218Config.password, 20));
            beginRequest(payload, sizeof(payload));
            break;
        }
        case TABLE0: {
            const uint8_t payload[] = {0x30, 0, 0};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case TABLE28: {
            bool canReadExtended = c1218Config.extendedTable28 && maxPackets > 1;
            bool extended = canReadExtended && millis64() >= nextExtendedTable;
            readExtendedTable = extended;
            uint16_t values = extended ? 26 : (canReadExtended ? 4 : 10);
            uint16_t bytes = values * 4;
            const uint8_t payload[] = {0x3F, 0, 28, 0, 0, 0, (uint8_t) (bytes >> 8), (uint8_t) bytes};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case TABLE23: {
            const uint8_t payload[] = {0x3F, 0, 23, 0, 0, 0, 0, 8};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case LOGOFF: {
            const uint8_t payload[] = {0x52};
            beginRequest(payload, sizeof(payload));
            break;
        }
        case TERMINATE: {
            const uint8_t payload[] = {0x21};
            beginRequest(payload, sizeof(payload));
            break;
        }
        default:
            break;
    }
}

bool C1218MeterCommunicator::finishStage(uint64_t now) {
    switch(stage) {
        case IDENT: stage = NEGOTIATE; break;
        case NEGOTIATE:
            if(responseLength == 1 && response[0] == C1218_SNS) {
                sessionNegotiated = false;
                packetSize = DEFAULT_PACKET_SIZE;
                maxPackets = DEFAULT_MAX_PACKETS;
                responseLength = 0;
                stage = LOGON;
                break;
            }
            if(responseLength < 3) {
                requestFailed(F("invalid negotiate response"));
                abortCycle(now);
                return false;
            }
            packetSize = ((uint16_t) response[0] << 8) | response[1];
            maxPackets = response[2];
            if(packetSize < 32 || packetSize > FRAME_CAPACITY || maxPackets == 0) {
                requestFailed(F("invalid negotiate values"));
                abortCycle(now);
                return false;
            }
            if(responseLength >= 4) {
                uint32_t baud = c1218Baud(response[3]);
                if(baud && baud != serialBaud) {
                    serialBaud = baud;
                    serial->updateBaudRate(baud);
                    debugger->print(F("C12.18 negotiated baud: "));
                    debugger->println(baud);
                }
            }
            sessionNegotiated = true;
            stage = LOGON;
            break;
        case LOGON:
            sessionOpen = true;
            sessionStarted = now;
            stage = SECURITY;
            break;
        case SECURITY:
            if(responseLength == 1 && response[0] == C1218_SNS) responseLength = 0;
            stage = TABLE0;
            break;
        case TABLE0: {
            if(responseLength < 3 || ((response[0] << 8) | response[1]) + 2 > responseLength) {
                requestFailed(F("incomplete response"));
                abortCycle(now);
                return false;
            }
            bigEndian = (response[2] & 0x01) != 0;
            stage = TABLE28;
            break;
        }
        case TABLE28: {
            size_t values = c1218Config.extendedTable28 && maxPackets > 1 ? (readExtendedTable ? 26 : 4) : 10;
            if(!parseTable(response, responseLength, table28, values)) {
                requestFailed(F("invalid table response"));
                abortCycle(now);
                return false;
            }
            if(readExtendedTable || table28Values < values) table28Values = values;
            if(readExtendedTable) nextExtendedTable = now + EXTENDED_TABLE_INTERVAL;
            stage = TABLE23;
            break;
        }
        case TABLE23:
            if(!parseTable(response, responseLength, table23, 2)) {
                requestFailed(F("invalid table response"));
                abortCycle(now);
                return false;
            }
            failures = 0;
            lastError = 0;
            updated = true;
            nextPoll = now + 2000;
            stage = WAIT_POLL;
            return true;
        case LOGOFF:
            sessionOpen = false;
            stage = TERMINATE;
            break;
        case TERMINATE:
            sessionOpen = false;
            sessionNegotiated = false;
            packetSize = DEFAULT_PACKET_SIZE;
            maxPackets = DEFAULT_MAX_PACKETS;
            serialBaud = 9600;
            stage = WAIT_POLL;
            nextPoll = now + RETRY_DELAY;
            break;
        default:
            break;
    }
    return false;
}

void C1218MeterCommunicator::beginRequest(const uint8_t* payload, size_t length) {
    txFrame[0] = C1218_START;
    txFrame[1] = 0;
    txFrame[2] = toggle ? 0x20 : 0;
    txFrame[3] = 0;
    txFrame[4] = length >> 8;
    txFrame[5] = length;
    memcpy(txFrame + 6, payload, length);
    uint16_t crc = crc16_x25(txFrame, length + 6);
    txFrame[length + 6] = crc >> 8;
    txFrame[length + 7] = crc;
    txLength = length + 8;
    toggle = !toggle;
    attempts = packets = 0;
    responseLength = 0;
    rejectedPackets = 0;
    hasLastRxPacket = false;
    expectedRxSequence = 0;
    transmit(millis64());
}

void C1218MeterCommunicator::transmit(uint64_t now) {
    discardInput();
    serial->write(txFrame, txLength);
    attempts++;
    ioState = WAIT_ACK;
    deadline = now + RESPONSE_TIMEOUT;
}

void C1218MeterCommunicator::sendControl(uint8_t control) {
    delayMicroseconds(175);
    serial->write(control);
}

C1218MeterCommunicator::RequestStatus C1218MeterCommunicator::serviceRequest(uint64_t now) {
    if(ioState == WAIT_ACK) {
        if(serial->available()) {
            uint8_t reply = serial->read();
            if(reply == C1218_ACK) {
                ioState = WAIT_START;
                deadline = now + CHANNEL_TIMEOUT;
            } else if(attempts <= MAX_RETRIES) {
                transmit(now);
            } else {
                return requestFailed(reply == C1218_NACK ? F("NACK") : F("unexpected ACK reply"));
            }
        } else if(now >= deadline) {
            if(attempts <= MAX_RETRIES) transmit(now);
            else return requestFailed(F("ACK timeout"));
        }
        return PENDING;
    }

    if(ioState == WAIT_START) {
        while(serial->available()) {
            if(serial->read() == C1218_START) {
                rxFrame[0] = C1218_START;
                rxLength = 1;
                rxExpected = 6;
                ioState = READ_FRAME;
                intercharacterDeadline = now + INTERCHAR_TIMEOUT;
                break;
            }
        }
        if(ioState == WAIT_START) return now >= deadline ? requestFailed(F("frame start timeout")) : PENDING;
    }

    while(serial->available() && rxLength < rxExpected) {
        rxFrame[rxLength++] = serial->read();
        intercharacterDeadline = now + INTERCHAR_TIMEOUT;
    }
    if(rxLength < rxExpected) return now >= intercharacterDeadline ? rejectPacket(F("inter-character timeout"), now) : PENDING;

    uint16_t length = (rxFrame[4] << 8) | rxFrame[5];
    if(rxExpected == 6) {
        if(length + 8 > FRAME_CAPACITY || length + 8 > packetSize || responseLength + length > MESSAGE_CAPACITY) {
            return rejectPacket(F("frame too large"), now);
        }
        rxExpected = length + 8;
        intercharacterDeadline = now + INTERCHAR_TIMEOUT;
        while(serial->available() && rxLength < rxExpected) {
            rxFrame[rxLength++] = serial->read();
            intercharacterDeadline = now + INTERCHAR_TIMEOUT;
        }
        if(rxLength < rxExpected) return now >= intercharacterDeadline ? rejectPacket(F("inter-character timeout"), now) : PENDING;
    }

    uint16_t crc = crc16_x25(rxFrame, length + 6);
    if(rxFrame[length + 6] != (uint8_t) (crc >> 8) || rxFrame[length + 7] != (uint8_t) crc) {
        return rejectPacket(F("CRC mismatch"), now);
    }

    uint8_t identity = rxFrame[1];
    uint8_t control = rxFrame[2];
    uint8_t sequence = rxFrame[3];
    uint16_t wireCrc = ((uint16_t) rxFrame[length + 6] << 8) | rxFrame[length + 7];
    if(identity == 0xFF || (control & 0x1F)) return rejectPacket(F("invalid packet control"), now);
    if(hasLastRxPacket && identity == lastRxIdentity && control == lastRxControl && sequence == lastRxSequence && wireCrc == lastRxCrc) {
        sendControl(C1218_ACK);
        ioState = WAIT_START;
        deadline = now + CHANNEL_TIMEOUT;
        return PENDING;
    }
    if(packets == 0) {
        if(control & 0x80) {
            if(!(control & 0x40) || sequence == 0) return rejectPacket(F("invalid first sequence"), now);
            expectedRxSequence = sequence - 1;
        } else if((control & 0x40) || sequence != 0) {
            return rejectPacket(F("invalid packet sequence"), now);
        }
    } else {
        if(!(control & 0x80) || (control & 0x40) || sequence != expectedRxSequence || identity != lastRxIdentity || ((control & 0x20) == (lastRxControl & 0x20))) {
            return rejectPacket(F("invalid packet sequence"), now);
        }
        if(expectedRxSequence > 0) expectedRxSequence--;
    }
    lastRxIdentity = identity;
    lastRxControl = control;
    lastRxSequence = sequence;
    lastRxCrc = wireCrc;
    hasLastRxPacket = true;
    rejectedPackets = 0;

    if(packets >= maxPackets) return rejectPacket(F("packet limit exceeded"), now);
    memcpy(response + responseLength, rxFrame + 6, length);
    responseLength += length;
    sendControl(C1218_ACK);
    packets++;
    if((control & 0x80) && sequence != 0) {
        ioState = WAIT_START;
        deadline = now + CHANNEL_TIMEOUT;
        return PENDING;
    }

    ioState = IO_IDLE;
    if(responseLength < 1) return requestFailed(F("empty response"));
    if(response[0] != C1218_OK) {
        if((stage == NEGOTIATE || stage == SECURITY) && response[0] == C1218_SNS) return COMPLETE;
        return requestFailed(F("meter status error"));
    }
    memmove(response, response + 1, --responseLength);
    return COMPLETE;
}

C1218MeterCommunicator::RequestStatus C1218MeterCommunicator::requestFailed(const __FlashStringHelper* reason) {
    logFailure(reason);
    return FAILED;
}

C1218MeterCommunicator::RequestStatus C1218MeterCommunicator::rejectPacket(const __FlashStringHelper* reason, uint64_t now) {
    logFailure(reason);
    sendControl(C1218_NACK);
    if(++rejectedPackets > MAX_RETRIES) return FAILED;
    rxLength = rxExpected = 0;
    ioState = WAIT_START;
    deadline = now + CHANNEL_TIMEOUT;
    return PENDING;
}

void C1218MeterCommunicator::logFailure(const __FlashStringHelper* reason) {
    debugger->print(F("C12.18 "));
    debugger->print(stageName());
    debugger->print(F(" failed on RX="));
    debugger->print(meterConfig.rxPin);
    debugger->print(F(", TX="));
    debugger->print(meterConfig.txPin);
    debugger->print(F(": "));
    debugger->println(reason);
}

const char* C1218MeterCommunicator::stageName() const {
    switch(stage) {
        case IDENT: return "IDENT";
        case NEGOTIATE: return "NEGOTIATE";
        case LOGON: return "LOGON";
        case SECURITY: return "SECURITY";
        case TABLE0: return "TABLE0";
        case TABLE28: return "TABLE28";
        case TABLE23: return "TABLE23";
        case LOGOFF: return "LOGOFF";
        case TERMINATE: return "TERMINATE";
        default: return "WAIT_POLL";
    }
}

void C1218MeterCommunicator::abortCycle(uint64_t now) {
    ioState = IO_IDLE;
    bool sessionMayBeOpen = sessionOpen || (stage >= LOGON && stage < LOGOFF);
    if(sessionMayBeOpen && stage != LOGOFF && stage != TERMINATE) {
        stage = sessionOpen ? LOGOFF : TERMINATE;
    } else {
        sessionOpen = false;
        toggle = false;
        stage = WAIT_POLL;
        nextPoll = now + RETRY_DELAY;
    }
    lastError = 97;
    if(++failures >= 3) {
        fail(F("C12.18 resetting UART after 3 failed cycles"));
        serialBaud = 9600;
        resetSerial();
        failures = 0;
    }
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
