#include "IEC6205675.h"
#include "lwip/def.h"
#include "Timezone.h"

IEC6205675::IEC6205675(const char* d, uint8_t useMeterType, uint8_t distributionSystem, CosemDateTime packageTimestamp, HDLCConfig* hc) {
    double val;
    char str[64];

    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};
    TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};
    Timezone tz(CEST, CET);

    this->packageTimestamp = getTimestamp(packageTimestamp);

    val = getNumber(AMS_OBIS_ACTIVE_IMPORT, sizeof(AMS_OBIS_ACTIVE_IMPORT), ((char *) (d)));
    if(val == NOVALUE) {
        CosemData* data = getCosemDataAt(1, ((char *) (d)));
        
        // Kaifa special case...
        if(data->base.type == CosemTypeOctetString) {
            this->packageTimestamp = this->packageTimestamp > 0 ? tz.toUTC(this->packageTimestamp) : 0;

            memcpy(str, data->oct.data, data->oct.length);
            str[data->oct.length] = 0x00;
            String listId = String(str);
            if(listId.startsWith("KFM_001")) {
                this->listId = listId;
                meterType = AmsTypeKaifa;

                int idx = 0;
                data = getCosemDataAt(idx, ((char *) (d)));
                idx+=2;
                if(data->base.length == 0x0D || data->base.length == 0x12) {
                    listType = data->base.length == 0x12 ? 3 : 2;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterId = String(str);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterModel = String(str);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportPower = ntohl(data->dlu.data);
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportPower = ntohl(data->dlu.data);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveImportPower = ntohl(data->dlu.data);
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveExportPower = ntohl(data->dlu.data);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1current = ntohl(data->dlu.data) / 1000.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2current = ntohl(data->dlu.data) / 1000.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3current = ntohl(data->dlu.data) / 1000.0;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1voltage = ntohl(data->dlu.data) / 10.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2voltage = ntohl(data->dlu.data) / 10.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3voltage = ntohl(data->dlu.data) / 10.0;
                } else if(data->base.length == 0x09 || data->base.length == 0x0E) {
                    listType = data->base.length == 0x0E ? 3 : 2;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterId = String(str);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterModel = String(str);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportPower = ntohl(data->dlu.data);
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportPower = ntohl(data->dlu.data);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveImportPower = ntohl(data->dlu.data);
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveExportPower = ntohl(data->dlu.data);

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1current = ntohl(data->dlu.data) / 1000.0;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1voltage = ntohl(data->dlu.data) / 10.0;
                }

                if(listType >= 2 && memcmp(meterModel.c_str(), "MA304T3", 7) == 0) {
                    l2current = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
                    l2voltage = sqrt(pow(l1voltage - l3voltage * cos(60 * (PI/180)), 2) + pow(l3voltage * sin(60 * (PI/180)),2));
                }

                if(listType == 3) {
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    switch(data->base.type) {
                        case CosemTypeOctetString: {
                            if(data->oct.length == 0x0C) {
                                AmsOctetTimestamp* amst = (AmsOctetTimestamp*) data;
                                time_t ts = getTimestamp(amst->dt);
                                meterTimestamp = tz.toUTC(ts);
                            }
                        }
                    }

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportCounter = ntohl(data->dlu.data) / 1000.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportCounter = ntohl(data->dlu.data) / 1000.0;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveImportCounter = ntohl(data->dlu.data) / 1000.0;
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveExportCounter = ntohl(data->dlu.data) / 1000.0;
                }

                lastUpdateMillis = millis();
            }
        } else if(useMeterType == AmsTypeKaifa && data->base.type == CosemTypeDLongUnsigned) {
            this->packageTimestamp = this->packageTimestamp > 0 ? tz.toUTC(this->packageTimestamp) : 0;
            listType = 1;
            meterType = AmsTypeKaifa;
            activeImportPower = ntohl(data->dlu.data);
            lastUpdateMillis = millis();
        }
        // Kaifa end
    } else {
        listType = 1;
        activeImportPower = val;

        meterType = AmsTypeUnknown;
        CosemData* version = findObis(AMS_OBIS_VERSION, sizeof(AMS_OBIS_VERSION), d);
        if(version != NULL && version->base.type == CosemTypeString) {
            if(memcmp(version->str.data, "AIDON", 5) == 0) {
                meterType = AmsTypeAidon;
            } else if(memcmp(version->str.data, "Kamstrup", 8) == 0) {
                meterType = AmsTypeKamstrup;
            }
        } else {
            version = getCosemDataAt(1, ((char *) (d)));
            if(version->base.type == CosemTypeString) {
                if(memcmp(version->str.data, "Kamstrup", 8) == 0) {
                    meterType = AmsTypeKamstrup;
                }
            } 
        }
        // Try system title
        if(meterType == AmsTypeUnknown && hc != NULL) {
            if(memcmp(hc->system_title, "SAGY", 4)) {
                meterType = AmsTypeSagemcom;
            }
        }

        if(meterType == AmsTypeKamstrup || meterType == AmsTypeAidon) {
            this->packageTimestamp = this->packageTimestamp > 0 ? tz.toUTC(this->packageTimestamp) : 0;
        }

        uint8_t str_len = 0;
        str_len = getString(AMS_OBIS_VERSION, sizeof(AMS_OBIS_VERSION), ((char *) (d)), str);
        if(str_len > 0) {
            listId = String(str);
        }

        val = getNumber(AMS_OBIS_ACTIVE_EXPORT, sizeof(AMS_OBIS_ACTIVE_EXPORT), ((char *) (d)));
        if(val != NOVALUE) {
            activeExportPower = val;
        }

        val = getNumber(AMS_OBIS_REACTIVE_IMPORT, sizeof(AMS_OBIS_REACTIVE_IMPORT), ((char *) (d)));
        if(val != NOVALUE) {
            reactiveImportPower = val;
        }

        val = getNumber(AMS_OBIS_REACTIVE_EXPORT, sizeof(AMS_OBIS_REACTIVE_EXPORT), ((char *) (d)));
        if(val != NOVALUE) {
            reactiveExportPower = val;
        }

        val = getNumber(AMS_OBIS_VOLTAGE_L1, sizeof(AMS_OBIS_VOLTAGE_L1), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l1voltage = val;
        }
        val = getNumber(AMS_OBIS_VOLTAGE_L2, sizeof(AMS_OBIS_VOLTAGE_L2), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l2voltage = val;
        }
        val = getNumber(AMS_OBIS_VOLTAGE_L3, sizeof(AMS_OBIS_VOLTAGE_L3), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l3voltage = val;
        }

        val = getNumber(AMS_OBIS_CURRENT_L1, sizeof(AMS_OBIS_CURRENT_L1), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l1current = val;
        }
        val = getNumber(AMS_OBIS_CURRENT_L2, sizeof(AMS_OBIS_CURRENT_L2), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l2current = val;
        }
        val = getNumber(AMS_OBIS_CURRENT_L3, sizeof(AMS_OBIS_CURRENT_L3), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 2;
            l3current = val;
        }

        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_COUNT, sizeof(AMS_OBIS_ACTIVE_IMPORT_COUNT), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 3;
            activeImportCounter = val / 1000.0;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_COUNT, sizeof(AMS_OBIS_ACTIVE_EXPORT_COUNT), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 3;
            activeExportCounter = val / 1000.0;
        }
        val = getNumber(AMS_OBIS_REACTIVE_IMPORT_COUNT, sizeof(AMS_OBIS_REACTIVE_IMPORT_COUNT), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 3;
            reactiveImportCounter = val / 1000.0;
        }
        val = getNumber(AMS_OBIS_REACTIVE_EXPORT_COUNT, sizeof(AMS_OBIS_REACTIVE_EXPORT_COUNT), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 3;
            reactiveExportCounter = val / 1000.0;
        }

        str_len = getString(AMS_OBIS_METER_MODEL, sizeof(AMS_OBIS_METER_MODEL), ((char *) (d)), str);
        if(str_len > 0) {
            meterModel = String(str);
        } else {
            str_len = getString(AMS_OBIS_METER_MODEL_2, sizeof(AMS_OBIS_METER_MODEL_2), ((char *) (d)), str);
            if(str_len > 0) {
                meterModel = String(str);
            }
        }

        str_len = getString(AMS_OBIS_METER_ID, sizeof(AMS_OBIS_METER_ID), ((char *) (d)), str);
        if(str_len > 0) {
            meterId = String(str);
        } else {
            str_len = getString(AMS_OBIS_METER_ID_2, sizeof(AMS_OBIS_METER_ID_2), ((char *) (d)), str);
            if(str_len > 0) {
                meterId = String(str);
            }
        }

        CosemData* meterTs = findObis(AMS_OBIS_METER_TIMESTAMP, sizeof(AMS_OBIS_METER_TIMESTAMP), ((char *) (d)));
        if(meterTs != NULL) {
            AmsOctetTimestamp* amst = (AmsOctetTimestamp*) meterTs;
            time_t ts = getTimestamp(amst->dt);
            if(meterType == AmsTypeKamstrup || meterType == AmsTypeAidon) {
                meterTimestamp = tz.toUTC(ts);
            } else {
                meterTimestamp = ts;
            }
        }

        val = getNumber(AMS_OBIS_POWER_FACTOR, sizeof(AMS_OBIS_POWER_FACTOR), ((char *) (d)));
        if(val != NOVALUE) {
            powerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L1, sizeof(AMS_OBIS_POWER_FACTOR_L1), ((char *) (d)));
        if(val != NOVALUE) {
            l1PowerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L2, sizeof(AMS_OBIS_POWER_FACTOR_L2), ((char *) (d)));
        if(val != NOVALUE) {
            l2PowerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L3, sizeof(AMS_OBIS_POWER_FACTOR_L3), ((char *) (d)));
        if(val != NOVALUE) {
            l3PowerFactor = val;
        }

        if(meterType == AmsTypeKamstrup) {
            if(listType >= 3) {
                activeImportCounter *= 10;
                activeExportCounter *= 10;
                reactiveImportCounter *= 10;
                reactiveExportCounter *= 10;
            }
            if(l1current != 0)
                l1current /= 100;
            if(l2current != 0)
                l2current /= 100;
            if(l3current != 0)
                l3current /= 100;
            if(powerFactor != 0)
                powerFactor /= 100;
            if(l1PowerFactor != 0)
                l1PowerFactor /= 100;
            if(l2PowerFactor != 0)
                l2PowerFactor /= 100;
            if(l3PowerFactor != 0)
                l3PowerFactor /= 100;
        } else if(meterType == AmsTypeSagemcom) {
            CosemData* meterTs = getCosemDataAt(1, ((char *) (d)));
            if(meterTs != NULL) {
                AmsOctetTimestamp* amst = (AmsOctetTimestamp*) meterTs;
                time_t ts = getTimestamp(amst->dt);
                meterTimestamp = ts;
            }

            CosemData* mid = getCosemDataAt(58, ((char *) (d))); // TODO: Get last item
            if(mid != NULL) {
                switch(mid->base.type) {
                    case CosemTypeString:
                        memcpy(&meterId, mid->str.data, mid->str.length);
                        meterId[mid->str.length] = 0;
                        break;
                    case CosemTypeOctetString:
                        memcpy(&meterId, mid->oct.data, mid->oct.length);
                        meterId[mid->oct.length] = 0;
                        break;
                }
            }
        }

        lastUpdateMillis = millis();
    }

    threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
    if(!threePhase)
        twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);

    // Special case for Norwegian IT/TT meters that does not report all values
    if(distributionSystem == 1) {
        if(threePhase) {
            if(l2current == 0.0 && l1current > 0.0 && l3current > 0.0) {
                l2current = (((activeImportPower - activeExportPower) * sqrt(3)) - (l1voltage * l1current) - (l3voltage * l3current)) / l2voltage;
                if(activeExportPower == 0.0) {
                    l2current = max((float) 0.0, l2current);
                }
            }
        } else if(twoPhase && l1current > 0.0 && l2current > 0.0 && l3current > 0.0) {
            l2voltage = sqrt(pow(l1voltage - l3voltage * cos(60.0 * (PI/180.0)), 2) + pow(l3voltage * sin(60.0 * (PI/180.0)),2));
            threePhase = true;
        }
    }
}

