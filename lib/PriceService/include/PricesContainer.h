/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#ifndef _PRICESCONTAINER_H
#define _PRICESCONTAINER_H

#define PRICE_NO_VALUE -127

struct PricesContainer {
    char currency[4];
    char measurementUnit[4];
    int32_t points[25];
    char source[4];
};
#endif
