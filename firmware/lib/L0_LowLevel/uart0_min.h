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
 * @brief This file provides minimal UART0 implementation
 * 			The getchar and putchar functions provide polling version of data I/O
 */
#ifndef UART0_MIN_H_
#define UART0_MIN_H_
#ifdef __cplusplus
extern "C" {
#endif



/**
 * Initializes UART0 of LPC17xx
 * @param baud_rate  The baud rate to set
 */
void uart0_init(unsigned int baud_rate);

/**
 * @param unused Unused parameter to comply with char function pointer.
 * @returns character received over UART0
 */
char uart0_getchar(char unused);

/**
 * @param out   The character to output over UART0
 * @returns     Always 1.
 */
char uart0_putchar(char out);

/**
 * outputs a string using uart0_putchar()
 * @param c_string	The pointer to null terminated string
 */
void uart0_puts(const char* c_string);



#ifdef __cplusplus
}
#endif
#endif /* UART0_MIN_H_ */