CosemData* IEC6205675::getCosemDataAt(uint8_t index, const char* ptr) {
    CosemData* item = (CosemData*) ptr;
    int i = 0;
    char* pos = (char*) ptr;
    do {
        item = (CosemData*) pos;
        if(i == index) return item;
        switch(item->base.type) {
            case CosemTypeArray:
            case CosemTypeStructure:
                pos += 2;
                break;
            case CosemTypeOctetString:
            case CosemTypeString:
                pos += 2 + item->base.length;
                break;
            case CosemTypeLongSigned:
                pos += 5;
                break;
            case CosemTypeLongUnsigned:
                pos += 3;
                break;
            case CosemTypeDLongUnsigned:
                pos += 5;
                break;
            case CosemTypeNull:
                return NULL;
            default:
                pos += 2;
        }
        i++;
    } while(item->base.type != HDLC_FLAG);
    return NULL;
}

CosemData* IEC6205675::findObis(uint8_t* obis, int matchlength, const char* ptr) {
    CosemData* item = (CosemData*) ptr;
    int ret = 0;
    char* pos = (char*) ptr;
    do {
        item = (CosemData*) pos;
        if(ret == 1) return item;
        switch(item->base.type) {
            case CosemTypeArray:
            case CosemTypeStructure:
                pos += 2;
                break;
            case CosemTypeOctetString: {
                ret = 1;
                uint8_t* found = item->oct.data;
                int x = 6 - matchlength;
                for(int i = x; i < 6; i++) {
                    if(found[i] != obis[i-x]) ret = 0;
                }
            } // Fallthrough
            case CosemTypeString: {
                pos += 2 + item->base.length;
                break;
            }
            case CosemTypeLongSigned:
                pos += 5;
                break;
            case CosemTypeLongUnsigned:
                pos += 3;
                break;
            case CosemTypeDLongUnsigned:
                pos += 5;
                break;
            case CosemTypeNull:
                return NULL;
            default:
                pos += 2;
        }
    } while(item->base.type != HDLC_FLAG);
    return NULL;
}

