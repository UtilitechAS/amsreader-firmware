/**
 * ANSI C12.18 optical meter communicator.
 *
 * License: Fair Source
 */
#pragma once

#if defined(ESP32)

#include "MeterCommunicator.h"
#include "Timezone.h"

class C1218MeterCommunicator : public MeterCommunicator {
public:
#if defined(AMS_REMOTE_DEBUG)
    C1218MeterCommunicator(RemoteDebug* debugger, AmsConfiguration* config);
#else
    C1218MeterCommunicator(Stream* debugger, AmsConfiguration* config);
#endif
    ~C1218MeterCommunicator();

    void configure(MeterConfig& config, Timezone* tz);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void ackConfigChanged();
    void getCurrentConfig(MeterConfig& config);
    HardwareSerial* getHwSerial();

private:
    static const size_t FRAME_CAPACITY = 160;
    static const size_t MESSAGE_CAPACITY = 192;
    static const uint32_t RESPONSE_TIMEOUT = 2000;
    static const uint32_t CHANNEL_TIMEOUT = 6000;
    static const uint32_t INTERCHAR_TIMEOUT = 500;
    static const uint32_t RETRY_DELAY = 5000;
    static const uint32_t EXTENDED_TABLE_INTERVAL = 60000;
    static const uint8_t MAX_RETRIES = 3;
    static const uint8_t DEFAULT_MAX_PACKETS = 1;
    static const uint16_t DEFAULT_PACKET_SIZE = 64;

    enum Stage : uint8_t { WAIT_POLL, IDENT, NEGOTIATE, LOGON, SECURITY, TABLE0, TABLE28, TABLE23, LOGOFF, TERMINATE };
    enum IoState : uint8_t { IO_IDLE, WAIT_ACK, WAIT_START, READ_FRAME };
    enum RequestStatus : uint8_t { PENDING, COMPLETE, FAILED };

#if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger;
#else
    Stream* debugger;
#endif
    AmsConfiguration* config;
    HardwareSerial* serial = &Serial1;
    MeterConfig meterConfig = {};
    C1218Config c1218Config = {};

    bool initialized = false;
    bool sessionOpen = false;
    bool updated = false;
    bool bigEndian = false;
    bool toggle = false;
    bool sessionNegotiated = false;
    uint8_t failures = 0;
    int lastError = 0;
    uint64_t nextPoll = 0;
    uint64_t sessionStarted = 0;
    uint16_t packetSize = DEFAULT_PACKET_SIZE;
    uint8_t maxPackets = DEFAULT_MAX_PACKETS;
    uint32_t serialBaud = 9600;
    uint64_t nextExtendedTable = 0;
    bool readExtendedTable = false;
    int32_t table23[2] = {};
    int32_t table28[26] = {};
    size_t table28Values = 0;
    Stage stage = WAIT_POLL;
    IoState ioState = IO_IDLE;
    uint8_t attempts = 0;
    uint8_t packets = 0;
    uint8_t txFrame[FRAME_CAPACITY] = {};
    size_t txLength = 0;
    uint8_t rxFrame[FRAME_CAPACITY] = {};
    size_t rxLength = 0;
    size_t rxExpected = 0;
    uint8_t response[MESSAGE_CAPACITY] = {};
    size_t responseLength = 0;
    uint64_t deadline = 0;
    uint64_t intercharacterDeadline = 0;
    uint8_t rejectedPackets = 0;
    bool hasLastRxPacket = false;
    uint8_t lastRxIdentity = 0;
    uint8_t lastRxControl = 0;
    uint8_t lastRxSequence = 0;
    uint16_t lastRxCrc = 0;
    uint8_t expectedRxSequence = 0;

    void resetSerial();
    void startStage();
    bool finishStage(uint64_t now);
    void beginRequest(const uint8_t* payload, size_t length);
    void transmit(uint64_t now);
    void sendControl(uint8_t control);
    RequestStatus serviceRequest(uint64_t now);
    RequestStatus requestFailed(const __FlashStringHelper* reason);
    RequestStatus rejectPacket(const __FlashStringHelper* reason, uint64_t now);
    void logFailure(const __FlashStringHelper* reason);
    const char* stageName() const;
    void abortCycle(uint64_t now);
    void discardInput();
    void fail(const __FlashStringHelper* message);
    int32_t readInt32(const uint8_t* value) const;
    bool parseTable(const uint8_t* response, size_t responseLength, int32_t* values, size_t count);
};

#endif
