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
#ifndef SPI1_H_
#define SPI1_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "LPC17xx.h"
#include "base/ssp_prv.h"



/**
 * Initializes SPI 1
 * Configures CLK, MISO, MOSI pins with a slow SCK speed
 */
static inline void ssp1_init(void)
{
    lpc_pconp(pconp_ssp1, true);
    lpc_pclk(pclk_ssp1, clkdiv_1);

    ssp_init(LPC_SSP1);

    void ssp1_dma_init();
    ssp1_dma_init();
}

/**
 * Sets SPI Clock speed
 * @param max_clock_mhz   The maximum speed of this SPI in Megahertz
 * @note The speed may be set lower to max_clock_mhz if it cannot be attained.
 */
static inline void ssp1_set_max_clock(unsigned int max_clock_mhz)
{
    ssp_set_max_clock(LPC_SSP1, max_clock_mhz);
}

/**
 * Exchanges a byte over SPI bus
 * @param out   The byte to send out
 * @returns     The byte received over SPI
 */
static inline char ssp1_exchange_byte(char out)
{
    return ssp_exchange_byte(LPC_SSP1, out);
}

/**
 * Writes a byte to the SPI FIFO
 * @warning YOU MUST ENSURE SPI IS NOT BUSY before calling this function
 */
static inline void ssp1_exchange_data(void* data, int len)
{
    ssp_exchange_data(LPC_SSP1, data, len);
}

/**
 * Transfers data over SPI (SSP#1)
 * @param pBuffer   The read or write buffer
 * @param num_bytes  The length of the transfer in bytes
 * @param is_write_op Non-zero for Write-operation, zero for read operation
 *
 * @note If is_write_op is true:
 *          - SPI data is sent from pBuffer
 *          - SPI data is copied from SSP DR to dummy buffer and discarded
 *       If is_write_op is false (read operation) :
 *          - SPI data is copied from SSP DR to pBuffer
 *          - 0xFF is sent out for each byte transfered
 *
 * @return 0 upon success, or non-zero upon failure.
 */
unsigned ssp1_dma_transfer_block(unsigned char* pBuffer, uint32_t num_bytes, char is_write_op);



#ifdef __cplusplus
}
#endif
#endif /* SPI1_H_ */
