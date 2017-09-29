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
 * @brief Contains printf library to use inside ISR, or print data on heap.
 */
#ifndef PRINTF_LIB_H__
#define PRINTF_LIB_H__
#ifdef __cplusplus
extern "C" {
#endif



/**
 * Prints a debug statement.
 * This is same as printf(), but will ensure that all data sent over uart0 is flushed completely.
 * This solves the problem where one can printf() something immediately because it is possible that
 * regular printf() may queue the data and the system crashes before anything useful was printed.
 *
 * @param [in] format   The printf format string
 * @param [in] ...      The printf arguments
 */
int u0_dbg_printf(const char *format, ...);

/**
 * Prints a string to uart and returns after entire string is printed.
 * @param [in] string    NULL terminated string to print
 */
void u0_dbg_put(const char *string);

/**
 * Prints a formatted string on a heap pointer and returns it back.
 * @param [in] format   The printf format string
 * @param [in] ...      The printf arguments
 * @returns   The pointer to the printed string
 *
 * @warning   Use this function carefully as it can fragment memory on a system if used excessively.
 * @warning   Pointer obtained from this function must be freed.
 */
char* mprintf(const char *format, ...);



#ifdef __cplusplus
}
#endif
#endif
