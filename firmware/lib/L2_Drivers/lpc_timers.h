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
 * This API provides a means to start a simple timer, and get/set the timer value
 * 20140610: Initial
 */
#ifndef LPC_TIMERS_H__
#define LPC_TIMERS_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "LPC17xx.h"



/**
 * The type of timers supported by LPC17xx
 * Note that for the SJ-One project, lpc_timer1 is being used by lpc_sys.c
 * to carry out several background services and capture IR receiver signals.
 */
typedef enum {
    lpc_timer0,
    lpc_timer1,
    lpc_timer2,
    lpc_timer3,
} lpc_timer_t;

/**
 * @returns the timer structure pointer based on the timer parameter
 */
static inline LPC_TIM_TypeDef* lpc_timer_get_struct(const lpc_timer_t timer)
{
    const uint32_t timer_mem_bases[] = { LPC_TIM0_BASE, LPC_TIM1_BASE, LPC_TIM2_BASE, LPC_TIM3_BASE };
    return (LPC_TIM_TypeDef*) timer_mem_bases[timer];
}

/**
 * @returns the timer ISR number based ont he timer parameter
 */
static inline IRQn_Type lpc_timer_get_irq_num(const lpc_timer_t timer)
{
    const IRQn_Type irqs[] = { TIMER0_IRQn, TIMER1_IRQn, TIMER2_IRQn, TIMER3_IRQn };
    return irqs[timer];
}

/**
 * Enables and starts the timer with the given tick rate
 * @param [in] timer        The timer type
 * @param [in] us_per_tick  The resolution of each tick
 *
 * @note The logic requires the CPU to be running at greater than or equal to 1Mhz
 * @post The timer's clock divider is set to (CPU clock)/1
 */
void lpc_timer_enable(const lpc_timer_t timer, const uint32_t us_per_tick);

/**
 * Get the value of the timer
 * @param [in] timer    The timer type
 * @returns             The value of the timer
 */
uint32_t lpc_timer_get_value(const lpc_timer_t timer);

/**
 * Set the value of the timer; you normally shouldn't do this as the hardware
 * is ticking the timer in the background but you can use it to reset it to zero.
 * @param [in] timer    The timer type
 * @param [in] value    The value to set.
 */
void lpc_timer_set_value(const lpc_timer_t timer, const uint32_t value);



#ifdef __cplusplus
}
#endif
#endif /* LPC_TIMERS_H__ */
