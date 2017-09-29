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
#include "lpc_rit.h"
#include "lpc_isr.h"
#include "FreeRTOS.h"



static void_func_t g_rit_callback = 0; /**< RIT Callback function pointer */


/** RIT Interrupt function (see startup.cpp) */
void RIT_IRQHandler()
{
    const uint32_t isr_flag_bitmask = (1 << 0);
    g_rit_callback();
    LPC_RIT->RICTRL |= isr_flag_bitmask;
}

void rit_enable(void_func_t function, uint32_t time_ms)
{
    if (0 == function) {
        return;
    }
    // Divide by zero guard
    if(0 == time_ms) {
        time_ms = 1;
    }

    // Power up first otherwise writing to RIT will give us Hard Fault
    lpc_pconp(pconp_rit, true);

    // Enable CLK/1 to simplify RICOMPVAL calculation below
    lpc_pclk(pclk_rit, clkdiv_1);

    LPC_RIT->RICTRL = 0;
    LPC_RIT->RICOUNTER = 0;
    LPC_RIT->RIMASK = 0;
    LPC_RIT->RICOMPVAL = sys_get_cpu_clock() / (1000 / time_ms);

    // Clear timer upon match, and enable timer
    const uint32_t isr_clear_bitmask = (1 << 0);
    const uint32_t timer_clear_bitmask = (1 << 1);
    const uint32_t timer_enable_bitmask = (1 << 3);
    LPC_RIT->RICTRL = isr_clear_bitmask | timer_clear_bitmask | timer_enable_bitmask;

    // Enable System Interrupt and connect the callback
    g_rit_callback = function;
    vTraceSetISRProperties(RIT_IRQn, "RIT", IP_RIT);
    NVIC_EnableIRQ(RIT_IRQn);
}

void rit_disable(void)
{
    LPC_RIT->RICTRL = 0;
    NVIC_DisableIRQ(RIT_IRQn);
}

bool rit_is_running(void)
{
    const uint32_t timer_enable_bitmask = (1 << 3);
    return !!(LPC_RIT->RICTRL & timer_enable_bitmask);
}
