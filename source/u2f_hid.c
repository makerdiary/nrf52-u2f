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

#include "u2f.h"
#include "u2f_hid.h"
#include "u2f_hid_if.h"

#include "mem_manager.h"
#include "timer_interface.h"

#define NRF_LOG_MODULE_NAME u2f_hid

#include "nrf_log.h"

NRF_LOG_MODULE_REGISTER();

#define MAX_U2F_CHANNELS    5

#define CID_STATE_IDLE      1
#define CID_STATE_READY     2


typedef struct { struct u2f_channel *pFirst, *pLast; } u2f_channel_list_t;

typedef struct u2f_channel {
    struct u2f_channel * pPrev;
    struct u2f_channel * pNext;
    uint32_t cid;
    uint8_t cmd;
    uint8_t state;
    Timer timer;
    uint16_t bcnt;
    uint8_t req[U2F_MAX_REQ_SIZE];
    uint8_t resp[U2F_MAX_RESP_SIZE];
} u2f_channel_t;

typedef struct __attribute__ ((__packed__))
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t lc1;
    uint8_t lc2;
    uint8_t lc3;
} u2f_req_apdu_header_t;


extern bool is_user_button_pressed(void);


/**
 * @brief List of channels.
 *
 */
u2f_channel_list_t m_u2f_ch_list = {NULL, NULL};


/**
 * @brief The count of channel used.
 *
 */
static uint8_t m_channel_used_cnt = 0;


/**@brief U2F Channel allocation function.
 *
 *
 * @retval    Valid memory location if the procedure was successful, else, NULL.
 */
static u2f_channel_t * u2f_channel_alloc(void)
{
    u2f_channel_t * p_ch;
    size_t size = sizeof(u2f_channel_t);

    if(m_channel_used_cnt > MAX_U2F_CHANNELS)
    {
        NRF_LOG_WARNING("MAX_U2F_CHANNELS.");
        return NULL;
    }

    p_ch = nrf_malloc(size);
    if(p_ch == NULL)
    {
        NRF_LOG_ERROR("nrf_malloc: Invalid memory location!");
    }
    else
    {
        m_channel_used_cnt++;       
    }

    return p_ch;
}


/**@brief Initialize U2F Channel.
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * @param[in]  cid   Channel identifier.
 *
 */
static void u2f_channel_init(u2f_channel_t * p_ch, uint32_t cid)
{
    size_t size = sizeof(u2f_channel_t);

    memset(p_ch, 0, size);

    p_ch->cid = cid;
    p_ch->state = CID_STATE_IDLE;
    p_ch->pPrev = NULL;
    p_ch->pNext = NULL;

    if(m_u2f_ch_list.pFirst == NULL)
    {
        m_u2f_ch_list.pFirst = m_u2f_ch_list.pLast = p_ch;
    }
    else
    {
        p_ch->pPrev = m_u2f_ch_list.pLast;
        m_u2f_ch_list.pLast->pNext = p_ch;
        m_u2f_ch_list.pLast = p_ch;
    }
}


/**@brief Uninitialize U2F Channel.
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 *
 */
static void u2f_channel_deinit(u2f_channel_t * p_ch)
{
    if(p_ch->pPrev == NULL && p_ch->pNext == NULL)  //only one item in the list
    {
        m_u2f_ch_list.pFirst = m_u2f_ch_list.pLast = NULL;
    }
    else if(p_ch->pPrev == NULL)  // the first item
    {
        m_u2f_ch_list.pFirst = p_ch->pNext;
        p_ch->pNext->pPrev = NULL;
    }
    else if(p_ch->pNext == NULL) // the last item
    {
        m_u2f_ch_list.pLast = p_ch->pPrev;
        p_ch->pPrev->pNext = NULL;
    }
    else
    {
        p_ch->pPrev->pNext = p_ch->pNext;
        p_ch->pNext->pPrev = p_ch->pPrev;
    }
    nrf_free(p_ch);
    m_channel_used_cnt--;
}


/**@brief Find the U2F Channel by cid.
 *
 * @param[in]  cid  Channel identifier.
 *
 * @retval     Valid U2F Channel if the procedure was successful, else, NULL.
 */
