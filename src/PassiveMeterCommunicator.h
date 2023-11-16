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
#include "SoftwareSerial.h"
#include "Timezone.h"
#include "PassthroughMqttHandler.h"

#define BUF_SIZE_HAN (1280)

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

    uint8_t hanBuffer[BUF_SIZE_HAN];
    Stream *hanSerial;
    SoftwareSerial *swSerial = NULL;
    HardwareSerial *hwSerial = NULL;
    uint8_t rxBufferErrors = 0;

    bool autodetect = false, validDataReceived = false;
    unsigned long meterAutodetectLastChange = 0;
    uint8_t meterAutoIndex = 0;
    uint32_t bauds[6];
    uint8_t parities[6];
    bool inverts[6];

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

    void setupHanPort(uint32_t baud, uint8_t parityOrdinal, bool invert);
    int16_t unwrapData(uint8_t *buf, DataParserContext &context);
    void debugPrint(byte *buffer, int start, int length);
    void printHanReadError(int pos);
    void handleAutodetect(unsigned long now);
};

#endif
