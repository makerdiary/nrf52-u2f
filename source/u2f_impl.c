/**
* Copyright (c) 2018 makerdiary
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* * Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above
*   copyright notice, this list of conditions and the following
*   disclaimer in the documentation and/or other materials provided
*   with the distribution.

* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nrf.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "fds.h"

#include "nrf_crypto.h"
#include "nrf_crypto_ecc.h"
#include "nrf_crypto_ecdsa.h"
#include "nrf_crypto_hash.h"
#include "nrf_crypto_error.h"

#include "u2f.h"

#define NRF_LOG_MODULE_NAME u2f_impl

#include "nrf_log.h"

NRF_LOG_MODULE_REGISTER();

/* File ID and Key used for the configuration record. */
#define CONFIG_AES_KEY_FILE     (0xEF10)
#define CONFIG_AES_KEY_REC_KEY  (0x7F10)

/* File ID and Key used for the configuration record. */
#define CONFIG_COUNTER_FILE     (0xEF11)
#define CONFIG_COUNTER_REC_KEY  (0x7F11)

#define AES_KEY_SIZE             16


extern uint8_t aes_key[];
extern const uint8_t attestation_cert[];
extern const uint8_t attestation_private_key[];
extern uint16_t attestation_cert_size;
extern uint8_t attestation_private_key_size;

extern bool is_user_button_pressed(void);


/* authentication counter */
uint32_t m_auth_counter = 0;

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

/* The record descriptor of counter */
static fds_record_desc_t m_counter_record_desc;

/* A record containing m_auth_counter. */
static fds_record_t const m_counter_record =
{
    .file_id           = CONFIG_COUNTER_FILE,
    .key               = CONFIG_COUNTER_REC_KEY,
    .data.p_data       = &m_auth_counter,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = sizeof(m_auth_counter) / sizeof(uint32_t),
};


#ifdef CONFIG_RANDOM_AES_KEY_ENABLED
/* A record containing AES key. */
static fds_record_t const m_aes_key_record =
{
    .file_id           = CONFIG_AES_KEY_FILE,
    .key               = CONFIG_AES_KEY_REC_KEY,
    .data.p_data       = aes_key,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = AES_KEY_SIZE / sizeof(uint32_t),
};
#endif /* CONFIG_RANDOM_AES_KEY_ENABLED */


/**@brief Convert a signature to the correct format. For more info:
 * http://bitcoin.stackexchange.com/questions/12554/why-the-signature-is-always
 * -65-13232-bytes-long
 *
 * @param[in]  p_dest_sig  The output signature.
 * @param[in]  p_src_sig   The input signature.
 *
 * @retval     The output signature size.
 *
 */
static uint16_t signature_convert(uint8_t * p_dest_sig, uint8_t * p_src_sig);


static void fds_evt_handler(fds_evt_t const * p_evt)
{

    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_evt->result == FDS_SUCCESS)
            {
                m_fds_initialized = true;
            }
            break;

        case FDS_EVT_WRITE:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
            }
        } break;

        case FDS_EVT_DEL_RECORD:
        {
            if (p_evt->result == FDS_SUCCESS)
            {
                NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
                NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
                NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
            }
        } break;

        default:
            break;
    }
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized)
    {
        // Just waiting
    }
}


