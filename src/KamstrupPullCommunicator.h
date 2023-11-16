/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _KAMSTRUPPULLCOMMUNICATOR_H
#define _KAMSTRUPPULLCOMMUNICATOR_H

#include "PassiveMeterCommunicator.h"
#include "RemoteDebug.h"
#include "AmsConfiguration.h"
#include "DataParsers.h"
#include "SoftwareSerial.h"
#include "Timezone.h"
#include "PassthroughMqttHandler.h"
#include "HdlcParser.h"
#include "crc.h"
#include "OBIScodes.h"

#define BUF_SIZE_HAN (1280)

#define STATE_DISCONNECTED 0
#define STATE_CONNECTING 1
#define STATE_CONNECTED_NOT_ASSOCIATED 2
#define STATE_CONNECTED_ASSOCIATING 3
#define STATE_CONNECTED_ASSOCIATED 4
#define STATE_CONNECTION_BROKEN 7
#define STATE_DISCONNECT 8
#define STATE_DISCONNECTING 9

struct ConnectParameter2b {
    uint8_t type;
    uint8_t length;
    uint16_t data;
} __attribute__((packed));

struct ConnectParameter4b {
    uint8_t type;
    uint8_t length;
    uint32_t data;
} __attribute__((packed));

class KamstrupPullCommunicator : public PassiveMeterCommunicator  {
public:
    KamstrupPullCommunicator(RemoteDebug* debugger) : PassiveMeterCommunicator(debugger) {};
    void configure(MeterConfig&, Timezone*);
    bool loop();
    AmsData* getData(AmsData& meterState);
    int getLastError();
    bool isConfigChanged();
    void getCurrentConfig(MeterConfig& meterConfig);
    void setPassthroughMqttHandler(PassthroughMqttHandler*);

    HardwareSerial* getHwSerial();
    void rxerr(int err);

private:
    uint8_t state = STATE_DISCONNECTED;
    uint64_t lastLoop = 0;
    uint64_t lastMessageTime = 0;
    String passkey = "12345";
    uint8_t clientSap = 0x25;
    uint8_t serverSap = 0x21;
    uint8_t obisPosition = 2;
    OBIS_code_t currentObis = OBIS_NULL;

    void setupHanPort(uint32_t baud, uint8_t parityOrdinal, bool invert);
    void sendConnectMessage();
    bool checkForConnectConfirmed();
    void sendAssociateMessage();
    bool checkForAssociationConfirmed();
    bool requestData();
    void sendDisconnectMessage();
    bool checkForDisconnectMessage();

    void debugPrint(byte *buffer, int start, int length);
};

#endif
