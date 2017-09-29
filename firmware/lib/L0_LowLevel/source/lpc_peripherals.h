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
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY.  IT IS AUTOMATICALLY INCLUDED WHEN YOU
 * INCLUDE LPC17xx.h
 */
#ifndef LPC_PERIPHERALS_H__
#define LPC_PERIPHERALS_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>



/// This enumeration matches the PCONP register numbers for peripherals
typedef enum {
    pconp_timer0 = 1,
    pconp_timer1 = 2,
    pconp_uart0  = 3,
    pconp_uart1  = 4,

    pconp_pwm1  = 6,
    pconp_i2c0  = 7,
    pconp_spi   = 8,
    pconp_rtc   = 9,
    pconp_ssp1  = 10,

    pconp_adc  = 12,
    pconp_can1 = 13,
    pconp_can2 = 14,

    pconp_gpio  = 15,
    pconp_rit   = 16,
    pconp_mcpwm = 17,
    pconp_qei   = 18,
    pconp_i2c1  = 19,

    pconp_ssp0   = 21,
    pconp_timer2 = 22,
    pconp_timer3 = 23,
    pconp_uart2  = 24,
    pconp_uart3  = 25,
    pconp_i2c2   = 26,
    pconp_i2s    = 27,
    pconp_gpdma  = 29,
    pconp_enet   = 30,
    pconp_usb    = 31,
} lpc_pconp_t;

/// This enumeration matches peripheral clock registers (PCLKSEL0 and PCLKSEL1)
typedef enum {
    pclk_watchdog = 0,
    pclk_timer0   = 1,
    pclk_timer1   = 2,
    pclk_uart0    = 3,
    pclk_uart1    = 4,

    pclk_pwm1 = 6,
    pclk_i2c0 = 7,
    pclk_spi  = 8,

    pclk_ssp1    = 10,
    pclk_dac     = 11,
    pclk_adc     = 12,
    pclk_can1    = 13,
    pclk_can2    = 14,
    pclk_can_flt = 15,

    pclk_qei     = 16,
    pclk_gpioint = 17,
    pclk_pcb     = 18,
    pclk_i2c1    = 19,

    pclk_ssp0   = 21,
    pclk_timer2 = 22,
    pclk_timer3 = 23,
    pclk_uart2  = 24,
    pclk_uart3  = 25,
    pclk_i2c2   = 26,

    pclk_rit    = 29,
    pclk_syscon = 30,
    pclk_mc     = 31,
} lpc_pclk_t;

/// This enumeration matches the clock divider
typedef enum {
    clkdiv_4 = 0,
    clkdiv_1 = 1,
    clkdiv_2 = 2,
    clkdiv_8 = 3,
} clkdiv_t;

/**
 * Powers ON or powers OFF the peripheral
 * @param peripheral  The peripheral type.  @see lpc_pconp_t
 * @param on  If true, peripheral is turned on, otherwise turned off
 */
void lpc_pconp(lpc_pconp_t peripheral, bool on);
/**
 * Sets the peripheral clock divider.
 * @param peripheral  The peripheral type.  @see lpc_pclk_t
 * @param divider     The divider type
 * @note The reset value for all peripherals is clkdiv_4
 */
void lpc_pclk(lpc_pclk_t peripheral, clkdiv_t divider);



#ifdef __cplusplus
}
#endif
#endif /* LPC_PERIPHERALS_H__ */
