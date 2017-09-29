/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
 * @file
 * @ingroup Drivers
 *
 * This API provides a means to start, stop, and hookup an ISR function to RIT
 */
#ifndef LPC_RIT_TIMER_H
#define LPC_RIT_TIMER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "lpc_sys.h" // void_func_t

/**
 * Setup a repetitive callback function at the given time.
 * @param [in] function  The void function name
 * @param [in] time_ms   The time in milliseconds
 */
void rit_enable(void_func_t function, uint32_t time_ms);

/// Disables the RIT setup by sys_rit_setup()
void rit_disable(void);

/// @returns true if the RIT is running.
bool rit_is_running(void);



#ifdef __cplusplus
}
#endif
#endif /* LPC_RIT_TIMER_H */
