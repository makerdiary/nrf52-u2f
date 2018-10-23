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
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_drv_power.h"

#include "app_timer.h"
#include "app_error.h"
#include "bsp.h"

#include "u2f_hid.h"

#include "bsp_cli.h"
#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#define NRF_LOG_MODULE_NAME main

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

NRF_LOG_MODULE_REGISTER();

/**
 * @brief CLI interface over UART
 */
NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
            "u2f_cli:~$ ",
            &m_cli_uart_transport.transport,
            '\r',
            8);


/**
 * @brief Additional key release events
 *
 * This example needs to process release events of used buttons
 */
enum {
    BSP_USER_EVENT_RELEASE_0 = BSP_EVENT_KEY_LAST + 1, /**< Button 0 released */
    BSP_USER_EVENT_RELEASE_1,                          /**< Button 1 released */
    BSP_USER_EVENT_RELEASE_2,                          /**< Button 2 released */
    BSP_USER_EVENT_RELEASE_3,                          /**< Button 3 released */
    BSP_USER_EVENT_RELEASE_4,                          /**< Button 4 released */
    BSP_USER_EVENT_RELEASE_5,                          /**< Button 5 released */
    BSP_USER_EVENT_RELEASE_6,                          /**< Button 6 released */
    BSP_USER_EVENT_RELEASE_7,                          /**< Button 7 released */
};


/** SysTick counter to avoid busy wait delay. */
volatile uint32_t ms_ticks = 0;

/** U2F user button state. */
static bool m_user_button_pressed = false;

/**
 * \brief SysTick handler used to measure precise delay. 
 */
void SysTick_Handler(void)
{
    ms_ticks++;
}


/**
 * \brief Check user button state. 
 */
bool is_user_button_pressed(void)
{
    if(m_user_button_pressed)
    {
        m_user_button_pressed = false;
        return true;
    }
    return false;
}


static void bsp_event_callback(bsp_event_t ev)
{
    switch ((unsigned int)ev)
    {
        case CONCAT_2(BSP_EVENT_KEY_, BTN_U2F_USER):
            //NRF_LOG_INFO("BTN_U2F_USER pressed!");
            break;
        case CONCAT_2(BSP_USER_EVENT_RELEASE_, BTN_U2F_USER):
            m_user_button_pressed = true;
            //NRF_LOG_INFO("BTN_U2F_USER released!");
            break;
        default:
            return; // no implementation needed
    }
}


/**
 * @brief Auxiliary internal macro
 *
 * Macro used only in @ref init_bsp to simplify the configuration
 */
#define INIT_BSP_ASSIGN_RELEASE_ACTION(btn)                      \
    APP_ERROR_CHECK(                                             \
        bsp_event_to_button_action_assign(                       \
            btn,                                                 \
            BSP_BUTTON_ACTION_RELEASE,                           \
            (bsp_event_t)CONCAT_2(BSP_USER_EVENT_RELEASE_, btn)) \
    )

static void init_bsp(void)
{
    
    ret_code_t ret;
    ret = bsp_init(BSP_INIT_BUTTONS, bsp_event_callback);
    APP_ERROR_CHECK(ret);
    
    INIT_BSP_ASSIGN_RELEASE_ACTION(BTN_U2F_USER);
    
    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);

    /* Enable SysTick interrupt for non busy wait delay. */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        NRF_LOG_ERROR("init_bsp: SysTick configuration error!");
        while(1);
    }
}

static void init_cli(void)
{
    ret_code_t ret;
    ret = bsp_cli_init(bsp_event_callback);
    APP_ERROR_CHECK(ret);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = TX_PIN_NUMBER;
    uart_config.pselrxd = RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    ret = nrf_cli_init(&m_cli_uart, &uart_config, true, true, 
                       NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(&m_cli_uart);
    APP_ERROR_CHECK(ret);
}


int main(void)
{
    ret_code_t ret;

    ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    init_bsp();
    init_cli();

    NRF_LOG_INFO("Hello FIDO U2F Security Key!");

    ret = u2f_hid_init();
    if(ret != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Fail to initialize U2F HID![%d]", ret);
    }
    APP_ERROR_CHECK(ret);

    while (true)
    {

        u2f_hid_process();

        nrf_cli_process(&m_cli_uart);

        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
        
    }
}
