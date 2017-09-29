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
 * @ingroup Utilities
 *
 * 04172012 : Initial
 */
#ifndef UTILITIES_H__
#define UTILITIES_H__
#ifdef __cplusplus
extern "C" {
#endif



/**
 * Delays in microseconds
 * @param micro_sec The delay in microseconds
 */
void delay_us(unsigned int micro_sec);

/**
 * Delays in milliseconds
 * @param milli_sec The delay in milliseconds.
 */
void delay_ms(unsigned int milli_sec);

/**
 * @returns non-zero result if FreeRTOS is running
 */
char is_freertos_running();

/**
 * Logs CPU boot information to the Storage
 * "boot.csv" is appended with the time this function is called
 */
void log_boot_info(const char*);


/**
 * Macro that can be used to print the timing/performance of a block
 * Example:
 * @code
 *      PRINT_EXECUTION_SPEED()
 *      {
 *          // ...
 *      }
 *      // At the end, the time taken between this block will be printed
 * @endcode
 */
#define PRINT_EXECUTION_SPEED() for(unsigned int __time=sys_get_uptime_us(); __time!=0; \
                                    printf("   Finished in %u us\n", (unsigned int)sys_get_uptime_us()-__time),__time=0)



#ifdef __cplusplus
}
#endif
#endif /* UTILITIES_H__ */