uint32_t u2f_impl_init(void)
{
    ret_code_t ret;
    fds_find_token_t  tok  = {0};

    ret = nrf_crypto_init();
    if(ret != NRF_SUCCESS) return ret;

    /* Register first to receive an event when initialization is complete. */
    (void) fds_register(fds_evt_handler);

    ret = fds_init();
    if(ret != NRF_SUCCESS) return ret;

    wait_for_fds_ready();

    /* update m_auth_counter */
    ret = fds_record_find(CONFIG_COUNTER_FILE, CONFIG_COUNTER_REC_KEY, 
                          &m_counter_record_desc, &tok);
    if(ret == NRF_SUCCESS)
    {
        /* A counter is in flash. Let's update it. */
        fds_flash_record_t config = {0};
        
        /* Open the record and read its contents. */
        ret = fds_record_open(&m_counter_record_desc, &config);
        if(ret != NRF_SUCCESS) return ret;

        /* Copy the counter value from flash into m_auth_counter. */
        memcpy(&m_auth_counter, config.p_data, sizeof(m_auth_counter));

        NRF_LOG_INFO("m_auth_counter = %d", m_auth_counter);

        /* Close the record when done reading. */
        ret = fds_record_close(&m_counter_record_desc);
        if(ret != NRF_SUCCESS) return ret;
    }
    else
    {
        /* m_auth_counter not found; write a new one. */
        NRF_LOG_INFO("Writing m_auth_counter...");

        ret = fds_record_write(&m_counter_record_desc, &m_counter_record);
        if(ret != NRF_SUCCESS) return ret;
    }

#ifdef CONFIG_RANDOM_AES_KEY_ENABLED
    /* update AES key */

    fds_record_desc_t aes_key_record_desc = {0};

    memset(&tok, 0, sizeof(fds_find_token_t));

    ret = fds_record_find(CONFIG_AES_KEY_FILE, CONFIG_AES_KEY_REC_KEY, 
                          &aes_key_record_desc, &tok);
    if(ret == NRF_SUCCESS)
    {
        fds_flash_record_t config = {0};

        /* Open the record and read its contents. */
        ret = fds_record_open(&aes_key_record_desc, &config);
        if(ret != NRF_SUCCESS) return ret;
        
        /* Copy the counter value from flash into aes_key. */
        memcpy(aes_key, config.p_data, AES_KEY_SIZE);

        /* Close the record when done reading. */
        ret = fds_record_close(&aes_key_record_desc);
        if(ret != NRF_SUCCESS) return ret;
    }
    else
    {
        /* aes_key not found; generate a random one. */
        NRF_LOG_INFO("Generating a random AES key...");

        ret = nrf_crypto_rng_vector_generate(aes_key, AES_KEY_SIZE);
        if(ret != NRF_SUCCESS) return ret;

        ret = fds_record_write(&aes_key_record_desc, &m_aes_key_record);
        if(ret != NRF_SUCCESS) return ret;
    }
#endif /* CONFIG_RANDOM_AES_KEY_ENABLED */

    return ret;
}


