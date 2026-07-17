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
    uint8_t failures = 0;
    int lastError = 0;
    uint64_t nextPoll = 0;
    uint64_t sessionStarted = 0;
    int32_t table23[2] = {};
    int32_t table28[26] = {};
    size_t table28Values = 0;

    void resetSerial();
    bool openSession();
    void closeSession();
    bool poll();
    bool request(const uint8_t* payload, size_t length, uint8_t* response, size_t& responseLength);
    bool sendFrame(const uint8_t* payload, size_t length);
    bool receiveMessage(uint8_t* response, size_t& responseLength);
    bool receiveFrame(uint8_t* payload, size_t& payloadLength, uint8_t& control, uint8_t& sequence);
    bool readTable(uint16_t table, uint8_t* response, size_t& responseLength);
    bool readPartialTable(uint16_t table, uint16_t length, uint8_t* response, size_t& responseLength);
    bool waitByte(uint8_t& value, uint32_t timeout = 2000);
    bool readExact(uint8_t* target, size_t length, uint32_t timeout = 2000);
    void discardInput();
    void fail(const __FlashStringHelper* message);
    int32_t readInt32(const uint8_t* value) const;
    bool parseTable(const uint8_t* response, size_t responseLength, int32_t* values, size_t count);
};

#endif