uint8_t IEC6205675::getString(uint8_t* obis, int matchlength, const char* ptr, char* target) {
    CosemData* item = findObis(obis, matchlength, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeString:
                memcpy(target, item->str.data, item->str.length);
                target[item->str.length] = 0;
                return item->str.length;
            case CosemTypeOctetString:
                memcpy(target, item->oct.data, item->oct.length);
                target[item->oct.length] = 0;
                return item->oct.length;
        }
    }
    return 0;
}

double IEC6205675::getNumber(uint8_t* obis, int matchlength, const char* ptr) {
    CosemData* item = findObis(obis, matchlength, ptr);
    return getNumber(item);
}

double IEC6205675::getNumber(CosemData* item) {
    if(item != NULL) {
        double ret = 0.0;
        char* pos = ((char*) item);
        switch(item->base.type) {
            case CosemTypeLongUnsigned: {
                uint16_t u16 = ntohs(item->lu.data);
                ret = u16;
                pos += 3;
                break;
            }
            case CosemTypeDLongUnsigned: {
                uint32_t u32 = ntohl(item->dlu.data);
                ret = u32;
                pos += 5;
                break;
            }
            case CosemTypeLongSigned: {
                int16_t i16 = ntohs(item->ls.data);
                ret = i16;
                pos += 3;
                break;
            }
        }
        if(pos != NULL) {
            if(*pos++ == 0x02 && *pos++ == 0x02) {
                int8_t scale = *++pos;
                ret *= pow(10, scale);
            }
        }
        return ret;
    }
    return NOVALUE;
}

