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

#ifndef __TIMER_INTERFACE_H_
#define __TIMER_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The platform specific timer header that defines the Timer struct
 */
#include "timer_platform.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Timer Type
 *
 * Forward declaration of a timer struct.  The definition of this struct is
 * platform dependent.  When porting to a new platform add this definition
 * in "timer_<platform>.h" and include that file above.
 *
 */
typedef struct Timer Timer;

/**
 * @brief Check if a timer is expired
 *
 * Call this function passing in a timer to check if that timer has expired.
 *
 * @param Timer - pointer to the timer to be checked for expiration
 * @return bool - true = timer expired, false = timer not expired
 */
bool has_timer_expired(Timer *);

/**
 * @brief Create a timer (milliseconds)
 *
 * Sets the timer to expire in a specified number of milliseconds.
 *
 * @param Timer - pointer to the timer to be set to expire in milliseconds
 * @param uint32_t - set the timer to expire in this number of milliseconds
 */
void countdown_ms(Timer *, uint32_t);

/**
 * @brief Create a timer (seconds)
 *
 * Sets the timer to expire in a specified number of seconds.
 *
 * @param Timer - pointer to the timer to be set to expire in seconds
 * @param uint32_t - set the timer to expire in this number of seconds
 */
void countdown_sec(Timer *, uint32_t);

/**
 * @brief Check the time remaining on a given timer
 *
 * Checks the input timer and returns the number of milliseconds remaining 
 * on the timer.
 *
 * @param Timer - pointer to the timer to be set to checked
 * @return int - milliseconds left on the countdown timer
 */
uint32_t left_ms(Timer *);

/**
 * @brief Initialize a timer
 *
 * Performs any initialization required to the timer passed in.
 *
 * @param Timer - pointer to the timer to be initialized
 */
void init_timer(Timer *);

#ifdef __cplusplus
}
#endif

#endif //__TIMER_INTERFACE_H_
