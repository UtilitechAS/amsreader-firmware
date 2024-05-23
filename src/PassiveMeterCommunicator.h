/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PASSIVEMETERCOMMUNICATOR_H
#define _PASSIVEMETERCOMMUNICATOR_H

#include "MeterCommunicator.h"
#include "RemoteDebug.h"
#include "AmsConfiguration.h"
#include "DataParsers.h"
#include "Timezone.h"
#include "PassthroughMqttHandler.h"

#if defined(ESP8266)
#include "SoftwareSerial.h"
#endif

class PassiveMeterCommunicator : public MeterCommunicator  {
public:
    PassiveMeterCommunicator(RemoteDebug* debugger);
    void configure(MeterConfig&, Timezone*);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);
    void setPassthroughMqttHandler(PassthroughMqttHandler*);

    HardwareSerial* getHwSerial();
    void rxerr(int err);

protected:
    RemoteDebug* debugger = NULL;
    MeterConfig meterConfig;
    bool configChanged = false;
    Timezone* tz;

    PassthroughMqttHandler* pt = NULL;

    uint8_t *hanBuffer = NULL;
    uint16_t hanBufferSize = 0;
    Stream *hanSerial;
    #if defined(ESP8266)
    SoftwareSerial *swSerial = NULL;
    #endif
    HardwareSerial *hwSerial = NULL;
    uint8_t rxBufferErrors = 0;

    bool autodetect = false, validDataReceived = false;
    unsigned long meterAutodetectLastChange = 0;
    long rate = 10000;
    uint32_t autodetectBaud = 0;
    uint8_t autodetectParity = 11;
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
    void debugPrint(byte *buffer, int start, int length);
    void printHanReadError(int pos);
    void handleAutodetect(unsigned long now);
    uint32_t detectBaudRate(uint8_t pin);
};

#endif
