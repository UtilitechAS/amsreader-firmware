#pragma once

#include "AmsDataStorage.h"
#include "AmsConfiguration.h"

class AmsJsonGenerator {
public:
    static void generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    static void generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    static void generateConfigurationJson(AmsConfiguration* config, char* buf, size_t bufSize);
};