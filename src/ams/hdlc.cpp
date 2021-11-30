#include "Arduino.h"
#include "hdlc.h"
#include "crc.h"
#include "lwip/def.h"
#if defined(ESP8266)
#include "bearssl/bearssl.h"
#elif defined(ESP32)
#include "mbedtls/gcm.h"
#endif

void mbus_hexdump(const uint8_t* buf, int len) {
    printf("\nDUMP (%db) [ ", len); 
    for(const uint8_t* p = buf; p-buf < len; ++p)
        printf("%02X ", *p);
    printf("]\n");
}

int HDLC_validate(const uint8_t* d, int length, HDLCConfig* config, CosemDateTime* timestamp) {
    //mbus_hexdump(d, len);

	HDLCHeader* h = (HDLCHeader*) d;

	// Length field (11 lsb of format)
    int len = (ntohs(h->format) & 0x7FF) + 2;
    if(len > length)
        return HDLC_FRAME_INCOMPLETE;

	HDLCFooter* f = (HDLCFooter*) (d + len - sizeof *f);

	// First and last byte should be MBUS_HAN_TAG
	if(h->flag != HDLC_FLAG || f->flag != HDLC_FLAG)
		return HDLC_BOUNDRY_FLAG_MISSING;

	// Verify FCS
	if(ntohs(f->fcs) != crc16_x25(d + 1, len - sizeof *f - 1))
		return HDLC_FCS_ERROR;

    int headersize = 8;
    int footersize = 3;
    uint8_t* ptr = (uint8_t*) &h[1];
    // Frame format type 3
    if((h->format & 0xF0) == 0xA0) {

        // Skip destination address, LSB marks last byte
        while(((*ptr) & 0x01) == 0x00) {
            ptr++;
            headersize++;
        }
        ptr++;

        // Skip source address, LSB marks last byte
        while(((*ptr) & 0x01) == 0x00) {
            ptr++;
            headersize++;
        }
        ptr++;

        HDLC3CtrlHcs* t3 = (HDLC3CtrlHcs*) (ptr);
        headersize += 3;

        // Verify HCS
        if(ntohs(t3->hcs) != crc16_x25(d + 1, ptr-d))
            return HDLC_HCS_ERROR;

        ptr += sizeof *t3;
    }

    // Extract LLC
    HDLCLLC* llc = (HDLCLLC*) ptr;
    ptr += sizeof *llc;

    if(((*ptr) & 0xFF) == 0x0F) {
        // Unencrypted APDU
        int i = 0;
        HDLCADPU* adpu = (HDLCADPU*) (ptr);
        ptr += sizeof *adpu;

        // ADPU timestamp
        CosemData* dateTime = (CosemData*) ptr;
        if(dateTime->base.type == CosemTypeOctetString) {
            if(dateTime->base.length == 0x0C) {
                memcpy(timestamp, ptr+1, dateTime->base.length);
            }
            ptr += 2 + dateTime->base.length;
        } else if(dateTime->base.type == CosemTypeNull) {
            timestamp = 0;
            ptr++;
        } else if(dateTime->base.type == CosemTypeDateTime) {
            memcpy(timestamp, ptr, dateTime->base.length);
        } else if(dateTime->base.type == 0x0C) { // Kamstrup bug...
            memcpy(timestamp, ptr, 0x0C);
            ptr += 13;
        } else {
            return -99;
        }

        return ptr-d;
    } else if(((*ptr) & 0xFF) == 0xDB) {
        // Encrypted APDU
        // http://www.weigu.lu/tutorials/sensors2bus/04_encryption/index.html
        if(config == NULL)
            return HDLC_ENCRYPTION_CONFIG_MISSING;

        memcpy(config->system_title, d + headersize + 2, 8);
        memcpy(config->initialization_vector, config->system_title, 8);
        memcpy(config->initialization_vector + 8, d + headersize + 14, 4);
        memcpy(config->additional_authenticated_data, d + headersize + 13, 1);
        memcpy(config->additional_authenticated_data + 1, config->authentication_key, 16);
        memcpy(config->authentication_tag, d + headersize + len - headersize - footersize - 12, 12);

        #if defined(ESP8266)
            br_gcm_context gcmCtx;
            br_aes_ct_ctr_keys bc;
            br_aes_ct_ctr_init(&bc, config->encryption_key, 16);
            br_gcm_init(&gcmCtx, &bc.vtable, br_ghash_ctmul32);
            br_gcm_reset(&gcmCtx, config->initialization_vector, sizeof(config->initialization_vector));
            br_gcm_aad_inject(&gcmCtx, config->additional_authenticated_data, sizeof(config->additional_authenticated_data));
            br_gcm_flip(&gcmCtx);
            br_gcm_run(&gcmCtx, 0, (void*) (d + headersize + 18), (len - headersize - footersize - 18 - 12));
            if(br_gcm_check_tag_trunc(&gcmCtx, config->authentication_tag, 12) != 1) {
                return -91;
            }
        #elif defined(ESP32)
            uint8_t cipher_text[len - headersize - footersize - 18 - 12];
            memcpy(cipher_text, d + headersize + 18, len - headersize - footersize - 12 - 18);

            mbedtls_gcm_context m_ctx;
            mbedtls_gcm_init(&m_ctx);
            int success = mbedtls_gcm_setkey(&m_ctx, MBEDTLS_CIPHER_ID_AES, config->encryption_key, 128);
            if (0 != success ) {
                return -92;
            }
            success = mbedtls_gcm_auth_decrypt(&m_ctx, sizeof(cipher_text), config->initialization_vector, sizeof(config->initialization_vector),
                config->additional_authenticated_data, sizeof(config->additional_authenticated_data), config->authentication_tag, sizeof(config->authentication_tag),
                cipher_text, (unsigned char*)(d + headersize + 18));
            if (0 != success) {
                return -91;
            }
            mbedtls_gcm_free(&m_ctx);
        #endif

        ptr += 23; // TODO: Come to this number in a proper way...

        // ADPU timestamp
        CosemData* dateTime = (CosemData*) ptr;
        if(dateTime->base.type == CosemTypeOctetString) {
            if(dateTime->base.length == 0x0C) {
                memcpy(timestamp, ptr+1, dateTime->base.length);
            }
            ptr += 2 + dateTime->base.length;
        } else if(dateTime->base.type == CosemTypeNull) {
            timestamp = 0;
            ptr++;
        } else if(dateTime->base.type == CosemTypeDateTime) {
            memcpy(timestamp, ptr, dateTime->base.length);
        } else if(dateTime->base.type == 0x0C) { // Kamstrup bug...
            memcpy(timestamp, ptr, 0x0C);
            ptr += 13;
        } else {
            return -99;
        }

        return ptr-d;
    }    

    // Unknown payload
	return 0;
}