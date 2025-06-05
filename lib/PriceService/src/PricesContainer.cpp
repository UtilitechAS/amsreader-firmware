#include "PricesContainer.h"
#include <cstring>

PricesContainer::PricesContainer(char* source) {
    strncpy(this->source, source, 4);
}

void PricesContainer::setup(uint8_t resolutionInMinutes, uint8_t numberOfPoints, bool differentExportPrices) {
    this->resolutionInMinutes = resolutionInMinutes;
    this->differentExportPrices = differentExportPrices;
    this->numberOfPoints = numberOfPoints;
    this->points = new int32_t[numberOfPoints * (differentExportPrices ? 2 : 1)];
    memset(this->points, PRICE_NO_VALUE * 10000, numberOfPoints * (differentExportPrices ? 2 : 1) * sizeof(int32_t));
}

char* PricesContainer::getSource() {
    return this->source;
}

void PricesContainer::setCurrency(char* currency) {
    strncpy(this->currency, currency, 4);
}

char* PricesContainer::getCurrency() {
    return this->currency;
}

uint8_t PricesContainer::getResolutionInMinutes() {
    return this->resolutionInMinutes;
}

uint8_t PricesContainer::getNumberOfPoints() {
    return this->numberOfPoints;
}

void PricesContainer::setPrice(uint8_t point, float value, uint8_t direction) {
    if(direction == PRICE_DIRECTION_EXPORT && !differentExportPrices) {
        return; // Export prices not supported
    }
    if(direction != PRICE_DIRECTION_EXPORT) {
        points[point] = static_cast<int32_t>(value * 10000);
    }
    if(differentExportPrices && direction != PRICE_DIRECTION_IMPORT) {
        points[point + numberOfPoints] = static_cast<int32_t>(value * 10000);
    }
}

bool PricesContainer::hasPrice(uint8_t point, uint8_t direction) {
    float val = getPrice(point, direction);
    return val != PRICE_NO_VALUE;
}

float PricesContainer::getPrice(uint8_t point, uint8_t direction) {
    if(differentExportPrices && direction == PRICE_DIRECTION_EXPORT) {
        if(point < numberOfPoints) {
            return static_cast<float>(points[point + numberOfPoints]) / 10000.0f;
        }
    }

    if(differentExportPrices && direction == PRICE_DIRECTION_BOTH) return PRICE_NO_VALUE; // Can't get a price for both directions if the export prices are different

    if(point < numberOfPoints) {
        return static_cast<float>(points[point]) / 10000.0f;
    }
    
    return PRICE_NO_VALUE; // Invalid point
}