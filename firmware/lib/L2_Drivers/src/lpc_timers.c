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
#include "lpc_timers.h"
#include "LPC17xx.h"
#include "lpc_sys.h"



void lpc_timer_enable(const lpc_timer_t timer, const uint32_t us_per_tick)
{
    LPC_TIM_TypeDef *pTimerStruct = lpc_timer_get_struct(timer);
    const lpc_pconp_t pconp[] = { pconp_timer0, pconp_timer1, pconp_timer2, pconp_timer3 };
    const lpc_pclk_t  pclk[]  = { pclk_timer0,  pclk_timer1,  pclk_timer2,  pclk_timer3  };

    /* Power on the timer, and set the pclk = cpu clock (divide by 1) */
    lpc_pconp(pconp[timer], true);
    lpc_pclk (pclk [timer], clkdiv_1);

    /* Enable the timer, and increment on PCLK */
    pTimerStruct->TC = 0;
    pTimerStruct->TCR = 1;
    pTimerStruct->CTCR = 0;

    /* Set the resolution */
    pTimerStruct->PR = (sys_get_cpu_clock() / (1000*1000) * us_per_tick);
}

uint32_t lpc_timer_get_value(const lpc_timer_t timer)
{
    return (lpc_timer_get_struct(timer)->TC);
}

void lpc_timer_set_value(const lpc_timer_t timer, uint32_t value)
{
    lpc_timer_get_struct(timer)->TC = value;
}