static u2f_channel_t * u2f_channel_find(uint32_t cid)
{
    
    u2f_channel_t *p_ch;

    for(p_ch = m_u2f_ch_list.pFirst; p_ch != NULL; p_ch = p_ch->pNext)
    {
        if(p_ch->cid == cid)
        {
            return p_ch;
        }
    }

    return NULL;
}


/**@brief Generate new U2F Channel identifier.
 *
 *
 * @retval     New Channel identifier.
 */
static uint32_t generate_new_cid(void)
{
    static uint32_t cid = 0;
    do
    {
        cid++;
    }while(cid == 0 || cid == CID_BROADCAST);
    return cid;
}


/**@brief Send a U2FHID_ERROR response
 *
 * @param[in]  cid   Channel identifier.
 * @param[in]  code  Error code.
 * 
 */
static void u2f_hid_error_response(uint32_t cid, uint8_t error)
{
    u2f_hid_if_send(cid, U2FHID_ERROR, &error, 1);
}


/**@brief Handle a U2FHID INIT response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_init_response(u2f_channel_t *p_ch)
{
    U2FHID_INIT_RESP *p_resp_init = (U2FHID_INIT_RESP *)p_ch->resp;
    
    u2f_channel_t *p_new_ch;

    if (p_ch->cid != CID_BROADCAST) {
         u2f_hid_error_response(p_ch->cid, ERR_INVALID_CMD);
         return;
    }

    p_new_ch = u2f_channel_alloc();
    if (p_new_ch == NULL) {
         u2f_hid_error_response(p_ch->cid, ERR_CHANNEL_BUSY);
         return;
    }

    u2f_channel_init(p_new_ch, generate_new_cid());

    memcpy(p_resp_init->nonce, p_ch->req, INIT_NONCE_SIZE);

    p_resp_init->cid = p_new_ch->cid;                    // Channel identifier 
    p_resp_init->versionInterface = U2FHID_IF_VERSION;   // Interface version
    p_resp_init->versionMajor = U2FHID_FW_VERSION_MAJOR; // Major version number
    p_resp_init->versionMinor = U2FHID_FW_VERSION_MINOR; // Minor version number
    p_resp_init->versionBuild = U2FHID_FW_VERSION_BUILD; // Build version number
    p_resp_init->capFlags = CAPFLAG_WINK;                // Capabilities flags

    UNUSED_RETURN_VALUE(is_user_button_pressed());    // clear user button state

    u2f_hid_if_send(p_ch->cid, p_ch->cmd, (uint8_t *)p_resp_init, 
                    sizeof(U2FHID_INIT_RESP));
}


/**@brief Handle a U2FHID WINK response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_wink_response(u2f_channel_t *p_ch)
{
    bsp_board_led_invert(LED_U2F_WINK);
    u2f_hid_if_send(p_ch->cid, p_ch->cmd, NULL, 0);
}


/**@brief Send a U2F HID status code only
 *
 * @param[in]  p_ch    Pointer to U2F Channel.
 * @param[in]  status  U2F HID status code.
 *
 */
static void u2f_hid_status_response(u2f_channel_t * p_ch, uint16_t status)
{
    uint8_t be_status[2];
    uint8_t size = uint16_big_encode(status, be_status);

    u2f_hid_if_send(p_ch->cid, p_ch->cmd, be_status, size);
}