time_t IEC6205675::getTimestamp(uint8_t* obis, int matchlength, const char* ptr) {
    CosemData* item = findObis(obis, matchlength, ptr);
    if(item != NULL) {
        switch(item->base.type) {
            case CosemTypeOctetString: {
                if(item->oct.length == 0x0C) {
                    AmsOctetTimestamp* ts = (AmsOctetTimestamp*) item;
                    return getTimestamp(ts->dt);
                }
            }
        }
    }
    return 0;
}

time_t IEC6205675::getTimestamp(CosemDateTime timestamp) {
    tmElements_t tm;
    uint16_t year = ntohs(timestamp.year);
    if(year < 1970) return 0;
    tm.Year = year - 1970;
    tm.Month = timestamp.month;
    tm.Day = timestamp.dayOfMonth;
    tm.Hour = timestamp.hour;
    tm.Minute = timestamp.minute;
    tm.Second = timestamp.second;

    //Serial.printf("\nY: %d, M: %d, D: %d, h: %d, m: %d, s: %d, deviation: 0x%2X, status: 0x%1X\n", tm.Year, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second, timestamp.deviation, timestamp.status);

    time_t time = makeTime(tm);
    int16_t deviation = ntohs(timestamp.deviation);
    if(deviation >= -720 && deviation <= 720) {
        time -= deviation * 60;
    }
    return time;
}
