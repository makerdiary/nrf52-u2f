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

/**
 * @file timer.c
 * @brief implementation of the timer interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "timer_platform.h"


extern volatile uint32_t ms_ticks;
/**
 * \brief Get time in milliseconds.
 *
 * \return milli second ticks count.
 */
static uint32_t getTimeInMillis(void)
{
	return ms_ticks;
}

/**
 * @brief Check if a timer is expired
 *
 * Call this function passing in a timer to check if that timer has expired.
 *
 * @param Timer - pointer to the timer to be checked for expiration
 * @return bool - true = timer expired, false = timer not expired
 */

bool has_timer_expired(Timer *timer) {
	return ((timer->end_time > 0)
	&& ((getTimeInMillis() + timer->offset) > timer->end_time));
}

/**
 * @brief Create a timer (milliseconds)
 *
 * Sets the timer to expire in a specified number of milliseconds.
 *
 * @param Timer - pointer to the timer to be set to expire in milliseconds
 * @param uint32_t - set the timer to expire in this number of milliseconds
 */
void countdown_ms(Timer *timer, uint32_t timeout) {
	uint32_t timems = getTimeInMillis();

	timer->end_time = timems + timeout;

	if (timer->end_time < timems) {
		timer->offset = ~0 - timems + 1;
		timer->end_time += timer->offset;
	}
	else {
		timer->offset = 0;
	}
}

/**
 * @brief Create a timer (seconds)
 *
 * Sets the timer to expire in a specified number of seconds.
 *
 * @param Timer - pointer to the timer to be set to expire in seconds
 * @param uint32_t - set the timer to expire in this number of seconds
 */
 void countdown_sec(Timer *timer, uint32_t timeout) {
	uint32_t timems = getTimeInMillis();

	timer->end_time = timems + (timeout * 1000);

	if (timer->end_time < timems) {
		timer->offset = ~0 - timems + 1;
		timer->end_time += timer->offset;
	}
	else {
		timer->offset = 0;
	}
}

/**
 * @brief Check the time remaining on a given timer
 *
 * Checks the input timer and returns the number of milliseconds remaining 
 * on the timer.
 *
 * @param Timer - pointer to the timer to be set to checked
 * @return int - milliseconds left on the countdown timer
 */
uint32_t left_ms(Timer *timer) {
	int diff = timer->end_time - (getTimeInMillis() + timer->offset);
	return (diff > 0 ? diff : 0);
}

/**
 * @brief Initialize a timer
 *
 * Performs any initialization required to the timer passed in.
 *
 * @param Timer - pointer to the timer to be initialized
 */
void init_timer(Timer *timer) {
	timer->end_time = 0;
	timer->offset = 0;
}

#ifdef __cplusplus
}
#endif
