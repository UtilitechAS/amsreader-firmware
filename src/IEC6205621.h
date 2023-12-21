#ifndef _IEC62056_21_H
#define _IEC62056_21_H

#include "Arduino.h"
#include "AmsData.h"
#include "Timezone.h"
#include "AmsConfiguration.h"

class IEC6205621 : public AmsData {
public:
    IEC6205621(const char* payload, Timezone* tz, MeterConfig* meterConfig);

private:
    String extract(String payload, String obis);
    double extractDouble(String payload, String obis);
    float extractFloat(String payload, String obis);
};
#endif
