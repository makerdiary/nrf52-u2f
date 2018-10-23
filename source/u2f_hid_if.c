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

#include "nrf.h"
#include "app_util_platform.h"
#include "app_fifo.h"
#include "bsp.h"

#include "timer_interface.h"

#include "u2f_hid.h"
#include "u2f_hid_if.h"

#include "nrf_drv_usbd.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_hid_generic.h"

#define NRF_LOG_MODULE_NAME u2f_hid_if

#include "nrf_log.h"

NRF_LOG_MODULE_REGISTER();


//static uint8_t u2f_hid_fifo_buf[1024];

//static app_fifo_t m_u2f_hid_fifo;

/**
 * @brief User event handler.
 * */
static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event);


/**
 * @brief Reuse HID mouse report descriptor for HID generic class
 */
APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(u2f_hid_desc,
                                          APP_USBD_U2F_HID_REPORT_DSC);

static const app_usbd_hid_subclass_desc_t * reports[] = {&u2f_hid_desc};

/*lint -save -e26 -e64 -e123 -e505 -e651*/

/**
 * @brief Global HID generic instance
 */
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_u2f_hid,
                                U2F_HID_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(),
                                reports,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_NONE,
                                APP_USBD_HID_PROTO_GENERIC);

/*lint -restore*/


/**
 * @brief Mark the ongoing transmission
 *
 * Marks that the report buffer is busy and cannot be used until 
 * transmission finishes or invalidates (by USB reset or suspend event).
 */
static bool m_report_pending = false;


/**
 * @brief Mark a report received.
 *
 */
static bool m_report_received = false;


/**
 * \brief send one HID frame. 
 */
static uint8_t u2f_hid_if_frame_send(U2FHID_FRAME * p_frame)
{
    ret_code_t ret;

    while(m_report_pending){
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
    }

    ret = app_usbd_hid_generic_in_report_set(
        &m_app_u2f_hid,
        (uint8_t *)p_frame,
        HID_RPT_SIZE);

    if(ret == NRF_SUCCESS)
    {
        m_report_pending = true;
        return ERR_NONE;
    }

    return ERR_OTHER;
}


uint8_t u2f_hid_if_send(uint32_t cid, uint8_t cmd, uint8_t *p_data, size_t size)
{
    U2FHID_FRAME frame;
    int ret;
    size_t frameLen;
    uint8_t seq = 0;

    frame.cid = cid;
    frame.init.cmd = TYPE_INIT | cmd;
    frame.init.bcnth = (size >> 8) & 0xFF;
    frame.init.bcntl = (size & 0xFF);

    frameLen = MIN(size, sizeof(frame.init.data));
    memset(frame.init.data, 0, sizeof(frame.init.data));
    memcpy(frame.init.data, p_data, frameLen);

    do 
    {
        ret = u2f_hid_if_frame_send(&frame);
        if(ret != ERR_NONE) return ret;

        size -= frameLen;
        p_data += frameLen;

        frame.cont.seq = seq++;
        frameLen = MIN(size, sizeof(frame.cont.data));
        memset(frame.cont.data, 0, sizeof(frame.cont.data));
        memcpy(frame.cont.data, p_data, frameLen);
    } while(size);

    return ERR_NONE;
}



