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


#ifndef U2F_HID_IF_H__
#define U2F_HID_IF_H__

#include "nrf_drv_usbd.h"
#include "app_usbd_hid_generic.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable USB power detection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

/**
 * @brief HID generic class interface number.
 * */
#define U2F_HID_INTERFACE  0

/**
 * @brief HID generic class endpoint number.
 * */
#define U2F_HID_EPIN       NRF_DRV_USBD_EPIN1
#define U2F_HID_EPOUT      NRF_DRV_USBD_EPOUT1

/**
 * @brief Number of reports defined in report descriptor.
 */
#define REPORT_IN_QUEUE_SIZE    1

/**
 * @brief Size of maximum output report. HID generic class will reserve
 *        this buffer size + 1 memory space. 
 *
 * Maximum value of this define is 63 bytes. Library automatically adds
 * one byte for report ID. This means that output report size is limited
 * to 64 bytes.
 */
#define REPORT_OUT_MAXSIZE  63

/**
 * @brief HID generic class endpoints count.
 * */
#define HID_GENERIC_EP_COUNT  2

/**
 * @brief List of HID generic class endpoints.
 * */
#define ENDPOINT_LIST()                                  \
(                                                        \
        U2F_HID_EPIN,                                    \
        U2F_HID_EPOUT                                    \
)

/**
 * @brief FIDO U2F HID report descriptor.
 *
 */
#define APP_USBD_U2F_HID_REPORT_DSC {                                     \
    0x06, 0xd0, 0xf1, /*     Usage Page (FIDO Alliance),           */     \
    0x09, 0x01,       /*     Usage (U2F HID Authenticator Device), */     \
    0xa1, 0x01,       /*     Collection (Application),             */     \
    0x09, 0x20,       /*     Usage (Input Report Data),            */     \
    0x15, 0x00,       /*     Logical Minimum (0),                  */     \
    0x26, 0xff, 0x00, /*     Logical Maximum (255),                */     \
    0x75, 0x08,       /*     Report Size (8),                      */     \
    0x95, 0x40,       /*     Report Count (64),                    */     \
    0x81, 0x02,       /*     Input (Data, Variable, Absolute)      */     \
    0x09, 0x21,       /*     Usage (Output Report Data),           */     \
    0x15, 0x00,       /*     Logical Minimum (0),                  */     \
    0x26, 0xff, 0x00, /*     Logical Maximum (255),                */     \
    0x75, 0x08,       /*     Report Size (8),                      */     \
    0x95, 0x40,       /*     Report Count (64),                    */     \
    0x91, 0x02,       /*     Output (Data, Variable, Absolute)     */     \
    0xc0,             /*     End Collection,                       */     \
}


/**
 * @brief Initialize USB HID interface.
 *
 * @return Standard error code.
 */
uint32_t u2f_hid_if_init(void);


/**
 * @brief Send U2F HID Data.
 *
 *
 * @param[in] cid       HID Channel identifier.
 * @param[in] cmd       Frame command.
 * @param[in] p_data    Frame Data packet.
 * @param[in] size      Data length
 *
 * @return Standard error code.
 */
uint8_t u2f_hid_if_send(uint32_t cid, uint8_t cmd, 
                        uint8_t * p_data, size_t size);



/**
 * @brief Receive U2F HID Data.
 *
 *
 * @param[out] p_cid       HID Channel identifier.
 * @param[out] p_cmd       Frame command.
 * @param[out] p_data      Frame Data packet.
 * @param[out] p_size      Data length
 * @param[in]  timeout     message timeout in ms 
 *
 * @return Standard error code.
 */
uint8_t u2f_hid_if_recv(uint32_t * p_cid, uint8_t * p_cmd, 
                    uint8_t * p_data, size_t * p_size,
                    uint32_t timeout);

/**
 * @brief U2F HID interface process.
 *
 *
 */
void u2f_hid_if_process(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* U2F_HID_IF_H__ */
