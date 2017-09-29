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
#ifndef SPI0_H__
#define SPI0_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "LPC17xx.h"
#include "sys_config.h"
#include "base/ssp_prv.h"



/**
 * Initializes SPI 1
 * Configures CLK, MISO, MOSI pins with a slow SCK speed
 */
static inline void ssp0_init(unsigned int max_clock_mhz)
{
    // @note Pins are initialized by bio.h
    lpc_pconp(pconp_ssp0, true);
    lpc_pclk(pclk_ssp0, clkdiv_1);
    ssp_init(LPC_SSP0);
}

/**
 * Sets SPI Clock speed
 * @pre   The SPI clock must be CPU clock (CPU clock must not be divided into this peripheral)
 * @param max_clock_mhz   The maximum speed of this SPI in megahertz
 * @note  The speed may be set lower to max_clock_mhz if it cannot be attained.
 */
static inline void ssp0_set_max_clock(unsigned int max_clock_mhz)
{
    ssp_set_max_clock(LPC_SSP0, max_clock_mhz);
}


/**
 * Exchanges a byte over SPI bus
 * @param out   The byte to send out
 * @returns     The byte received over SPI
 */
static inline char ssp0_exchange_byte(char out)
{
    return ssp_exchange_byte(LPC_SSP0, out);
}

/**
 * Exchanges multi-byte data over SPI Bus
 */
static inline void ssp0_exchange_data(void *data, int len)
{
    ssp_exchange_data(LPC_SSP0, data, len);
}



#ifdef __cplusplus
}
#endif
#endif /* SPI0_H__ */
