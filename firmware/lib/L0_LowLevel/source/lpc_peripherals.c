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
 * @brief This file provides LPC peripheral numbers according to LPC17xx datasheet
 */
#include <stdint.h>
#include "LPC17xx.h"
#include "lpc_peripherals.h"
#include "FreeRTOS.h"



void lpc_pconp(lpc_pconp_t peripheral, bool on)
{
    vPortEnterCritical();
    if (on) {
        LPC_SC->PCONP |= (UINT32_C(1) << peripheral);
    }
    else {
        LPC_SC->PCONP &= ~(UINT32_C(1) << peripheral);
    }
    vPortExitCritical();
}

void lpc_pclk(lpc_pclk_t peripheral, clkdiv_t divider)
{
    /**
     * This is a quick and dirty trick to use uint64_t such that we don't have to
     * use if/else statements to pick either PCLKSEL0 or PCLKSEL1 register.
     */
    uint64_t *pclk_ptr = (uint64_t*)  &(LPC_SC->PCLKSEL0);
    const uint32_t bitpos = ((uint32_t)peripheral * 2);
    const uint32_t b11 = 3; // 0b11 in binary = 3

    vPortEnterCritical();
    *pclk_ptr &= ~((uint64_t)b11 << bitpos);
    *pclk_ptr |=  ((uint64_t)(divider & b11) << bitpos);
    vPortExitCritical();
}
