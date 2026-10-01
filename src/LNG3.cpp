/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 */

#include "LNG3.h"
#include "byteorder.h"
#include "Uptime.h"

static const uint8_t LNG3_LU_SIZE = sizeof(CosemLongUnsigned);
static const uint8_t LNG3_DLU_SIZE = sizeof(CosemDLongUnsigned);

static const uint8_t LNG3_TOTALS_ITEMS = 0x09;
static const uint8_t LNG3_PHASES_ITEMS = 0x0E;

static bool runOfType(const char* ptr, uint8_t count, uint8_t type, uint8_t size) {
    for(uint8_t i = 0; i < count; i++) {
        if(((uint8_t) ptr[i * size]) != type) return false;
    }
    return true;
}

// Iskra sends a 9-item structure of the very same shape, led by its "ISK..."
// list identifier (test/payloads/iskraemeco/gh869-6.hex), but with different
// semantics. Every list identifier in this decoder contains letters, while the
// L&G id is the bare meter serial, so digits-only tells the two apart.
static bool isAllDigits(const char* ptr, uint8_t length) {
    if(length == 0) return false;
    for(uint8_t i = 0; i < length; i++) {
        if(ptr[i] < '0' || ptr[i] > '9') return false;
    }
    return true;
}

// Offset of the first value after the leading meter id, or 0 if the payload is
// not one of the two known shapes.
static uint16_t bodyOffset(const char* payload, uint16_t length) {
    if(length < 4) return 0;
    if(((uint8_t) payload[0]) != CosemTypeStructure) return 0;
    if(((uint8_t) payload[2]) != CosemTypeOctetString) return 0;

    uint16_t body = 4 + ((uint8_t) payload[3]);
    if(length < body) return 0;
    if(!isAllDigits(payload + 4, (uint8_t) payload[3])) return 0;
    switch((uint8_t) payload[1]) {
        case LNG3_TOTALS_ITEMS:
            if(length < body + (8 * LNG3_DLU_SIZE)) return 0;
            if(!runOfType(payload + body, 8, CosemTypeDLongUnsigned, LNG3_DLU_SIZE)) return 0;
            return body;
        case LNG3_PHASES_ITEMS:
            if(length < body + (7 * LNG3_LU_SIZE) + (6 * LNG3_DLU_SIZE)) return 0;
            if(!runOfType(payload + body, 7, CosemTypeLongUnsigned, LNG3_LU_SIZE)) return 0;
            if(!runOfType(payload + body + (7 * LNG3_LU_SIZE), 6, CosemTypeDLongUnsigned, LNG3_DLU_SIZE)) return 0;
            return body;
    }
    return 0;
}

bool LNG3::matches(const char* payload, uint16_t length) {
    return bodyOffset(payload, length) > 0;
}

LNG3::LNG3(AmsData& meterState, const char* payload, uint8_t useMeterType, MeterConfig* meterConfig, DataParserContext &ctx) {
    uint16_t body = bodyOffset(payload, ctx.length);
    if(body == 0) return;

    apply(meterState);
    meterType = AmsTypeLandisGyr;
    this->packageTimestamp = ctx.timestamp;

    if(((uint8_t) payload[1]) == LNG3_TOTALS_ITEMS) {
        // 96.1.0, the manufacturer serial. The other frame leads with 96.1.1,
        // a separate register the utility assigns, so only this one may set
        // meterId -- taking both would flip the id (and the MQTT topic and
        // Home Assistant entity) every couple of seconds wherever the two
        // registers differ. apply() above carries the id into the other frame.
        CosemString* id = (CosemString*) (payload + sizeof(CosemBasic));
        uint8_t idLength = id->length < sizeof(meterId) - 1 ? id->length : sizeof(meterId) - 1;
        memcpy(meterId, id->data, idLength);
        meterId[idLength] = '\0';

        Lng3Totals* d = (Lng3Totals*) (payload + body);

        activeImportPower = ntohl(d->activeImportPower.data);
        activeExportPower = ntohl(d->activeExportPower.data);
        reactiveImportPower = ntohl(d->reactiveImportPower.data);
        reactiveExportPower = ntohl(d->reactiveExportPower.data);

        activeImportCounter = ntohl(d->activeImportCounter.data) / 1000.0;
        activeExportCounter = ntohl(d->activeExportCounter.data) / 1000.0;
        reactiveImportCounter = ntohl(d->reactiveImportCounter.data) / 1000.0;
        reactiveExportCounter = ntohl(d->reactiveExportCounter.data) / 1000.0;

        listType = meterState.getListType() > 3 ? meterState.getListType() : 3;
    } else {
        Lng3Phases* d = (Lng3Phases*) (payload + body);

        l1current = ntohs(d->i1.data) / 100.0;
        l2current = ntohs(d->i2.data) / 100.0;
        l3current = ntohs(d->i3.data) / 100.0;

        // Whole volts, unlike every other list this decoder reads
        l1voltage = ntohs(d->u1.data);
        l2voltage = ntohs(d->u2.data);
        l3voltage = ntohs(d->u3.data);

        l1activeImportPower = ntohl(d->p1.data);
        l2activeImportPower = ntohl(d->p2.data);
        l3activeImportPower = ntohl(d->p3.data);

        l1activeExportPower = ntohl(d->p1Export.data);
        l2activeExportPower = ntohl(d->p2Export.data);
        l3activeExportPower = ntohl(d->p3Export.data);

        threePhase = true;

        listType = meterState.getListType() > 4 ? meterState.getListType() : 4;
    }

    lastUpdateMillis = millis64();
}
