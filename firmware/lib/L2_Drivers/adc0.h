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
 * 20131202 : Enclosed adc conversion inside critical section
 * 20131101 : Fix possible divide by zero.  i was set to 0 during loop init
 */
#ifndef ADC0_H_
#define ADC0_H_
#ifdef __cplusplus
extern "C" {
#endif



/**
 * Initializes the ADC Peripheral
 * @note The PIN that will be used by ADC needs to be selected using PINSEL externally
 */
void adc0_init(void);

/**
 * Gets an ADC reading from a channel number between 0 - 7
 * @returns 12-bit ADC value read from the ADC.
 * @note If FreeRTOS is running, adc conversion will use interrupts and not poll for the result.
 */
uint16_t adc0_get_reading(uint8_t channel_num);



#ifdef __cplusplus
}
#endif
#endif /* ADC0_H_ */
