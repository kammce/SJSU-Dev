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

#ifndef SPI_SEM_H__
#define SPI_SEM_H__
#ifdef __cplusplus
extern "C" {
#endif



/** @{
 * SPI Access should be locked in multi-tasking environment if you are using SPI BUS.
 * @warning Only use this API if you are running FreeRTOS and scheduler has started.
 */
void spi1_lock(void);    ///< Lock SPI access
void spi1_unlock(void);  ///< Unlock SPI access
/** @} */



#ifdef __cplusplus
}
#endif
#endif
