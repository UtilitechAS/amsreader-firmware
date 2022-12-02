#ifndef _PRICESCONTAINER_H
#define _PRICESCONTAINER_H
struct PricesContainer {
    char currency[4];
    char measurementUnit[4];
    int32_t points[24];
};
#endif
