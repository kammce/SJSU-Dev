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
#ifndef SSP_PRIVATE_H__
#define SSP_PRIVATE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "LPC17xx.h"
#include "sys_config.h"



/**
 * Sets SSP Clock speed
 * @param max_clock_mhz   The maximum speed of this SPI in Megahertz
 * @note The speed may be set lower to max_clock_mhz if it cannot be attained.
 */
static inline void ssp_set_max_clock(LPC_SSP_TypeDef *pSSP, unsigned int max_clock_mhz)
{
    unsigned int divider = 2;
    const unsigned int cpuClockMhz = sys_get_cpu_clock() / (1000 * 1000UL);

    // Keep scaling down divider until calculated is higher
    // Example:
    // 60, need 18
    // 60/2   18 < 30 YES
    // 60/4   18 < 15 NO
    while(max_clock_mhz < (cpuClockMhz / divider) && divider <= 254)
    {
        divider += 2;
    }

    pSSP->CPSR = divider;
}

/**
 * Initializes SSP
 * Pins must be selected separately
 */
static inline void ssp_init(LPC_SSP_TypeDef *pSSP)
{
    pSSP->CR0 = 7;          // 8-bit mode
    pSSP->CR1 = (1 << 1);   // Enable SSP as Master
    ssp_set_max_clock(pSSP, 1);   // Set default speed
}



/**
 * Exchanges a byte over SPI bus
 * @param out   The byte to send out
 * @returns     The byte received over SPI
 */
static inline char ssp_exchange_byte(LPC_SSP_TypeDef *pSSP, char out)
{
    pSSP->DR = out;
    while(pSSP->SR & (1 << 4)); // Wait until SSP is busy
    return pSSP->DR;
}

/**
 * Writes a byte to the SPI FIFO
 * @warning YOU MUST ENSURE SPI IS NOT BUSY before calling this function
 */
static inline void ssp_exchange_data(LPC_SSP_TypeDef *pSSP, void* data, int len)
{
    const uint32_t rx_fifo_half_full_bitmask = (1 << 2);
    const uint32_t spi_busy_bitmask = (1 << 4);
    const int spi_fifo_size = 8;
    const int spi_half_fifo_size = spi_fifo_size / 2;

    char *dataOut = (char*)data;
    char *dataIn = (char*)data;

    while (len > 0) {
        if (len >= spi_fifo_size) {
            pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++;
            pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++;

            /* Pick up half the transmitted bytes as soon as RX fifo is half full */
            len -= spi_fifo_size;
            while (!(pSSP->RIS & rx_fifo_half_full_bitmask));
            *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR;

            /* Pick up the rest of the half after SSP is fully done */
            while(pSSP->SR & spi_busy_bitmask);
            *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR;
        }
        else if (len >= spi_half_fifo_size) {
            pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++; pSSP->DR = *dataOut++;
            len -= spi_half_fifo_size;
            while(pSSP->SR & spi_busy_bitmask);
            *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR; *dataIn++ = pSSP->DR;
        }
        else {
            pSSP->DR = *dataOut++;
            --len;
            while(pSSP->SR & spi_busy_bitmask);
            *dataIn++ = pSSP->DR;
        }
    }
}



#ifdef __cplusplus
}
#endif
#endif /* SSP_PRIVATE_H__ */