uint8_t u2f_hid_if_recv(uint32_t * p_cid, uint8_t * p_cmd, 
                    uint8_t * p_data, size_t * p_size,
                    uint32_t timeout)
{
    uint8_t * p_recv_buf;
    size_t recv_size, totalLen, frameLen;
    U2FHID_FRAME * p_frame;
    uint8_t seq = 0;

    Timer timer;
    countdown_ms(&timer, timeout);

    if(!m_report_received) return ERR_OTHER+1;
    m_report_received = false;

    p_recv_buf = (uint8_t *)app_usbd_hid_generic_out_report_get(&m_app_u2f_hid, 
                                                                &recv_size);

    if(recv_size != sizeof(U2FHID_FRAME)) return ERR_OTHER;

    p_frame = (U2FHID_FRAME *)p_recv_buf;

    if(FRAME_TYPE(*p_frame) != TYPE_INIT) return ERR_INVALID_CMD;

    *p_cid = p_frame->cid;
    *p_cmd = p_frame->init.cmd;

    totalLen = MSG_LEN(*p_frame);
    frameLen = MIN(sizeof(p_frame->init.data), totalLen);

    *p_size = totalLen;

    memcpy(p_data, p_frame->init.data, frameLen);
    totalLen -= frameLen;
    p_data += frameLen;

    while(totalLen)
    {
        while(!m_report_received)
        {
            while (app_usbd_event_queue_process())
            {
                /* Nothing to do */
            }
            if(has_timer_expired(&timer)) return ERR_MSG_TIMEOUT;
        }
        m_report_received = false;

        p_recv_buf = (uint8_t *)app_usbd_hid_generic_out_report_get(
                                                                &m_app_u2f_hid, 
                                                                &recv_size);

        if(recv_size != sizeof(U2FHID_FRAME)) continue;

        p_frame = (U2FHID_FRAME *)p_recv_buf;

        if(p_frame->cid != *p_cid) continue;
        if(FRAME_TYPE(*p_frame) != TYPE_CONT) return ERR_INVALID_SEQ;
        if(FRAME_SEQ(*p_frame) != seq++) return ERR_INVALID_SEQ;

        frameLen = MIN(sizeof(p_frame->cont.data), totalLen);

        memcpy(p_data, p_frame->cont.data, frameLen);
        totalLen -= frameLen;
        p_data += frameLen;
    }

    return ERR_NONE;
}


/**
 * @brief Class specific event handler.
 *
 * @param p_inst    Class instance.
 * @param event     Class specific event.
 * */
static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event)
{

    switch (event)
    {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
        {
            m_report_received = true;
            break;
        }
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
        {
            m_report_pending = false;
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        {
            UNUSED_RETURN_VALUE(hid_generic_clear_buffer(p_inst));
            NRF_LOG_INFO("SET_BOOT_PROTO");
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
        {
            UNUSED_RETURN_VALUE(hid_generic_clear_buffer(p_inst));
            NRF_LOG_INFO("SET_REPORT_PROTO");
            break;
        }
        default:
            break;
    }
}

/**
 * @brief USBD library specific event handler.
 *
 * @param event     USBD library event.
 * */
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SOF:
            break;
        case APP_USBD_EVT_DRV_RESET:
            m_report_pending = false;
            break;
        case APP_USBD_EVT_DRV_SUSPEND:
            m_report_pending = false;
            // Allow the library to put the peripheral into sleep mode
            app_usbd_suspend_req(); 
            bsp_board_leds_off();
            break;
        case APP_USBD_EVT_DRV_RESUME:
            m_report_pending = false;
            bsp_board_led_on(LED_U2F_WINK);
            break;
        case APP_USBD_EVT_STARTED:
            m_report_pending = false;
            bsp_board_led_on(LED_U2F_WINK);
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            bsp_board_leds_off();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}


/**
 * @brief handler for idle reports.
 *
 * @param p_inst      Class instance.
 * @param report_id   Number of report ID that needs idle transfer.
 * */
static ret_code_t idle_handle(app_usbd_class_inst_t const * p_inst, 
                              uint8_t report_id)
{
    switch (report_id)
    {
        case 0:
        {
            uint8_t report[] = {0xBE, 0xEF};
            return app_usbd_hid_generic_idle_report_set(
              &m_app_u2f_hid,
              report,
              sizeof(report));
        }
        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }
}


uint32_t u2f_hid_if_init(void)
{
	ret_code_t ret;

	static const app_usbd_config_t usbd_config = {
	    .ev_state_proc = usbd_user_ev_handler
	};

	ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_inst_u2f;
    class_inst_u2f = app_usbd_hid_generic_class_inst_get(&m_app_u2f_hid);

    ret = hid_generic_idle_handler_set(class_inst_u2f, idle_handle);
    APP_ERROR_CHECK(ret);

    ret = app_usbd_class_append(class_inst_u2f);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    return ret;
}

void u2f_hid_if_process(void)
{
    while (app_usbd_event_queue_process())
    {
        /* Nothing to do */
    }
}


