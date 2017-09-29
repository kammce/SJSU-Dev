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
 */
#ifndef EINT_H__
#define EINT_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "lpc_sys.h"



/// The type of the interrupt for the port pin.
typedef enum {
    eint_rising_edge,  ///< Interrupt on rising edge
    eint_falling_edge  ///< Interrupt on falling edge
} eint_intr_t;

/**
 * Enables the callback when the interrupt occurs.  The entry added last is checked
 * first if multiple interrupts occur at the same time.  Each call will allocate
 * 16 bytes for the interrupt service entry.
 * @note EINT3 shares interrupt with Port0 and Port2
 * @param [in] pin_num  The pin number from 0-31.
 * @param [in] type     The type of interrupt.
 * @param [in] func     The callback function.
 */
void eint3_enable_port0(uint8_t pin_num, eint_intr_t type, void_func_t func);

/// @copydoc eint3_enable_port0()
void eint3_enable_port2(uint8_t pin_num, eint_intr_t type, void_func_t func);



#ifdef __cplusplus
}
#endif
#endif /* EINT_H__ */
