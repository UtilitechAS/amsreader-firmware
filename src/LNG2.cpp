#include "LNG2.h"
#include "Uptime.h"

LNG2::LNG2(const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx, RemoteDebug* debugger) {
    CosemBasic* h = (CosemBasic*) payload;
    if(h->length == 0x0e) {
        meterType = AmsTypeLandisGyr;
        this->packageTimestamp = ctx.timestamp;

        Lng2Data_3p* d = (Lng2Data_3p*) payload;
        this->l1voltage = ntohs(d->u1.data);
        this->l2voltage = ntohs(d->u2.data);
        this->l3voltage = ntohs(d->u3.data);

        this->l1current = ntohs(d->i1.data) / 100.0;
        this->l2current = ntohs(d->i2.data) / 100.0;
        this->l3current = ntohs(d->i3.data) / 100.0;

        this->activeImportPower = ntohl(d->activeImport.data);
        this->activeExportPower = ntohl(d->activeExport.data);
        this->activeImportCounter = ntohl(d->acumulatedImport.data) / 1000.0;
        this->activeExportCounter = ntohl(d->accumulatedExport.data) / 1000.0;

        char str[64];
        uint8_t str_len = getString((CosemData*) &d->meterId, str);
        if(str_len > 0) {
            this->meterId = String(str);
        }
        listType = 3;
        lastUpdateMillis = millis64();
    }
}

uint8_t LNG2::getString(CosemData* item, char* target) {
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
    return 0;

}
