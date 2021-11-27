#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "EEPROM.h"
#include "LittleFS.h"
#include "AmsStorage.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) {
    day.version = 3;
    month.version = 4;
    this->debugger = debugger;
}

void AmsDataStorage::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool AmsDataStorage::update(AmsData* data) {
    if(data->getListType() != 3) return false;
    time_t now = time(nullptr);
    if(debugger->isActive(RemoteDebug::DEBUG)) {
        debugger->printf("(AmsDataStorage) Time is: %d\n", now);
    }
    if(now < EPOCH_2021_01_01) {
        now = data->getMeterTimestamp();
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) New time is: %d\n", now);
        }
    }
    if(now-day.lastMeterReadTime < 3590) {
        if(debugger->isActive(RemoteDebug::INFO)) {
            debugger->printf("(AmsDataStorage) It is only %d seconds since last update, ignoring\n", (now-day.lastMeterReadTime));
        }
        return false;
    }

    tmElements_t tm;
    breakTime(now, tm);

    if(tm.Minute > 5) {
        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) Already %d minutes into the hour, ignoring\n", tm.Minute);
        }
        return false;
    }

    int16_t val = (day.activeImport == 0 ? 0 : ((data->getActiveImportCounter()*1000) - day.activeImport) - ((data->getActiveExportCounter()*1000) - day.activeExport)) / 10;
    if(debugger->isActive(RemoteDebug::DEBUG)) {
        debugger->printf("(AmsDataStorage) Usage for hour %d: %d", tm.Hour, val);
    }

    if(tm.Hour == 1) {
        day.h00 = val;
    } else if(tm.Hour == 2) {
        day.h01 = val;
    } else if(tm.Hour == 3) {
        day.h02 = val;
    } else if(tm.Hour == 4) {
        day.h03 = val;
    } else if(tm.Hour == 5) {
        day.h04 = val;
    } else if(tm.Hour == 6) {
        day.h05 = val;
    } else if(tm.Hour == 7) {
        day.h06 = val;
    } else if(tm.Hour == 8) {
        day.h07 = val;
    } else if(tm.Hour == 9) {
        day.h08 = val;
    } else if(tm.Hour == 10) {
        day.h09 = val;
    } else if(tm.Hour == 11) {
        day.h10 = val;
    } else if(tm.Hour == 12) {
        day.h11 = val;
    } else if(tm.Hour == 13) {
        day.h12 = val;
    } else if(tm.Hour == 14) {
        day.h13 = val;
    } else if(tm.Hour == 15) {
        day.h14 = val;
    } else if(tm.Hour == 16) {
        day.h15 = val;
    } else if(tm.Hour == 17) {
        day.h16 = val;
    } else if(tm.Hour == 18) {
        day.h17 = val;
    } else if(tm.Hour == 19) {
        day.h18 = val;
    } else if(tm.Hour == 20) {
        day.h19 = val;
    } else if(tm.Hour == 21) {
        day.h20 = val;
    } else if(tm.Hour == 22) {
        day.h21 = val;
    } else if(tm.Hour == 23) {
        day.h22 = val;
    } else if(tm.Hour == 0) {
        day.h23 = val;
    }
    day.activeImport = data->getActiveImportCounter()*1000;
    day.activeExport = data->getActiveExportCounter()*1000;
    day.lastMeterReadTime = now;

    // Update month plot
    if(tz != NULL) {
        time_t local = tz->toLocal(now);
        breakTime(local, tm);
    }

    if(tm.Hour == 0) {
        val = (month.activeImport == 0 ? 0 : ((data->getActiveImportCounter()*1000) - month.activeImport) - ((data->getActiveExportCounter()*1000) - month.activeExport)) / 10;

        if(debugger->isActive(RemoteDebug::DEBUG)) {
            debugger->printf("(AmsDataStorage) Usage for day %d: %d", tm.Day, val);
        }

        if(tm.Day == 1) {
            time_t yesterday = now-3600;
            breakTime(yesterday, tm);
            if(tm.Day == 29) {
                month.d28 = val;
            } else if(tm.Day == 30) {
                month.d29 = val;
            } else if(tm.Day == 31) {
                month.d30 = val;
            }
        } else if(tm.Day == 2) {
            month.d01 = val;
        } else if(tm.Day == 3) {
            month.d02 = val;
        } else if(tm.Day == 4) {
            month.d03 = val;
        } else if(tm.Day == 5) {
            month.d04 = val;
        } else if(tm.Day == 6) {
            month.d05 = val;
        } else if(tm.Day == 7) {
            month.d06 = val;
        } else if(tm.Day == 8) {
            month.d07 = val;
        } else if(tm.Day == 9) {
            month.d08 = val;
        } else if(tm.Day == 10) {
            month.d09 = val;
        } else if(tm.Day == 11) {
            month.d10 = val;
        } else if(tm.Day == 12) {
            month.d11 = val;
        } else if(tm.Day == 13) {
            month.d12 = val;
        } else if(tm.Day == 14) {
            month.d13 = val;
        } else if(tm.Day == 15) {
            month.d14 = val;
        } else if(tm.Day == 16) {
            month.d15 = val;
        } else if(tm.Day == 17) {
            month.d16 = val;
        } else if(tm.Day == 18) {
            month.d17 = val;
        } else if(tm.Day == 19) {
            month.d18 = val;
        } else if(tm.Day == 20) {
            month.d19 = val;
        } else if(tm.Day == 21) {
            month.d20 = val;
        } else if(tm.Day == 22) {
            month.d21 = val;
        } else if(tm.Day == 23) {
            month.d22 = val;
        } else if(tm.Day == 24) {
            month.d23 = val;
        } else if(tm.Day == 25) {
            month.d24 = val;
        } else if(tm.Day == 26) {
            month.d25 = val;
        } else if(tm.Day == 27) {
            month.d26 = val;
        } else if(tm.Day == 28) {
            month.d27 = val;
        } else if(tm.Day == 29) {
            month.d28 = val;
        } else if(tm.Day == 30) {
            month.d29 = val;
        } else if(tm.Day == 31) {
            month.d30 = val;
        }
        month.activeImport = data->getActiveImportCounter()*1000;
        month.activeExport = data->getActiveExportCounter()*1000;
    }
    
    return true;
}

