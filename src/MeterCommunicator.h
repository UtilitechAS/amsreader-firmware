/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _METERCOMMUNICATOR_H
#define _METERCOMMUNICATOR_H

#include <Arduino.h>
#include "RemoteDebug.h"
#include "AmsData.h"
#include "AmsConfiguration.h"

class MeterCommunicator {
public:
    virtual ~MeterCommunicator() {};
    virtual void configure(MeterConfig&, Timezone*);
    virtual bool loop();
    virtual AmsData* getData(AmsData& meterState);
    virtual int getLastError();
    virtual bool isConfigChanged();
    virtual void getCurrentConfig(MeterConfig& meterConfig);
};

#endif
