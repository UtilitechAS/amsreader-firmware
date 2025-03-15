/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PRICESCONTAINER_H
#define _PRICESCONTAINER_H

#define PRICE_NO_VALUE -127

class PricesContainer {
public:
    PricesContainer(char* source);

    void setup(uint8_t resolutionInMinutes, uint8_t hoursThisDay);

    char* getSource();
    void setCurrency(char* currency);
    char* getCurrency();

    uint8_t getResolutionInMinutes();
    uint8_t getHours();
    uint8_t getNumberOfPoints();

    void setPrice(uint8_t point, int32_t value);
    void setPrice(uint8_t point, float value);
    bool hasPrice(uint8_t point);
    float getPrice(uint8_t point); // int32_t / 10_000

private:
    char source[4];
    char currency[4];
    uint8_t resolutionInMinutes;
    uint8_t hours;
    uint8_t numberOfPoints;
    int32_t *points;
};
#endif