DayDataPoints AmsDataStorage::getDayDataPoints() {
    return day;
}

MonthDataPoints AmsDataStorage::getMonthDataPoints() {
    return month;
}

bool AmsDataStorage::load(AmsData* meterState) {
    if(!LittleFS.begin()) {
        printE("Unable to load LittleFS");
        return false;
    }
    bool ret = false;
    if(LittleFS.exists(FILE_DAYPLOT)) {
        File file = LittleFS.open(FILE_DAYPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        DayDataPoints* day = (DayDataPoints*) buf;
        file.close();

        if(day->version == 3) {
            memcpy(&this->day, day, sizeof(this->day));
            ret = true;
        } else {
            ret = false;
        }
    }

    if(LittleFS.exists(FILE_MONTHPLOT)) {
        File file = LittleFS.open(FILE_MONTHPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        MonthDataPoints* month = (MonthDataPoints*) buf;
        file.close();

        if(month->version == 3) { // dev-1.6
            month->d25 = month->d26;
            month->d26 = month->d27;
            month->d27 = month->d28;
            month->d28 = month->d29;
            month->d29 = month->d30;
            month->d30 = month->d31;
            month->version = 4;
        }

        if(month->version == 4) {
            memcpy(&this->month, month, sizeof(this->month));
            ret = true;
        } else {
            ret = false;
        }
    }

    LittleFS.end();

    return ret;
}

bool AmsDataStorage::save() {
    if(!LittleFS.begin()) {
        printE("Unable to load LittleFS");
        return false;
    }
    {
        File file = LittleFS.open(FILE_DAYPLOT, "w");
        char buf[sizeof(day)];
        memcpy(buf, &day, sizeof(day));
        for(int i = 0; i < sizeof(day); i++) {
            file.write(buf[i]);
        }
        file.close();
    }
    {
        File file = LittleFS.open(FILE_MONTHPLOT, "w");
        char buf[sizeof(month)];
        memcpy(buf, &month, sizeof(month));
        for(int i = 0; i < sizeof(month); i++) {
            file.write(buf[i]);
        }
        file.close();
    }

    LittleFS.end();
    return true;
}

void AmsDataStorage::printD(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf(String("(AmsDataStorage)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsDataStorage::printI(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::INFO)) debugger->printf(String("(AmsDataStorage)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsDataStorage::printW(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf(String("(AmsDataStorage)" + fmt + "\n").c_str(), args);
	va_end(args);
}

void AmsDataStorage::printE(String fmt, ...) {
	va_list args;
 	va_start(args, fmt);
	if(debugger->isActive(RemoteDebug::ERROR)) debugger->printf(String("(AmsDataStorage)" + fmt + "\n").c_str(), args);
	va_end(args);
}
