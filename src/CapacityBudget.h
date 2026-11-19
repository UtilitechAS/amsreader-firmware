/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#ifndef _CAPACITYBUDGET_H
#define _CAPACITYBUDGET_H

#include <stdint.h>
#include <string.h>

// Peak bookkeeping for the capacity tariff, kept free of Arduino so it can be tested on
// native. EnergyAccounting stores the highest hour of up to 5 days this month; the month
// max is the average of the top `hours` of them. The hour budget below simulates closing
// the current hour with this same code, so it cannot disagree with what EnergyAccounting
// will record when the hour ends.

struct EnergyAccountingPeak {
    uint8_t day;
    uint8_t hour;
    uint16_t value; // 10 Wh units
};

#define CAPACITY_NO_THRESHOLD 0xFFFF // thresholds[9] sentinel: no step above the previous one

#define CAPACITY_STATUS_OK 0
#define CAPACITY_STATUS_WARNING 1
#define CAPACITY_STATUS_ALARM 2

// Record `val` as the closed hour of `day`. Returns true if the peaks changed.
static inline bool capacityUpdatePeaks(EnergyAccountingPeak peaks[5], uint16_t val, uint8_t day, uint8_t hour) {
    for(uint8_t i = 0; i < 5; i++) {
        if(peaks[i].day == day || peaks[i].day == 0) {
            if(val > peaks[i].value) {
                peaks[i].day = day;
                peaks[i].hour = hour;
                peaks[i].value = val;
                return true;
            }
            return false;
        }
    }
    uint16_t test = val;
    uint8_t idx = 255;
    for(uint8_t i = 0; i < 5; i++) {
        if(val > peaks[i].value) {
            if(test > peaks[i].value) {
                test = peaks[i].value;
                idx = i;
            }
        }
    }
    if(idx < 5) {
        peaks[idx].value = val;
        peaks[idx].day = day;
        return true;
    }
    return false;
}

// Sum of the top `hours` peaks (10 Wh units), and how many there were
static inline uint32_t capacityTopSum(const EnergyAccountingPeak peaks[5], uint8_t hours, uint8_t& count) {
    count = 0;
    uint32_t sum = 0;
    bool included[5] = { false, false, false, false, false };

    for(uint8_t x = 0; x < (hours < 5 ? hours : 5); x++) {
        uint8_t maxIdx = 0;
        uint16_t maxVal = 0;
        for(uint8_t i = 0; i < 5; i++) {
            if(included[i]) continue;
            if(peaks[i].day == 0) continue;
            if(peaks[i].value > maxVal) {
                maxVal = peaks[i].value;
                maxIdx = i;
            }
        }
        if(maxVal > 0) {
            included[maxIdx] = true;
            count++;
        }
    }

    for(uint8_t i = 0; i < 5; i++) {
        if(!included[i]) continue;
        sum += peaks[i].value;
    }
    return sum;
}

// Month max in 10 Wh units, truncated the same way as EnergyAccounting::getMonthMax()
static inline uint32_t capacityMonthMax(const EnergyAccountingPeak peaks[5], uint8_t hours) {
    uint8_t count;
    uint32_t sum = capacityTopSum(peaks, hours, count);
    return count > 0 ? sum / count : 0;
}

// The most the current hour of `day` can use (10 Wh units) without lifting the month max
// above `threshold` (kWh). When the month max is already above every step, the limit is
// the month max itself: the hour should not make it worse.
static inline uint16_t capacityHourBudget(const EnergyAccountingPeak peaks[5], uint8_t hours, uint8_t day, uint16_t threshold) {
    uint32_t limit = capacityMonthMax(peaks, hours);
    if(threshold != CAPACITY_NO_THRESHOLD && threshold * 100UL > limit) limit = threshold * 100UL;

    // A higher hour never lowers the month max, so search for the largest value that fits
    uint32_t lo = 0, hi = 0xFFFF;
    while(lo < hi) {
        uint32_t mid = (lo + hi + 1) / 2;
        EnergyAccountingPeak test[5];
        memcpy(test, peaks, sizeof(test));
        capacityUpdatePeaks(test, mid, day, 0);
        if(capacityMonthMax(test, hours) <= limit) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    return lo;
}

// Average power (kW) that can be added for the rest of the hour and still stay within
// `budget`; negative is how much has to be shed. Above the budget, all of the current
// power is reported as to be shed. The time is floored at 30 s so the value does not
// explode in the last seconds of the hour.
static inline float capacityHeadroom(float budget, float used, float powerKw, uint16_t secondsLeft) {
    float left = budget - used;
    if(left < 0) left = 0;
    return (left * 3600.0f / (secondsLeft < 30 ? 30 : secondsLeft)) - powerKw;
}

// Alarm when the hour is projected to go over the budget, warning within 10% of it
static inline uint8_t capacityStatus(float budget, float projected) {
    if(projected > budget) return CAPACITY_STATUS_ALARM;
    if(projected > budget * 0.9f) return CAPACITY_STATUS_WARNING;
    return CAPACITY_STATUS_OK;
}

static inline const char* capacityStatusName(uint8_t status) {
    if(status == CAPACITY_STATUS_ALARM) return "alarm";
    if(status == CAPACITY_STATUS_WARNING) return "warning";
    return "ok";
}

#endif
