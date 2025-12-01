/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PASSIVEMETERCOMMUNICATOR_H
#define _PASSIVEMETERCOMMUNICATOR_H

#include "MeterCommunicator.h"
#if defined(AMS_REMOTE_DEBUG)
#include "RemoteDebug.h"
#endif
#include "AmsConfiguration.h"
#include "DataParsers.h"
#include "Timezone.h"
#include "AmsMqttHandler.h"

#if defined(ESP8266)
#include "SoftwareSerial.h"
#endif

const uint32_t AUTO_BAUD_RATES[] = { 2400, 9600, 115200 };

class PassiveMeterCommunicator : public MeterCommunicator  {
public:
    #if defined(AMS_REMOTE_DEBUG)
    PassiveMeterCommunicator(RemoteDebug* debugger);
    #else
    PassiveMeterCommunicator(Stream* debugger);
    #endif
    void configure(MeterConfig&, Timezone*);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void ackConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);

    HardwareSerial* getHwSerial();
    void rxerr(int err);

protected:
    #if defined(AMS_REMOTE_DEBUG)
    RemoteDebug* debugger = NULL;
    #else
    Stream* debugger = NULL;
    #endif
    MeterConfig meterConfig;
    bool configChanged = false;
    Timezone* tz;

    uint8_t *hanBuffer = NULL;
    uint16_t hanBufferSize = 0;
    Stream *hanSerial;
    #if defined(ESP8266)
    SoftwareSerial *swSerial = NULL;
    #endif
    HardwareSerial *hwSerial = NULL;
    uint8_t rxBufferErrors = 0;

    bool autodetect = false;
    uint8_t validDataReceived = 0;
    unsigned long meterAutodetectLastChange = 0;
    long rate = 10000;
    uint32_t autodetectBaud = 0;
    uint8_t autodetectParity = 11; // 8E1
    bool autodetectInvert = false;
    uint8_t autodetectCount = 0;

    bool dataAvailable = false;
    int len = 0;
    int pos = DATA_PARSE_INCOMPLETE;
    int lastError = DATA_PARSE_OK;
    bool serialInit = false;
    bool maxDetectPayloadDetectDone = false;
    uint8_t maxDetectedPayloadSize = 64;
    DataParserContext ctx = {0,0,0,0};

    HDLCParser *hdlcParser = NULL;
    MBUSParser *mbusParser = NULL;
    GBTParser *gbtParser = NULL;
    GCMParser *gcmParser = NULL;
    LLCParser *llcParser = NULL;
    DLMSParser *dlmsParser = NULL;
    DSMRParser *dsmrParser = NULL;

    void setupHanPort(uint32_t baud, uint8_t parityOrdinal, bool invert, bool passive = true);
    int16_t unwrapData(uint8_t *buf, DataParserContext &context);
    void printHanReadError(int pos);
    void handleAutodetect(unsigned long now);
    uint8_t getNextParity(uint8_t parityOrdinal);
};

#endif
