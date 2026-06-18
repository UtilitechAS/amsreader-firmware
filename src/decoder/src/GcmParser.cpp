/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 * 
 */

#include "GcmParser.h"
#include "byteorder.h"
#if defined(ESP8266)
#include "bearssl/bearssl.h"
#elif defined(ESP32)
#include "mbedtls/gcm.h"
#elif defined(NATIVE_TEST) && defined(HAVE_MBEDTLS)
#include "mbedtls/gcm.h"   // host (native tests) — system mbedTLS (2.x or 3.x)
#if defined(__has_include) && __has_include("mbedtls/build_info.h")
#include "mbedtls/build_info.h"   // 3.x defines MBEDTLS_VERSION_MAJOR here
#elif defined(__has_include) && __has_include("mbedtls/version.h")
#include "mbedtls/version.h"      // 2.x defines it here
#endif
#endif
#include <string.h>

GCMParser::GCMParser(uint8_t *encryption_key, uint8_t *authentication_key) {
    memcpy(this->encryption_key, encryption_key, 16);
    memcpy(this->authentication_key, authentication_key, 16);
}

int8_t GCMParser::parse(uint8_t *d, DataParserContext &ctx, bool hastag) {
    if(ctx.length < 12) return DATA_PARSE_INCOMPLETE;

    uint32_t headersize = 0;
    uint8_t* ptr = (uint8_t*) d;
    if(hastag) {
        if(*ptr != GCM_TAG) return DATA_PARSE_BOUNDARY_FLAG_MISSING;
        ptr++;
        headersize++;
    }
    // Encrypted APDU
    // http://www.weigu.lu/tutorials/sensors2bus/04_encryption/index.html

    uint8_t systemTitleLength = *ptr;
    ptr++;
    headersize++;

    uint8_t initialization_vector[12];
    memset(ctx.system_title, 0, 8);
    memset(initialization_vector, 0, 12);
    if(systemTitleLength > 0) {
        memcpy(ctx.system_title, ptr, systemTitleLength);
        memcpy(initialization_vector, ctx.system_title, systemTitleLength);
        ptr += systemTitleLength;
        headersize += systemTitleLength;
    }

    uint32_t len = 0;
    if(((*ptr) & 0xFF) == 0x81) {
        // 1-byte payload length
        ptr++;
        len = *ptr++;
        headersize += 2;
    } else if(((*ptr) & 0xFF) == 0x82) {
        // 2-byte payload length
        ptr++;
        len = *ptr++ << 8;
        len |= *ptr++;
        headersize += 3;
    } else if(((*ptr) & 0xFF) == 0x84) {
        // 4-byte payload length
        ptr++;
        len = *ptr++ << 24;
        len |= *ptr++ << 16;
        len |= *ptr++ << 8;
        len |= *ptr++;
        headersize += 5;
    } else {
        len = *ptr++;
        headersize++;
    }
    if(len + headersize > ctx.length)
        return DATA_PARSE_INCOMPLETE;

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
    bool authenticate = false;
    uint8_t authentication_tag[12];
    uint8_t authkeylen = 0, aadlen = 0;
    if((sec & 0x10) == 0x10) {
        // Need at least the 12-byte auth tag plus the 5 security/frame-counter
        // bytes; otherwise the tag memcpy and ciphertext length underflow.
        if(len < 17) return GCM_DECRYPT_FAILED;
        authkeylen = 12;
        aadlen = 17;
        footersize += authkeylen;
        memcpy(additional_authenticated_data + 1, authentication_key, 16);
        memcpy(authentication_tag, ptr + len - footersize - 5, authkeylen);
        for(uint8_t i = 0; i < 16; i++) authenticate |= authentication_key[i] > 0;
    }

    // Guard the ciphertext length (len - authkeylen - 5) against underflow so a
    // short/garbage length can't blow the stack via the cipher_text buffer.
    if(len < (uint32_t)authkeylen + 5) return GCM_DECRYPT_FAILED;

    #if defined(ESP8266)
        br_gcm_context gcmCtx;
        br_aes_ct_ctr_keys bc;
        br_aes_ct_ctr_init(&bc, encryption_key, 16);
        br_gcm_init(&gcmCtx, &bc.vtable, br_ghash_ctmul32);
        br_gcm_reset(&gcmCtx, initialization_vector, sizeof(initialization_vector));
        if(authenticate) {
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
        if (authenticate) {
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
    #elif defined(NATIVE_TEST) && defined(HAVE_MBEDTLS)
        // Native host tests with system mbedTLS (3.x API).
        uint8_t cipher_text[len - authkeylen - 5];
        memcpy(cipher_text, ptr, len - authkeylen - 5);

        mbedtls_gcm_context m_ctx;
        mbedtls_gcm_init(&m_ctx);
        if (mbedtls_gcm_setkey(&m_ctx, MBEDTLS_CIPHER_ID_AES, encryption_key, 128) != 0) {
            mbedtls_gcm_free(&m_ctx);
            return GCM_ENCRYPTION_KEY_FAILED;
        }
        if (authenticate) {
            int rc = mbedtls_gcm_auth_decrypt(&m_ctx, sizeof(cipher_text),
                initialization_vector, sizeof(initialization_vector),
                additional_authenticated_data, aadlen,
                authentication_tag, authkeylen,
                cipher_text, (unsigned char*)(ptr));
            if (authkeylen > 0 && rc == MBEDTLS_ERR_GCM_AUTH_FAILED) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_AUTH_FAILED;
            } else if (rc != 0) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
        } else {
        #if defined(MBEDTLS_VERSION_MAJOR) && MBEDTLS_VERSION_MAJOR >= 3
            size_t olen = 0;
            if (mbedtls_gcm_starts(&m_ctx, MBEDTLS_GCM_DECRYPT,
                    initialization_vector, sizeof(initialization_vector)) != 0 ||
                mbedtls_gcm_update(&m_ctx, cipher_text, sizeof(cipher_text),
                    (unsigned char*)(ptr), sizeof(cipher_text), &olen) != 0) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
        #else   // mbedTLS 2.x (e.g. Ubuntu libmbedtls-dev) — older API
            if (mbedtls_gcm_starts(&m_ctx, MBEDTLS_GCM_DECRYPT,
                    initialization_vector, sizeof(initialization_vector), NULL, 0) != 0 ||
                mbedtls_gcm_update(&m_ctx, sizeof(cipher_text),
                    cipher_text, (unsigned char*)(ptr)) != 0) {
                mbedtls_gcm_free(&m_ctx);
                return GCM_DECRYPT_FAILED;
            }
        #endif
        }
        mbedtls_gcm_free(&m_ctx);
    #else
        // Native / unsupported platform: decryption not available
        return GCM_DECRYPT_FAILED;
    #endif

    ctx.length -= footersize + headersize;
    return ptr-d;
}