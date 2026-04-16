/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */
#pragma once

#include "AmsDataStorage.h"

class AmsJsonGenerator {
public:
    static void generateDayPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
    static void generateMonthPlotJson(AmsDataStorage* ds, char* buf, size_t bufSize);
};