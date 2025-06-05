/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include <stdint.h>

#ifndef _PRICESCONTAINER_H
#define _PRICESCONTAINER_H

#define PRICE_NO_VALUE -127
#define PRICE_DIRECTION_IMPORT 0x01
#define PRICE_DIRECTION_EXPORT 0x02
#define PRICE_DIRECTION_BOTH 0x03

class PricesContainer {
public:
    PricesContainer(char* source);

    void setup(uint8_t resolutionInMinutes, uint8_t numberOfPoints, bool differentExportPrices);

    char* getSource();
    void setCurrency(char* currency);
    char* getCurrency();

    bool isExportPricesDifferentFromImport() {
        return differentExportPrices;
    }

    uint8_t getResolutionInMinutes();
    uint8_t getNumberOfPoints();

    void setPrice(uint8_t point, float value, uint8_t direction);
    bool hasPrice(uint8_t point, uint8_t direction);
    float getPrice(uint8_t point, uint8_t direction); // int32_t / 10_000

private:
    char source[4];
    char currency[4];
    uint8_t resolutionInMinutes;
    bool differentExportPrices;
    uint8_t numberOfPoints;
    int32_t *points;
};
#endif
