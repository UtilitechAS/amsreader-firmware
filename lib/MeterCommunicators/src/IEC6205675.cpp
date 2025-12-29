/**
 * @copyright Utilitech AS 2023
 * License: Fair Source
 * 
 */

#include "IEC6205675.h"
#include "lwip/def.h"
#include "Timezone.h"
#include "ntohll.h"
#include "Uptime.h"
#include "hexutils.h"

#if defined(AMS_REMOTE_DEBUG)
IEC6205675::IEC6205675(const char* d, Timezone* tz, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, AmsData &state, RemoteDebug* debugger) {
#else
IEC6205675::IEC6205675(const char* d, Timezone* tz, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, AmsData &state, Stream* debugger) {
#endif
    float val;
    char str[64];

    this->packageTimestamp = time(nullptr); // ctx.timestamp is mostly garbage, so we use current time as package timestamp

    val = getNumber(AMS_OBIS_ACTIVE_IMPORT, sizeof(AMS_OBIS_ACTIVE_IMPORT), ((char *) (d)));
    if(val == NOVALUE) {
        CosemData* data = getCosemDataAt(1, ((char *) (d)));
        
        // Kaifa special case...
        if(useMeterType == AmsTypeKaifa && data->base.type == CosemTypeDLongUnsigned) {
            #if defined(AMS_REMOTE_DEBUG)
            if (debugger->isActive(RemoteDebug::DEBUG))
            #endif
            debugger->printf_P(PSTR("Kaifa list 1\n"));

            this->packageTimestamp = this->packageTimestamp > 0 && tz != NULL ? tz->toUTC(this->packageTimestamp) : 0;
            listType = 1;
            meterType = AmsTypeKaifa;
            activeImportPower = ntohl(data->dlu.data);
            lastUpdateMillis = millis64();
        } else if(data->base.type == CosemTypeOctetString) {
            this->packageTimestamp = this->packageTimestamp > 0 && tz != NULL ? tz->toUTC(this->packageTimestamp) : 0;

            memcpy(str, data->oct.data, data->oct.length);
            str[data->oct.length] = 0x00;
            String listId = String(str);
            if(listId.startsWith(F("KFM_001"))) {
                this->listId = listId;
                meterType = AmsTypeKaifa;

                uint8_t idx = 0;
                data = getCosemDataAt(idx, ((char *) (d)));
                idx+=2;
                if(data->base.length == 0x0D || data->base.length == 0x12) {
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::DEBUG))
                    #endif
                    debugger->printf_P(PSTR("Kaifa list 3\n"));

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
                    #if defined(AMS_REMOTE_DEBUG)
                    if (debugger->isActive(RemoteDebug::DEBUG))
                    #endif
                    debugger->printf_P(PSTR("Kaifa list 2\n"));

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
                    l2voltage = sqrt(pow(l1voltage - l3voltage * cos(60 * (PI/180)), 2) + pow(l3voltage * sin(60 * (PI/180)),2));
                    l2currentMissing = true;
                }

                if(listType == 3) {
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    switch(data->base.type) {
                        case CosemTypeOctetString: {
                            if(data->oct.length == 0x0C) {
                                AmsOctetTimestamp* amst = (AmsOctetTimestamp*) data;
                                time_t ts = decodeCosemDateTime(amst->dt);
                                meterTimestamp = tz != NULL ? tz->toUTC(ts) : ts;
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

                lastUpdateMillis = millis64();
            } else if(listId.startsWith("ISK")) { // Iskra special case
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                debugger->printf_P(PSTR("Iskra list ID\n"));

                this->listId = listId;
                meterType = AmsTypeIskra;

                uint8_t idx = 0;
                data = getCosemDataAt(idx++, ((char *) (d)));
                if(data->base.length == 0x12) {
                    apply(state);
                    listType = state.getListType() > 4 ? state.getListType() : 4;

                    // 42.0.0 COSEM logical device name
                    idx++;

                    // 96.1.3 Device ID 4
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterId = String(str);

                    // 1.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportPower = ntohl(data->dlu.data);

                    // 2.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportPower = ntohl(data->dlu.data);

                    // 3.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveImportPower = ntohl(data->dlu.data);

                    // 4.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveExportPower = ntohl(data->dlu.data);

                    // 32.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1voltage = ntohs(data->lu.data) / 10.0;

                    // 52.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2voltage = ntohs(data->lu.data) / 10.0;

                    // 72.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3voltage = ntohs(data->lu.data) / 10.0;

                    // 31.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1current = ntohs(data->lu.data) / 100.0;

                    // 51.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2current = ntohs(data->lu.data) / 100.0;

                    // 71.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3current = ntohs(data->lu.data) / 100.0;

                    // 21.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1activeImportPower = ntohl(data->dlu.data);

                    // 41.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2activeImportPower = ntohl(data->dlu.data);

                    // 61.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3activeImportPower = ntohl(data->dlu.data);

                    // 22.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1activeExportPower = ntohl(data->dlu.data);

                    // 42.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2activeExportPower = ntohl(data->dlu.data);

                    // 62.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3activeExportPower = ntohl(data->dlu.data);
                    
                    lastUpdateMillis = millis64();
                } else if(data->base.length == 0x0C) {
                    CosemData* no3 = getCosemDataAt(3, ((char *) (d)));
                    if(no3->base.type == CosemTypeBoolean) {
                        apply(state);
                        listType = state.getListType() > 3 ? state.getListType() : 3;

                        // 42.0.0 COSEM logical device name
                        idx++;

                        // 96.1.3 Device ID 4
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        memcpy(str, data->oct.data, data->oct.length);
                        str[data->oct.length] = 0x00;
                        meterId = String(str);

                        // 96.3.10 Disconnect control
                        // 96.14.0 Currently acrive energy tariff
                        idx += 2;

                        // 1.8.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportCounter = ntohl(data->dlu.data) / 1000.0;

                        // 1.8.1
                        // 1.8.2
                        idx += 2;
                        
                        // 2.8.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportCounter = ntohl(data->dlu.data) / 1000.0;

                        // 2.8.1
                        // 2.8.2
                        idx += 2;

                        // 3.8.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveImportCounter = ntohl(data->dlu.data) / 1000.0;

                        // 4.8.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveExportCounter = ntohl(data->dlu.data) / 1000.0;

                        lastUpdateMillis = millis64();
                    } else if(no3->base.type == CosemTypeLongUnsigned) {
                        apply(state);
                        listType = state.getListType() > 2 ? state.getListType() : 2;

                        // 42.0.0 COSEM logical device name
                        idx++;

                        // 96.1.2 Device ID 3
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        memcpy(str, data->oct.data, data->oct.length);
                        str[data->oct.length] = 0x00;
                        meterId = String(str);

                        // 32.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1voltage = ntohs(data->lu.data) / 10.0;

                        // 52.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l2voltage = ntohs(data->lu.data) / 10.0;

                        // 72.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l3voltage = ntohs(data->lu.data) / 10.0;

                        // 31.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1current = ntohs(data->lu.data) / 100.0;

                        // 51.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l2current = ntohs(data->lu.data) / 100.0;

                        // 71.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l3current = ntohs(data->lu.data) / 100.0;

                        // 1.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportPower = ntohl(data->dlu.data);

                        // 2.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportPower = ntohl(data->dlu.data);

                        // 3.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveImportPower = ntohl(data->dlu.data);

                        // 4.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveExportPower = ntohl(data->dlu.data);

                        lastUpdateMillis = millis64();
                    }
                } else if(data->base.length ==  0x0A) {
                    CosemData* no7 = getCosemDataAt(7, ((char *) (d)));
                    if(no7->base.type == CosemTypeLongUnsigned) {
                        apply(state);
                        listType = state.getListType() > 4 ? state.getListType() : 4;

                        // 42.0.0 COSEM logical device name
                        idx++;
                        
                        // 96.1.2 Device ID 3
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        memcpy(str, data->oct.data, data->oct.length);
                        str[data->oct.length] = 0x00;
                        meterId = String(str);

                        // 1.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportPower = ntohl(data->dlu.data);

                        // 2.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportPower = ntohl(data->dlu.data);

                        // 3.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveImportPower = ntohl(data->dlu.data);

                        // 4.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveExportPower = ntohl(data->dlu.data);

                        // 32.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1voltage = ntohs(data->lu.data) / 10.0;

                        // 31.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1current = ntohs(data->lu.data) / 100.0;

                        // 21.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1activeImportPower = ntohl(data->dlu.data);

                        // 22.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1activeExportPower = ntohl(data->dlu.data);

                        lastUpdateMillis = millis64();
                    } else if(no7->base.type == CosemTypeDLongUnsigned) {
                        apply(state);
                        listType = state.getListType() > 3 ? state.getListType() : 3;

                        // 42.0.0 COSEM logical device name
                        idx++;
                        
                        // 96.1.3 Device ID 4
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        memcpy(str, data->oct.data, data->oct.length);
                        str[data->oct.length] = 0x00;
                        meterId = String(str);

                        // 1.8.1
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis181 = ntohl(data->dlu.data) / 1000.0;

                        // 1.8.2
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis182 = ntohl(data->dlu.data) / 1000.0;

                        if(activeImportCounter < obis181 + obis182)
                            activeImportCounter = obis181 + obis182;

                        // 2.8.1
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis281 = ntohl(data->dlu.data) / 1000.0;

                        // 2.8.2
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis282 = ntohl(data->dlu.data) / 1000.0;

                        if(activeExportCounter < obis281 + obis282)
                            activeExportCounter = obis281 + obis282;

                        // 3.8.1
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis381 = ntohl(data->dlu.data) / 1000.0;

                        // 3.8.2
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis382 = ntohl(data->dlu.data) / 1000.0;

                        if(reactiveImportCounter < obis381 + obis382)
                            reactiveImportCounter = obis381 + obis382;

                        // 4.8.1
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis481 = ntohl(data->dlu.data) / 1000.0;

                        // 4.8.2
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis482 = ntohl(data->dlu.data) / 1000.0;

                        if(reactiveExportCounter < obis481 + obis482)
                            reactiveExportCounter = obis481 + obis482;

                        lastUpdateMillis = millis64();
                    }
                } else if(data->base.length == 0x09) {
                    CosemData* no7 = getCosemDataAt(7, ((char *) (d)));
                    if(no7->base.type == CosemTypeLongUnsigned) {
                        apply(state);
                        listType = state.getListType() > 3 ? state.getListType() : 3;

                        // 42.0.0 COSEM logical device name
                        idx++;
                        
                        // 96.1.2 Device ID 3
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        memcpy(str, data->oct.data, data->oct.length);
                        str[data->oct.length] = 0x00;
                        meterId = String(str);

                        // 1.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportPower = ntohl(data->dlu.data);

                        // 2.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportPower = ntohl(data->dlu.data);

                        // 3.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveImportPower = ntohl(data->dlu.data);

                        // 4.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        reactiveExportPower = ntohl(data->dlu.data);

                        // 32.7.0
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        l1voltage = ntohs(data->lu.data) / 10.0;

                        // 2.8.1
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis281 = ntohl(data->dlu.data) / 1000.0;

                        // 2.8.2
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        double obis282 = ntohl(data->dlu.data) / 1000.0;

                        activeExportCounter = obis281 + obis282;

                        lastUpdateMillis = millis64();
                    } else if(no7->base.type == CosemTypeDLongUnsigned) {
                        apply(state);
                        listType = state.getListType() > 3 ? state.getListType() : 3;

                        // 42.0.0 COSEM logical device name?
                        idx++;

                        // 1.7.0?
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportPower = ntohl(data->dlu.data);

                        // 2.7.0?
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportPower = ntohl(data->dlu.data);

                        // 1.8.0?
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeImportCounter = ntohl(data->dlu.data) / 1000.0;

                        // 1.8.1?
                        // 1.8.2?
                        idx += 2;
                        
                        // 2.8.0?
                        data = getCosemDataAt(idx++, ((char *) (d)));
                        activeExportCounter = ntohl(data->dlu.data) / 1000.0;

                        // 2.8.1?
                        // 2.8.2?
                        idx += 2;

                        lastUpdateMillis = millis64();
                    }
                } else if(data->base.length == 0x08) {
                    // 42.0.0 COSEM logical device name
                    idx++;

                    // 96.1.2 Device ID 3
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    memcpy(str, data->oct.data, data->oct.length);
                    str[data->oct.length] = 0x00;
                    meterId = String(str);

                    // 32.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1voltage = ntohs(data->lu.data) / 10.0;

                    // 31.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1current = ntohs(data->lu.data) / 100.0;

                    // 1.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportPower = ntohl(data->dlu.data);

                    // 2.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportPower = ntohl(data->dlu.data);

                    // 3.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveImportPower = ntohl(data->dlu.data);

                    // 4.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    reactiveExportPower = ntohl(data->dlu.data);

                    lastUpdateMillis = millis64();
                } else if(data->base.length == 0x04) {
                    // ?
                    idx++;

                    // ?
                    idx++;

                    // 1.8.0?
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportCounter = ntohl(data->dlu.data) / 1000.0;

                    // 2.8.0?
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportCounter = ntohl(data->dlu.data) / 1000.0;
                }
            } else if(useMeterType == AmsTypeIskra && data->base.type == CosemTypeOctetString) { // Iskra special case
                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                debugger->printf_P(PSTR("Iskra selected\n"));

                meterType = AmsTypeIskra;

                uint8_t idx = 0;
                data = getCosemDataAt(idx, ((char *) (d)));
                if(data->base.length == 0x21) {
                    idx = 4;

                    // 1.8.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportCounter = ntohl(data->dlu.data) / 1000.0;

                    // 1.8.1
                    // 1.8.2
                    idx += 2;
                    
                    // 2.8.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportCounter = ntohl(data->dlu.data) / 1000.0;

                    // 2.8.1
                    // 2.8.2
                    idx += 2;

                    // 5.8.0
                    // 6.8.0
                    // 7.8.0
                    // 8.8.0
                    idx += 4;

                    // 1.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeImportPower = ntohl(data->dlu.data);

                    // 2.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    activeExportPower = ntohl(data->dlu.data);

                    // 13.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    powerFactor= ntohl(data->dlu.data) / 1000.0;

                    // 21.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1activeImportPower = ntohl(data->dlu.data);

                    // 41.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2activeImportPower = ntohl(data->dlu.data);

                    // 61.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3activeImportPower = ntohl(data->dlu.data);

                    // 22.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1activeExportPower = ntohl(data->dlu.data);

                    // 42.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2activeExportPower = ntohl(data->dlu.data);

                    // 62.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3activeExportPower = ntohl(data->dlu.data);

                    // 32.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1voltage = ntohs(data->lu.data) / 10.0;

                    // 52.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2voltage = ntohs(data->lu.data) / 10.0;

                    // 72.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3voltage = ntohs(data->lu.data) / 10.0;

                    // 31.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l1current = ntohs(data->lu.data) / 100.0;

                    // 51.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l2current = ntohs(data->lu.data) / 100.0;

                    // 71.7.0
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    l3current = ntohs(data->lu.data) / 100.0;

                    listType = 4;
                    lastUpdateMillis = millis64();
                } else {
                    idx = 5;

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        activeImportCounter = ntohl(data->dlu.data) / 1000.0;
                    }
        
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        activeExportCounter = ntohl(data->dlu.data) / 1000.0;
                    }
        
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        reactiveImportCounter = ntohl(data->dlu.data) / 1000.0;
                    }
        
                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        reactiveExportCounter = ntohl(data->dlu.data) / 1000.0;
                    }

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        activeImportPower = ntohl(data->dlu.data);
                    }

                    data = getCosemDataAt(idx++, ((char *) (d)));
                    if(data != NULL) {
                        activeExportPower = ntohl(data->dlu.data);
                    }

                    uint8_t str_len = 0;
                    str_len = getString(AMS_OBIS_UNKNOWN_1, sizeof(AMS_OBIS_UNKNOWN_1), ((char *) (d)), str);
                    if(str_len > 0) {
                        meterId = String(str);
                    }

                    listType = 4;
                    lastUpdateMillis = millis64();
                }
            } else if(useMeterType == AmsTypeUnknown) {
                uint8_t idx = 1;
                CosemData* d1 = getCosemDataAt(idx++, ((char *) (d)));
                CosemData* d2 = getCosemDataAt(idx++, ((char *) (d)));
                CosemData* d3 = getCosemDataAt(idx++, ((char *) (d)));

                #if defined(AMS_REMOTE_DEBUG)
                if (debugger->isActive(RemoteDebug::DEBUG))
                #endif
                {
                    debugger->printf("AMS Meter unknown type - checking for Iskra signature\n");
                    debugger->printf("  D1 type: %d\n", d1->base.type);
                    debugger->printf("  D2 type: %d\n", d2->base.type);
                    debugger->printf("  D3 type: %d\n", d3->base.type);
                }
                if(d1->base.type == d2->base.type == d3->base.type == CosemTypeOctetString) {
                    meterType = AmsTypeIskra;
                    lastUpdateMillis = millis64();
                    listType = 3;
                } else {
                    uint8_t str_len = 0;
                    str_len = getString(AMS_OBIS_UNKNOWN_1, sizeof(AMS_OBIS_UNKNOWN_1), ((char *) (d)), str);
                    if(str_len > 0) {
                        meterType = AmsTypeIskra;
                        meterId = String(str);
                        lastUpdateMillis = millis64();
                        listType = 3;
                    }
                }
            }
        }
    } else {
        listType = 1;
        activeImportPower = val;

        meterType = AmsTypeUnknown;
        CosemData* version = findObis(AMS_OBIS_VERSION, sizeof(AMS_OBIS_VERSION), d);
        if(version != NULL && (version->base.type == CosemTypeString || version->base.type == CosemTypeOctetString)) {
            if(memcmp(version->str.data, "AIDON", 5) == 0) {
                meterType = AmsTypeAidon;
            } else if(memcmp(version->str.data, "Kamstrup", 8) == 0) {
                meterType = AmsTypeKamstrup;
            } else if(memcmp(version->str.data, "KFM", 3) == 0) {
                meterType = AmsTypeKaifa;
            }
        } else {
            version = getCosemDataAt(1, ((char *) (d)));
            if(version->base.type == CosemTypeString) {
                if(memcmp(version->str.data, "Kamstrup", 8) == 0) {
                    meterType = AmsTypeKamstrup;
                }
            } 
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
        } else if(listType == 2) {
            l2currentMissing = true;
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
            this->meterTimestamp = adjustForKnownIssues(amst->dt, tz, meterType == AmsTypeUnknown ? useMeterType : meterType);
        }

        val = getNumber(AMS_OBIS_POWER_FACTOR, sizeof(AMS_OBIS_POWER_FACTOR), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 4;
            powerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L1, sizeof(AMS_OBIS_POWER_FACTOR_L1), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 4;
            l1PowerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L2, sizeof(AMS_OBIS_POWER_FACTOR_L2), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 4;
            l2PowerFactor = val;
        }
        val = getNumber(AMS_OBIS_POWER_FACTOR_L3, sizeof(AMS_OBIS_POWER_FACTOR_L3), ((char *) (d)));
        if(val != NOVALUE) {
            listType = 4;
            l3PowerFactor = val;
        }

        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L1, sizeof(AMS_OBIS_ACTIVE_IMPORT_L1), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l1activeImportPower = val;
        }
        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L2, sizeof(AMS_OBIS_ACTIVE_IMPORT_L2), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l2activeImportPower = val;
        }
        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L3, sizeof(AMS_OBIS_ACTIVE_IMPORT_L3), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l3activeImportPower = val;
        }

        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L1, sizeof(AMS_OBIS_ACTIVE_EXPORT_L1), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l1activeExportPower = val;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L2, sizeof(AMS_OBIS_ACTIVE_EXPORT_L2), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l2activeExportPower = val;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L3, sizeof(AMS_OBIS_ACTIVE_EXPORT_L3), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l3activeExportPower = val;
        }
        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L1_COUNT, sizeof(AMS_OBIS_ACTIVE_IMPORT_L1_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l1activeImportCounter = val/1000;
        }
        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L2_COUNT, sizeof(AMS_OBIS_ACTIVE_IMPORT_L2_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l2activeImportCounter = val/1000;
        }
        val = getNumber(AMS_OBIS_ACTIVE_IMPORT_L3_COUNT, sizeof(AMS_OBIS_ACTIVE_IMPORT_L3_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l3activeImportCounter = val/1000;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L1_COUNT, sizeof(AMS_OBIS_ACTIVE_EXPORT_L1_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l1activeExportCounter = val/1000;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L2_COUNT, sizeof(AMS_OBIS_ACTIVE_EXPORT_L2_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l2activeExportCounter = val/1000;
        }
        val = getNumber(AMS_OBIS_ACTIVE_EXPORT_L2_COUNT, sizeof(AMS_OBIS_ACTIVE_EXPORT_L2_COUNT), ((char *) (d)));
        if (val != NOVALUE) {
            listType = 4;
            l3activeExportCounter = val/1000;
        }



        if(meterType == AmsTypeKamstrup) {
            if(listType >= 3) {
                activeImportCounter *= 10;
                activeExportCounter *= 10;
                reactiveImportCounter *= 10;
                reactiveExportCounter *= 10;
                l1activeImportCounter *= 10;
                l2activeImportCounter *= 10;
                l3activeImportCounter *= 10;
                l1activeExportCounter *= 10;
                l2activeExportCounter *= 10;
                l3activeExportCounter *= 10;
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
                time_t ts = decodeCosemDateTime(amst->dt);
                meterTimestamp = ts;
            }

            CosemData* mid = getCosemDataAt(58, ((char *) (d))); // TODO: Get last item
            if(mid != NULL) {
                switch(mid->base.type) {
                    case CosemTypeString:
                        memcpy(str, mid->oct.data, mid->oct.length);
                        str[mid->oct.length] = 0x00;
                        meterId = String(str);
                        break;
                    case CosemTypeOctetString:
                        memcpy(str, mid->str.data, mid->str.length);
                        str[mid->str.length] = 0x00;
                        meterId = String(str);
                        break;
                }
            }
        }

        lastUpdateMillis = millis64();
    }

    // Try system title
    if(meterType == AmsTypeUnknown) {
        if(memcmp(ctx.system_title, "SAGY", 4) == 0) {
            meterType = AmsTypeSagemcom;
        } else if(memcmp(ctx.system_title, "KFM", 3) == 0) {
            meterType = AmsTypeKaifa;
        } else if(memcmp(ctx.system_title, "ISK", 3) == 0) {
            meterType = AmsTypeIskra;
        }

        if(meterId.isEmpty() && meterType != AmsTypeUnknown) {
        	stripNonAscii((uint8_t*) ctx.system_title, 8);
            meterId = String((const char*)ctx.system_title);
        }
    }

    if(meterConfig->wattageMultiplier > 0) {
        activeImportPower = activeImportPower > 0 ? activeImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        activeExportPower = activeExportPower > 0 ? activeExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        reactiveImportPower = reactiveImportPower > 0 ? reactiveImportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
        reactiveExportPower = reactiveExportPower > 0 ? reactiveExportPower * (meterConfig->wattageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->voltageMultiplier > 0) {
        l1voltage = l1voltage > 0 ? l1voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
        l2voltage = l2voltage > 0 ? l2voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
        l3voltage = l3voltage > 0 ? l3voltage * (meterConfig->voltageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->amperageMultiplier > 0) {
        l1current = l1current > 0 ? l1current * (meterConfig->amperageMultiplier / 1000.0) : 0;
        l2current = l2current > 0 ? l2current * (meterConfig->amperageMultiplier / 1000.0) : 0;
        l3current = l3current > 0 ? l3current * (meterConfig->amperageMultiplier / 1000.0) : 0;
    }
    if(meterConfig->accumulatedMultiplier > 0) {
        activeImportCounter = activeImportCounter > 0 ? activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        activeExportCounter = activeExportCounter > 0 ? activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        reactiveImportCounter = reactiveImportCounter > 0 ? reactiveImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        reactiveExportCounter = reactiveExportCounter > 0 ? reactiveExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l1activeImportCounter = l1activeImportCounter > 0 ? l1activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l2activeImportCounter = l2activeImportCounter > 0 ? l2activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l3activeImportCounter = l3activeImportCounter > 0 ? l3activeImportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l1activeExportCounter = l1activeExportCounter > 0 ? l1activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l2activeExportCounter = l2activeExportCounter > 0 ? l2activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
        l3activeExportCounter = l3activeExportCounter > 0 ? l3activeExportCounter * (meterConfig->accumulatedMultiplier / 1000.0) : 0;
    }

    threePhase = l1voltage > 0 && l2voltage > 0 && l3voltage > 0;
    if(!threePhase)
        twoPhase = (l1voltage > 0 && l2voltage > 0) || (l2voltage > 0 && l3voltage > 0) || (l3voltage > 0  && l1voltage > 0);

    // Special case for Norwegian IT/TT meters that does not report all values
    if(meterConfig->distributionSystem == 1) {
        if(twoPhase && l1current > 0.0 && l2current > 0.0 && l3current > 0.0) {
            l2voltage = sqrt(pow(l1voltage - l3voltage * cos(60.0 * (PI/180.0)), 2) + pow(l3voltage * sin(60.0 * (PI/180.0)),2));
            threePhase = true;
        }
    }
    meterId.trim();
}

CosemData* IEC6205675::getCosemDataAt(uint8_t index, const char* ptr) {
    CosemData* item = (CosemData*) ptr;
    int i = 0;
    char* pos = (char*) ptr;
    while(pos-ptr < 900) {
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
            case CosemTypeLongUnsigned:
                pos += 3;
                break;
            case CosemTypeDLongSigned:
            case CosemTypeDLongUnsigned:
                pos += 5;
                break;
            case CosemTypeLong64Signed:
            case CosemTypeLong64Unsigned:
                pos += 9;
                break;
            case CosemTypeNull:
                pos += 1;
                break;
            default:
                pos += 2;
        }
        i++;
    }
    return NULL;
}

CosemData* IEC6205675::findObis(uint8_t* obis, int matchlength, const char* ptr) {
    CosemData* item = (CosemData*) ptr;
    int ret = 0;
    char* pos = (char*) ptr;
    while(pos-ptr < 900) {
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
            case CosemTypeLongUnsigned:
                pos += 3;
                break;
            case CosemTypeDLongSigned:
            case CosemTypeDLongUnsigned:
                pos += 5;
                break;
            case CosemTypeLong64Signed:
            case CosemTypeLong64Unsigned:
                pos += 9;
                break;
            case CosemTypeNull:
                pos += 1;
                break;
            default:
                pos += 2;
        }
    }
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

float IEC6205675::getNumber(uint8_t* obis, int matchlength, const char* ptr) {
    CosemData* item = findObis(obis, matchlength, ptr);
    return getNumber(item);
}

float IEC6205675::getNumber(CosemData* item) {
    if(item != NULL) {
        float ret = 0.0;
        char* pos = ((char*) item);
        switch(item->base.type) {
            case CosemTypeLongSigned: {
                int16_t i16 = ntohs(item->ls.data);
                ret = (i16 * 1.0);
                pos += 3;
                break;
            }
            case CosemTypeLongUnsigned: {
                uint16_t u16 = ntohs(item->lu.data);
                ret = (u16 * 1.0);
                pos += 3;
                break;
            }
            case CosemTypeDLongSigned: {
                int32_t i32 = ntohl(item->dlu.data);
                ret = (i32 * 1.0);
                pos += 5;
                break;
            }
            case CosemTypeDLongUnsigned: {
                uint32_t u32 = ntohl(item->dlu.data);
                ret = (u32 * 1.0);
                pos += 5;
                break;
            }
            case CosemTypeLong64Signed: {
                int64_t i64 = ntohll(item->l64s.data);
                ret = (i64 * 1.0);
                pos += 9;
                break;
            }
            case CosemTypeLong64Unsigned: {
                uint64_t u64 = ntohll(item->l64u.data);
                ret = (u64 * 1.0);
                pos += 9;
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
                    return decodeCosemDateTime(ts->dt);
                }
            }
        }
    }
    return 0;
}

time_t IEC6205675::adjustForKnownIssues(CosemDateTime dt, Timezone* tz, uint8_t meterType) {    
    time_t ts = decodeCosemDateTime(dt);
    int16_t deviation = ntohs(dt.deviation);
    if(deviation < -720 || deviation > 720) {
        // Time zone not specified
        if(meterType == AmsTypeAidon || meterType == AmsTypeKamstrup) {
            // Special known case
            // 21.09.24, the clock is now correct for Aidon
            // 23.10.25, the clock is now correct for Kamstrup
            ts -= 3600;
        } else if(tz != NULL) {
            // Adjust from localtime to UTC
            ts = tz->toUTC(ts);
        }
    } else if(meterType == AmsTypeAidon) {
        // 21.09.24, the clock is now correct for Aidon
        ts -= 3600;
    }
    return ts;  
}