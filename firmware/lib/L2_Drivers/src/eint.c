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

#include <stdlib.h>
#include "eint.h"
#include "FreeRTOS.h"
#include "lpc_isr.h"



/// Linked list structure of EINTs (External interrupts)
typedef struct eint3_entry {
    uint32_t pin_mask;          ///< Port pin's concatenated pin-mask
    void_func_t callback;       ///< Callback when interrupt occurs
    struct eint3_entry* next;   ///< The pointer to the next entry
} eint3_entry_t;

/**
 * @{ Linked list of each ports' rising and falling edge interrupts
 */
static eint3_entry_t *gp_port0_rising_list  = NULL;
static eint3_entry_t *gp_port0_falling_list = NULL;
static eint3_entry_t *gp_port2_rising_list  = NULL;
static eint3_entry_t *gp_port2_falling_list = NULL;
/** @} */



/**
 * Goes through the linked list to find out which interrupt triggered, and makes the callback
 * @param [in] isr_bits_ptr   The pointer to the variable that contains interrupt status
 * @param [in] int_clr_ptr    The pointer to the register to clear the real interrupt
 * @param [in[ list_head_ptr  The linked list head pointer of the configured interrupts
 *
 * @note isr_bits_ptr are cleared if the callback was made.  If the bits are not cleared then
 *       the interrupt was set, and no callback was found.  This shouldn't happen though :)
 */
static inline void handle_eint_list(uint32_t *isr_bits_ptr, volatile uint32_t *int_clr_ptr,
                                    eint3_entry_t *list_head_ptr)
{
    eint3_entry_t *e = list_head_ptr;

    /* Loop through our list to find which pin triggered this interrupt */
    while (e && *isr_bits_ptr) {
        /* If we find the pin, make the callback and clear the interrupt source*/
        if (e->pin_mask & *isr_bits_ptr) {
            (e->callback)();
            *isr_bits_ptr &= ~(e->pin_mask);
            *int_clr_ptr = e->pin_mask;
        }
        e = e->next;
    };
}

/// Actual ISR Handler (mapped to startup file's interrupt vector function name)
#ifdef __cplusplus
extern "C" {
#endif
void EINT3_IRQHandler(void)
{
    /* Read all the ports' rising and falling isr status */
    uint32_t p0_rising  = LPC_GPIOINT->IO0IntStatR;
    uint32_t p0_falling = LPC_GPIOINT->IO0IntStatF;
    uint32_t p2_rising  = LPC_GPIOINT->IO2IntStatR;
    uint32_t p2_falling = LPC_GPIOINT->IO2IntStatF;

    /* Go through each list to handle the ISR.
     * This will clear the interrupt by writing either to IO0IntClr or IO2IntClr
     */
    handle_eint_list(&p0_rising,  &(LPC_GPIOINT->IO0IntClr), gp_port0_rising_list);
    handle_eint_list(&p0_falling, &(LPC_GPIOINT->IO0IntClr), gp_port0_falling_list);
    handle_eint_list(&p2_rising,  &(LPC_GPIOINT->IO2IntClr), gp_port2_rising_list);
    handle_eint_list(&p2_falling, &(LPC_GPIOINT->IO2IntClr), gp_port2_falling_list);

    /* In case interrupt handler not attached correctly, clear all interrupts here */
    if (p0_rising || p0_falling) {
        LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF;
    }
    if (p2_rising || p2_falling) {
        LPC_GPIOINT->IO2IntClr = 0xFFFFFFFF;
    }
}
#ifdef __cplusplus
}
#endif



/**
 * Enables a port pin interrupt by inserting entry to the linked list, and writing to the
 * enable register to enable the interrupt.
 *
 * @param [in] pin_number     0-31
 * @param [in] type           The type of the interrupt (rising or falling)
 * @param [in] func           The callback function.
 * @param [in] list_head_ptr  The pointer of the linked list to add the interrupt configuration to.
 * @param [in] int_en_reg_ptr The pointer of CPU register to enable the interrupt
 */
static void eint3_enable(uint8_t pin_num, eint_intr_t type, void_func_t func,
                         eint3_entry_t **list_head_ptr, volatile uint32_t *int_en_reg_ptr)
{
    const uint32_t pin_mask = (UINT32_C(1) << pin_num);
    eint3_entry_t *e = NULL;

    if (0 != pin_mask && NULL != func && NULL != (e = malloc(sizeof(*e))) )
    {
        /* Insert new entry at the head of the list */
        e->callback = func;
        e->pin_mask = pin_mask;
        e->next = *list_head_ptr;
        *list_head_ptr = e;

        /* Enable the interrupt */
        *int_en_reg_ptr |= e->pin_mask;

        /* EINT3 shares pin interrupts with Port0 and Port2 */
        vTraceSetISRProperties(EINT3_IRQn, "EINT3", IP_eint);
        NVIC_EnableIRQ(EINT3_IRQn);
    }
}

void eint3_enable_port0(uint8_t pin_num, eint_intr_t type, void_func_t func)
{
    eint3_enable(pin_num, type, func,
                 (eint_rising_edge == type) ? &gp_port0_rising_list : &gp_port0_falling_list,
                 (eint_rising_edge == type) ? &(LPC_GPIOINT->IO0IntEnR) : &(LPC_GPIOINT->IO0IntEnF));
}

void eint3_enable_port2(uint8_t pin_num, eint_intr_t type, void_func_t func)
{
    eint3_enable(pin_num, type, func,
                 (eint_rising_edge == type) ? &gp_port2_rising_list : &gp_port2_falling_list,
                 (eint_rising_edge == type) ? &(LPC_GPIOINT->IO2IntEnR) : &(LPC_GPIOINT->IO2IntEnF));
}
