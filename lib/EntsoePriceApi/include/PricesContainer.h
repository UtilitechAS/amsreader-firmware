#ifndef _PRICESCONTAINER_H
#define _PRICESCONTAINER_H
struct PricesContainer {
    char currency[4];
    char measurementUnit[4];
    int32_t points[25];
    char source[4];
};
#endif
