/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _REALTIMEPLOT_H
#define _REALTIMEPLOT_H

#include <stdint.h>
#include "AmsData.h"

#define REALTIME_SAMPLE 10000
#define REALTIME_SIZE 360

class RealtimePlot {
public:
    RealtimePlot();
    void update(AmsData& data);
    int32_t getValue(uint16_t req);
    int16_t getSize();

private:
    int8_t* values;
    uint8_t* scaling;

    unsigned long lastMillis = 0;
    double lastReading = 0;
    uint16_t lastPos = 0;
};
#endif
