#ifndef _IMPULSEAMSDATA_H
#define _IMPULSEAMSDATA_H

#include "AmsData.h"

class ImpulseAmsData : public AmsData {
public:
    ImpulseAmsData(AmsData &state, uint16_t pulsePerKwh, uint8_t pulses);
};

#endif