/**@brief Handle a U2FHID MESSAGE response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_msg_response(u2f_channel_t * p_ch)
{
    u2f_req_apdu_header_t * p_req_apdu_hdr = (u2f_req_apdu_header_t *)p_ch->req;

    uint32_t req_size;

    if(p_req_apdu_hdr->cla != 0)
    {
        u2f_hid_status_response(p_ch, U2F_SW_CLA_NOT_SUPPORTED);
        return;
    }

    req_size = (((uint32_t)p_req_apdu_hdr->lc1) << 16) |
               (((uint32_t)p_req_apdu_hdr->lc2) << 8)  |
               (((uint32_t)p_req_apdu_hdr->lc3) << 0);

    switch(p_req_apdu_hdr->ins)
    {
        case U2F_REGISTER:
        {
            U2F_REGISTER_REQ *p_req = (U2F_REGISTER_REQ *)(p_req_apdu_hdr + 1);
            U2F_REGISTER_RESP *p_resp = (U2F_REGISTER_RESP *)p_ch->resp;

            if(req_size != sizeof(U2F_REGISTER_REQ))
            {
                NRF_LOG_ERROR("U2F_SW_WRONG_LENGTH.");
                u2f_hid_status_response(p_ch, U2F_SW_WRONG_LENGTH);
                return;                 
            }

            uint16_t status, len = 0;
            uint8_t be_status[2];

            status = u2f_register(p_req, p_resp, p_req_apdu_hdr->p1, &len);

            if(status == U2F_SW_CONDITIONS_NOT_SATISFIED)
            {
                NRF_LOG_WARNING("Press to register the device now...");
            }
            else if(status != U2F_SW_NO_ERROR)
            {
                NRF_LOG_ERROR("Fail to register your device! [status = %d]", status);
            }
            else
            {
                NRF_LOG_INFO("Register your device successfully!");
            }

            uint8_t size = uint16_big_encode(status, be_status);

            memcpy(p_ch->resp + len, be_status, size);

            u2f_hid_if_send(p_ch->cid, p_ch->cmd, p_ch->resp, len + size);

        }
        break;

        case U2F_AUTHENTICATE:
        {
            U2F_AUTHENTICATE_REQ *p_req = (U2F_AUTHENTICATE_REQ *)(p_req_apdu_hdr + 1);
            U2F_AUTHENTICATE_RESP *p_resp = (U2F_AUTHENTICATE_RESP *)p_ch->resp;

            if(req_size > sizeof(U2F_AUTHENTICATE_REQ))
            {
                NRF_LOG_ERROR("Invalid request size: %d", req_size);
                u2f_hid_status_response(p_ch, U2F_SW_WRONG_LENGTH);
                return;                 
            }

            uint16_t status, len = 0;
            uint8_t be_status[2];

            status = u2f_authenticate(p_req, p_resp, p_req_apdu_hdr->p1, &len);

            if(status == U2F_SW_CONDITIONS_NOT_SATISFIED)
            {
                NRF_LOG_WARNING("Press to authenticate your device now...");
            }
            else if(status != U2F_SW_NO_ERROR)
            {
                NRF_LOG_ERROR("Fail to authenticate your device! [status = %d]", status);
            }
            else
            {
                NRF_LOG_INFO("Authenticate your device successfully!");
            }

            uint8_t size = uint16_big_encode(status, be_status);
            
            memcpy(p_ch->resp + len, be_status, size);

            u2f_hid_if_send(p_ch->cid, p_ch->cmd, p_ch->resp, len + size);    
        }
        break;

        case U2F_VERSION:
        {
            const char *ver_str = VENDOR_U2F_VERSION;
            uint8_t len = strlen(ver_str);

            NRF_LOG_INFO("U2F_VERSION.");

            if(req_size > 0)
            {
                u2f_hid_status_response(p_ch, U2F_SW_WRONG_LENGTH);
               return;                 
            }

            uint8_t be_status[2];
            uint8_t size = uint16_big_encode(U2F_SW_NO_ERROR, be_status);
            memcpy(p_ch->resp, ver_str, len);
            memcpy(p_ch->resp + len, be_status, size);

            u2f_hid_if_send(p_ch->cid, p_ch->cmd, p_ch->resp, len + size);
        }
        break;

        case U2F_CHECK_REGISTER:
            break;

        case U2F_AUTHENTICATE_BATCH:
            break;

        default:
            NRF_LOG_ERROR("U2F_SW_INS_NOT_SUPPORTED.");
            u2f_hid_status_response(p_ch, U2F_SW_INS_NOT_SUPPORTED);
            break;
    }
}

/**@brief Handle a U2FHID PING response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_ping_response(u2f_channel_t *p_ch)
{
    u2f_hid_if_send(p_ch->cid, p_ch->cmd, p_ch->req, p_ch->bcnt);
}


/**@brief Handle a U2FHID SYNC response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_sync_response(u2f_channel_t *p_ch)
{
	return;
}


/**@brief Handle a U2FHID LOCK response
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_hid_lock_response(u2f_channel_t *p_ch)
{
	return;
}


/**@brief Process U2FHID command
 *
 * @param[in]  p_ch  Pointer to U2F Channel.
 * 
 */
