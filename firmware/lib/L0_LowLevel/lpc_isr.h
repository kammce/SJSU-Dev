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
 * @brief Interrupt Service Routine (ISR) functions
 */
#ifndef _SYS_ISRS_H__
#define _SYS_ISRS_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "LPC17xx.h"



/**
 * Register a callback function for an interrupt.
 *
 * There are two ways to register an interrupt, one is to define the exact "IRQHandler" as
 * the one defined at startup.cpp, so something like this:
 *    C++ (*.cpp) file: extern "C" void UART0_IRQHandler() {}
 *    C   (*.c)   file: void UART0_IRQHandler() { }
 *
 * The second method is just to call this function and register your function as an ISR:
 * @param [in] num            The IRQ number; @see IRQn_Type.  Only IRQ 0-N can be registered.
 * @param [in] isr_func_ptr   void function name.
 *
 * @note This function is implemented at startup.cpp since that contains the interrupt vector.
 */
void isr_register(IRQn_Type num, void (*isr_func_ptr) (void));

/**
 * Interrupt Priorities (pre-configured by low_level_init.cpp)
 *  0 - Highest
 * 31 - Lowest
 *
 * Interrupts used in this project must NOT use higher priority than IP_SYSCALL, so
 * if IP_SYSCALL is set to 5, then interrupt priority must be higher number than 5
 * in order to set it to lower priority.
 *
 * FromISR() FreeRTOS API cannot be called from interrupts that have higher priority
 * than IP_SYSCALL, which means that:
 *  - YOUR ISR must have numerically equal to or HIGHER value than IP_SYSCALL
 *
 * The interrupts that use higher priority than IP_SYSCALL MUST NOT USE FREERTOS API.
 * These higher priority interrupts (even higher than FreeRTOS kernel) can be used
 * to specify "super" priority above everything else in the system.
 */
typedef enum
{
    /**
     * @{ FreeRTOS Interrupt Priorities
     * Be careful changing these because interrupt priorities set below
     * must fall in between these two priorities.
     */
        IP_above_freertos = 1, /* Do not use FreeRTOS API with this priority! */
        IP_SYSCALL = 2,        /* Do not use FreeRTOS API for ISR priorities below this */
        IP_KERNEL = 31,        /* Must be the lowest priority in the system */
    /** @} */

    /**
     * @{ Interrupt Priorities
     * These are used at to set default priorities before main() is called.
     * All interrupts use default priority unless otherwise stated in this enum.
     *
     * If you don't want interrupts to nest, set them to the same priority as IP_DEFAULT
     */
        /* Name the default, high and low priorities */
        IP_default = 20,                /**< Default priority of most interrupts, set arbitarily betweeen syscall and kernel */
        IP_high    = IP_SYSCALL + 1,    /**< Higher than default, but lower than syscall */
        IP_low     = IP_default + 1,    /**< Lower than default */

        /* Suggested interrupt priorities for commonly used peripherals */
        IP_eint = IP_default - 9, /**< Port0 and Port2 interrupt */

        IP_ssp  = IP_default - 6, /**< SSP can be super fast, so needs higher priority */
        IP_can  = IP_default - 5, /**< CAN can be fast, so use higher priority than other communication BUSes */

        IP_i2c  = IP_default - 2, /**< I2C set to higher priority than UART */
        IP_uart = IP_default - 1, /**< UART set to higher priority than default */

        /* Rest of the interrupts probably don't need a fast response so set them
         * to default priority.  You don't want to overcomplicate a system by
         * changing too many priorities unless absolutely needed.
         */
        IP_watchdog = IP_default,
        IP_timers = IP_default,
        IP_pwm1 = IP_default,
        IP_pll = IP_default,
        IP_spi = IP_default,
        IP_rtc = IP_default,
        IP_adc = IP_default,
        IP_bod = IP_default,
        IP_usb = IP_default,
        IP_dma = IP_default,
        IP_i2s = IP_default,
        IP_enet = IP_default,
        IP_mcpwm = IP_default,
        IP_qei    = IP_default,
        IP_RIT    = IP_default,
        IP_pll1   = IP_default,
        IP_usbact = IP_default,
        IP_canact = IP_default,
    /** @} */
} intr_priorities_t;



#ifdef __cplusplus
}
#endif
#endif