uint16_t u2f_register(U2F_REGISTER_REQ * p_req, U2F_REGISTER_RESP * p_resp, 
                      int flags, uint16_t * p_resp_len)
{
    NRF_LOG_INFO("u2f_register starting...");
    ret_code_t ret;
    size_t len;
    uint8_t buf[64];

    memset(p_resp, 0, sizeof(*p_resp));
    *p_resp_len = 0;
    p_resp->registerId = U2F_REGISTER_ID;

    if(!is_user_button_pressed())
    {
        return U2F_SW_CONDITIONS_NOT_SATISFIED;
    }

    bsp_board_led_on(LED_U2F_WINK);

    /* Generate a key pair */
    nrf_crypto_ecc_private_key_t privkey;
    nrf_crypto_ecc_public_key_t pubkey;
    
    ret = nrf_crypto_ecc_key_pair_generate(NULL, 
          &g_nrf_crypto_ecc_secp256r1_curve_info, &privkey, &pubkey);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to generate key pair! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    /* Export EC Public key */
    len = U2F_EC_KEY_SIZE * 2;
    ret = nrf_crypto_ecc_public_key_to_raw(&pubkey, buf, &len);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to export EC Public key! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    p_resp->pubKey.pointFormat = U2F_POINT_UNCOMPRESSED;
    memcpy(&p_resp->pubKey.x[0], buf, U2F_EC_KEY_SIZE * 2);

    /* Export EC Private key to buf */
    len = U2F_EC_KEY_SIZE;
    ret = nrf_crypto_ecc_private_key_to_raw(&privkey, buf, &len);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to export EC Private key! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    /* Copy appId to buf after private key */
    memcpy(buf + U2F_EC_KEY_SIZE, p_req->appId, U2F_APPID_SIZE);

    nrf_crypto_aes_context_t ecb_encr_128_ctx; // AES ECB encryption context

    /* Init encryption context for 128 bit key */
    ret = nrf_crypto_aes_init(&ecb_encr_128_ctx,
                              &g_nrf_crypto_aes_ecb_128_info,
                              NRF_CRYPTO_ENCRYPT);

    /* Convert EC private key to a key handle -> encrypt it and the appId 
     * using an AES private key */
    len = U2F_MAX_KH_SIZE;
    ret += nrf_crypto_aes_crypt(&ecb_encr_128_ctx,
                                &g_nrf_crypto_aes_ecb_128_info,
                                NRF_CRYPTO_ENCRYPT,
                                aes_key,
                                NULL,
                                buf,
                                U2F_EC_KEY_SIZE + U2F_APPID_SIZE,
                                p_resp->keyHandleCertSig,
                                &len);

    p_resp->keyHandleLen = len;

    ret += nrf_crypto_aes_uninit(&ecb_encr_128_ctx);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("AES encryption failed! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    /* Copy x509 attestation public key certificate */
    memcpy(&p_resp->keyHandleCertSig[p_resp->keyHandleLen], attestation_cert, 
           attestation_cert_size);

    /* Compute SHA256 hash of appId & chal & keyhandle & pubkey */

    nrf_crypto_hash_context_t   hash_context;

    memset(buf, 0, sizeof(buf));

    // Initialize the hash context
    ret = nrf_crypto_hash_init(&hash_context, &g_nrf_crypto_hash_sha256_info);

    /* hash update buf[0] = 0x00 */
    ret += nrf_crypto_hash_update(&hash_context, buf, 1);

    /* The application parameter [32 bytes] from 
     * the registration request message. */
    ret += nrf_crypto_hash_update(&hash_context, p_req->appId, U2F_APPID_SIZE);

    /* The challenge parameter [32 bytes] from 
     * the registration request message. */
    ret += nrf_crypto_hash_update(&hash_context, p_req->chal, U2F_CHAL_SIZE);

    /* The key handle [variable length] */
    ret += nrf_crypto_hash_update(&hash_context, p_resp->keyHandleCertSig, 
                                  p_resp->keyHandleLen);

    /* The user public key [65 bytes]. */
    ret += nrf_crypto_hash_update(&hash_context, (uint8_t *)&p_resp->pubKey, 
                                  U2F_EC_POINT_SIZE);

    len = 32;
    ret += nrf_crypto_hash_finalize(&hash_context, buf, &len);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to calculate hash! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    nrf_crypto_ecc_private_key_t sign_private_key;
    /* Sign the SHA256 hash using the attestation key */
    ret = nrf_crypto_ecc_private_key_from_raw(
                                        &g_nrf_crypto_ecc_secp256r1_curve_info,
                                        &sign_private_key,
                                        attestation_private_key,
                                        attestation_private_key_size);

    nrf_crypto_ecdsa_secp256r1_signature_t m_signature;
    size_t m_signature_size = sizeof(m_signature);

    ret += nrf_crypto_ecdsa_sign(NULL,
                                 &sign_private_key,
                                 buf,
                                 len,
                                 m_signature,
                                 &m_signature_size);
    // Key deallocation
    ret += nrf_crypto_ecc_private_key_free(&sign_private_key);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to generate signature! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    m_signature_size = signature_convert(
        &p_resp->keyHandleCertSig[p_resp->keyHandleLen + attestation_cert_size], 
        m_signature);

    *p_resp_len = p_resp->keyHandleCertSig - (uint8_t *)p_resp 
                  + p_resp->keyHandleLen + attestation_cert_size 
                  + m_signature_size;

    return U2F_SW_NO_ERROR;
}



uint16_t u2f_authenticate(U2F_AUTHENTICATE_REQ * p_req, 
                          U2F_AUTHENTICATE_RESP * p_resp, 
                          int flags, uint16_t * p_resp_len)
{
    NRF_LOG_INFO("u2f_authenticate starting...");

    ret_code_t ret;
    size_t len;
    uint8_t buf[U2F_EC_KEY_SIZE + U2F_APPID_SIZE];

    *p_resp_len = 0;

    if(flags == U2F_AUTH_ENFORCE && !is_user_button_pressed())
    {
        return U2F_SW_CONDITIONS_NOT_SATISFIED;
    }

    bsp_board_led_on(LED_U2F_WINK);

    /* Convert key handle to EC private key -> 
     * decrypt it using AES private key */
    nrf_crypto_aes_context_t ecb_decr_128_ctx; // AES ECB decryption context

    /* Init decryption context for 128 bit key */
    ret = nrf_crypto_aes_init(&ecb_decr_128_ctx,
                              &g_nrf_crypto_aes_ecb_128_info,
                              NRF_CRYPTO_DECRYPT);

    len = sizeof(buf);
    ret += nrf_crypto_aes_crypt(&ecb_decr_128_ctx,
                                &g_nrf_crypto_aes_ecb_128_info,
                                NRF_CRYPTO_DECRYPT,
                                aes_key,
                                NULL,
                                p_req->keyHandle,
                                p_req->keyHandleLen,
                                buf,
                                &len);
    ret += nrf_crypto_aes_uninit(&ecb_decr_128_ctx);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("AES decryption failed! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;        
    }

    if(memcmp(&buf[U2F_EC_KEY_SIZE], p_req->appId, U2F_APPID_SIZE) != 0)
    {
        NRF_LOG_ERROR("APPID MISMATCH!");
        return U2F_SW_WRONG_DATA;
    }

    uint32_big_encode(m_auth_counter, p_resp->ctr);
    m_auth_counter++;
    /* Write the updated record to flash. */
    ret = fds_record_update(&m_counter_record_desc, &m_counter_record);
    APP_ERROR_CHECK(ret);

    p_resp->flags = U2F_AUTH_FLAG_TUP;

    /* Get private key */
    nrf_crypto_ecc_private_key_t private_key;
    ret = nrf_crypto_ecc_private_key_from_raw(
                                        &g_nrf_crypto_ecc_secp256r1_curve_info,
                                        &private_key,
                                        buf,
                                        U2F_EC_KEY_SIZE);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to get private key from raw! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    /* Compute SHA256 hash of appId & user presence & counter & chal */
    nrf_crypto_hash_context_t   hash_context;

    // Initialize the hash context
    ret = nrf_crypto_hash_init(&hash_context, &g_nrf_crypto_hash_sha256_info);
    
    /* hash update appId */
    ret += nrf_crypto_hash_update(&hash_context, p_req->appId, U2F_APPID_SIZE);

    /* hash update user presence */
    ret += nrf_crypto_hash_update(&hash_context, &p_resp->flags, 1);

    /* hash update counter */
    ret += nrf_crypto_hash_update(&hash_context, p_resp->ctr, U2F_CTR_SIZE);

    /* hash update chal */
    ret += nrf_crypto_hash_update(&hash_context, p_req->chal, U2F_CHAL_SIZE);

    len = 32;
    ret += nrf_crypto_hash_finalize(&hash_context, buf, &len);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to calculate hash! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    /* Sign the SHA256 hash using the private key */
    nrf_crypto_ecdsa_secp256r1_signature_t m_signature;
    size_t m_signature_size = sizeof(m_signature);

    ret = nrf_crypto_ecdsa_sign(NULL,
                                &private_key,
                                buf,
                                len,
                                m_signature,
                                &m_signature_size);

    // Key deallocation
    ret += nrf_crypto_ecc_private_key_free(&private_key);
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to generate signature! [code = %d]", ret);
        return U2F_SW_INS_NOT_SUPPORTED;
    }

    m_signature_size = signature_convert(p_resp->sig, m_signature);

    *p_resp_len = p_resp->sig - (uint8_t *)p_resp + m_signature_size;

    return U2F_SW_NO_ERROR;
}