static void u2f_channel_cmd_process(u2f_channel_t * p_ch)
{

    countdown_ms(&p_ch->timer, U2FHID_TRANS_TIMEOUT);

    if(p_ch->state != CID_STATE_READY) return;

    switch(p_ch->cmd)
    {
        case U2FHID_PING:
            NRF_LOG_INFO("U2FHID_PING.");
            u2f_hid_ping_response(p_ch);
            break;

        case U2FHID_MSG:
            NRF_LOG_INFO("U2FHID_MSG.");
            u2f_hid_msg_response(p_ch);
            break;

        case U2FHID_LOCK:
            NRF_LOG_INFO("U2FHID_LOCK.");
            u2f_hid_lock_response(p_ch);
            break;

        case U2FHID_INIT:
            NRF_LOG_INFO("U2FHID_INIT.");
            u2f_hid_init_response(p_ch);
            break;

        case U2FHID_WINK:
            NRF_LOG_INFO("U2FHID_WINK.");
            u2f_hid_wink_response(p_ch);
            break;

        case U2FHID_SYNC:
            NRF_LOG_INFO("U2FHID_SYNC.");
            u2f_hid_sync_response(p_ch);
            break;

        case U2FHID_VENDOR_FIRST:
            NRF_LOG_INFO("U2FHID_VENDOR_FIRST.");
            break;

        case U2FHID_VENDOR_LAST:
            NRF_LOG_INFO("U2FHID_VENDOR_LAST.");
            break;

        default:
            NRF_LOG_WARNING("Unknown Command: %d", p_ch->cmd);
            break;
    }

    p_ch->state = CID_STATE_IDLE;
}

/**@brief Process U2FHID command of every ready channel.
 * 
 */
static void u2f_channel_process(void)
{
    u2f_channel_t *p_ch;

    for(p_ch = m_u2f_ch_list.pFirst; p_ch != NULL;)
    {
        
        // Transaction timeout, free the channel
        if(has_timer_expired(&p_ch->timer) && p_ch->state == CID_STATE_IDLE)
        {
            if(p_ch->cid != CID_BROADCAST)
            {
                u2f_channel_t * p_free_ch = p_ch;
                p_ch = p_ch->pNext;
                u2f_channel_deinit(p_free_ch);
                continue;
            }
        }
        p_ch = p_ch->pNext;
    }
}


/**
 * @brief Function for initializing the U2F HID.
 *
 * @return Error status.
 *
 */
ret_code_t u2f_hid_init(void)
{
    ret_code_t ret;
    u2f_channel_t *p_ch;

    ret = nrf_mem_init();
    if(ret != NRF_SUCCESS)
    {
        return ret;
    }

    ret = u2f_hid_if_init();
    if(ret != NRF_SUCCESS)
    {
    	return ret;
    }

    ret = u2f_impl_init();
    if(ret != NRF_SUCCESS)
    {
    	return ret;
    }

    p_ch = u2f_channel_alloc();
    if(p_ch == NULL)
    {
        NRF_LOG_ERROR("NRF_ERROR_NULL!");
        return NRF_ERROR_NULL;
    }

    u2f_channel_init(p_ch, CID_BROADCAST);

    return NRF_SUCCESS;
}



/**
 * @brief U2FHID process function, which should be executed when data is ready.
 *
 */
void u2f_hid_process(void)
{
    uint8_t ret;
    uint32_t cid;
    uint8_t cmd;
    size_t size;
    uint8_t buf[U2F_MAX_REQ_SIZE];

    u2f_hid_if_process();

    ret = u2f_hid_if_recv(&cid, &cmd, buf, &size, 1000);

    if(ret == ERR_NONE)
    {
        u2f_channel_t * p_ch;

        p_ch = u2f_channel_find(cid);

        if(p_ch == NULL)
        {
            NRF_LOG_ERROR("No valid channel found!");
            u2f_hid_error_response(cid, ERR_CHANNEL_BUSY);
        }
        else
        {
            p_ch->cmd = cmd;
            p_ch->bcnt = size;
            p_ch->state = CID_STATE_READY;
            memcpy(p_ch->req, buf, size);
            u2f_channel_cmd_process(p_ch);
        }
    }

    u2f_channel_process();
}

