#include "GcmParser.h"
#include "lwip/def.h"
#if defined(ESP8266)
#include "bearssl/bearssl.h"
#elif defined(ESP32)
#include "mbedtls/gcm.h"
#endif

GCMParser::GCMParser(uint8_t *encryption_key, uint8_t *authentication_key) {
    memcpy(this->encryption_key, encryption_key, 16);
    memcpy(this->authentication_key, authentication_key, 16);
}

int8_t GCMParser::parse(uint8_t *d, DataParserContext &ctx) {
    if(ctx.length < 12) return DATA_PARSE_INCOMPLETE;

    uint8_t* ptr = (uint8_t*) d;
    if(*ptr != GCM_TAG) return DATA_PARSE_BOUNDRY_FLAG_MISSING;
    ptr++;
    // Encrypted APDU
    // http://www.weigu.lu/tutorials/sensors2bus/04_encryption/index.html

    uint8_t systemTitleLength = *ptr;
    ptr++;

    uint8_t initialization_vector[12];
    memcpy(ctx.system_title, ptr, systemTitleLength);
    memcpy(initialization_vector, ctx.system_title, systemTitleLength);

    int len = 0;
    int headersize = 2 + systemTitleLength;
    ptr += systemTitleLength;
    if(((*ptr) & 0xFF) == 0x81) {
        ptr++;
        len = *ptr;
        // 1-byte payload length
        ptr++;
        headersize += 2;
    } else if(((*ptr) & 0xFF) == 0x82) {
        GCMSizeDef* h = (GCMSizeDef*) ptr;

        // 2-byte payload length
        len = (ntohs(h->format) & 0xFFFF);

        ptr += 3;
        headersize += 3;
    } else {
        len = *ptr;
        ptr++;
        headersize++;
    }
    if(len + headersize > ctx.length)
        return DATA_PARSE_INCOMPLETE;

    //Serial.printf("\nL: %d : %d, %d\n", length, len, headersize);

    uint8_t additional_authenticated_data[17];
    memcpy(additional_authenticated_data, ptr, 1);

    // Security tag
    uint8_t sec = *ptr;
    ptr++;
    headersize++;

    // Frame counter
    memcpy(initialization_vector + 8, ptr, 4);
    ptr += 4;
    headersize += 4;

    int footersize = 0;

    // Authentication enabled
    uint8_t authentication_tag[12];
    uint8_t authkeylen = 0, aadlen = 0;
    if((sec & 0x10) == 0x10) {
        authkeylen = 12;
        aadlen = 17;
        footersize += authkeylen;
        memcpy(additional_authenticated_data + 1, authentication_key, 16);
        memcpy(authentication_tag, ptr + len - footersize - 5, authkeylen);
    }

    #if defined(ESP8266)
        br_gcm_context gcmCtx;
        br_aes_ct_ctr_keys bc;
        br_aes_ct_ctr_init(&bc, encryption_key, 16);
        br_gcm_init(&gcmCtx, &bc.vtable, br_ghash_ctmul32);
        br_gcm_reset(&gcmCtx, initialization_vector, sizeof(initialization_vector));
        if(authkeylen > 0) {
            br_gcm_aad_inject(&gcmCtx, additional_authenticated_data, aadlen);
        }
        br_gcm_flip(&gcmCtx);
        br_gcm_run(&gcmCtx, 0, (void*) (ptr), len - authkeylen - 5); // 5 == security tag and frame counter
        if(authkeylen > 0 && br_gcm_check_tag_trunc(&gcmCtx, authentication_tag, authkeylen) != 1) {
            return GCM_AUTH_FAILED;
        }
    #elif defined(ESP32)
        uint8_t cipher_text[len - authkeylen - 5];
        memcpy(cipher_text, ptr, len - authkeylen - 5);

        mbedtls_gcm_context m_ctx;
        mbedtls_gcm_init(&m_ctx);
        int success = mbedtls_gcm_setkey(&m_ctx, MBEDTLS_CIPHER_ID_AES, encryption_key, 128);
        if (0 != success) {
            return GCM_ENCRYPTION_KEY_FAILED;
        }
        if (0 < authkeylen) {
            success = mbedtls_gcm_auth_decrypt(&m_ctx, sizeof(cipher_text), initialization_vector, sizeof(initialization_vector),
                additional_authenticated_data, aadlen, authentication_tag, authkeylen,
                cipher_text, (unsigned char*)(ptr));
            if (authkeylen > 0 && success == MBEDTLS_ERR_GCM_AUTH_FAILED) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_AUTH_FAILED;
            } else if(success == MBEDTLS_ERR_GCM_BAD_INPUT) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
        } else {
            success = mbedtls_gcm_starts(&m_ctx, MBEDTLS_GCM_DECRYPT, initialization_vector, sizeof(initialization_vector),NULL, 0);
            if (0 != success) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
            success = mbedtls_gcm_update(&m_ctx, sizeof(cipher_text), cipher_text, (unsigned char*)(ptr));
            if (0 != success) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
        }
        mbedtls_gcm_free(&m_ctx);
    #endif

    ctx.length -= footersize + headersize;
    return ptr-d;
}