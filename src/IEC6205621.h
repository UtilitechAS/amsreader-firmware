#ifndef _IEC62056_21_H
#define _IEC62056_21_H

#include "AmsData.h"

class IEC6205621 : public AmsData {
public:
    IEC6205621(String payload);

private:
    String extract(String payload, String obis);
    double extractDouble(String payload, String obis);
};
#endif