/**@brief Convert a signature to the correct format. For more info:
 * http://bitcoin.stackexchange.com/questions/12554/why-the-signature-is-always
 * -65-13232-bytes-long
 *
 * @param[in]  p_dest_sig  The output signature.
 * @param[in]  p_src_sig   The input signature.
 *
 * @retval     The output signature size.
 *
 */
static uint16_t signature_convert(uint8_t * p_dest_sig, uint8_t * p_src_sig)
{
    int idx = 0;

    p_dest_sig[idx++] = 0x30; //header: compound structure

    uint8_t *p_len = &p_dest_sig[idx];

    p_dest_sig[idx++] = 0x44; //total length (32 + 32 + 2 + 2) at least
    
    p_dest_sig[idx++] = 0x02; //header: integer

    if(p_src_sig[0] > 0x7f)
    {
        p_dest_sig[idx++] = 33;
        p_dest_sig[idx++] = 0;
        (*p_len)++;
    }
    else
    {
        p_dest_sig[idx++] = 32;
    }

    memcpy(&p_dest_sig[idx], p_src_sig, 32);
    idx += 32;

    p_dest_sig[idx++] = 0x02;

    if(p_src_sig[32] > 0x7f)
    {
        p_dest_sig[idx++] = 33;
        p_dest_sig[idx++] = 0;
        (*p_len)++;
    }
    else
    {
        p_dest_sig[idx++] = 32;
    }

    memcpy(&p_dest_sig[idx], p_src_sig+32, 32);
    idx += 32;

    return idx;  // new signature size
}




