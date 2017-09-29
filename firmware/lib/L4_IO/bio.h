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
 *  @file
 *  @brief Provides the map of all Board Pins, and initializes all connected signals of the board
 *  @ingroup BoardIO
 *
 */
 
#ifndef BIO_H__
#define BIO_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "LPC17xx.h"



/** @{ Board Pin defines */
#define BIO_FLASH_CS_P0PIN      6   ///< P0.6
#define BIO_NORDIC_CS_P0PIN     16  ///< P0.16
#define BIO_NORDIC_IRQ_P0PIN    22  ///< P0.22
#define BIO_NORDIC_CE_P1PIN     24  ///< P1.24
#define BIO_LIGHT_ADC_CH_NUM    2   ///< ADC0.2
#define BIO_SD_CARD_CS_P1PIN    25  ///< P1.25
#define BIO_SD_CARD_CD_P1PIN    26  ///< P1.26

#define BIO_LED_PORT1_MASK      ((1UL<<0) | (1<<1)  | (1<<4)  | (1<<8))
#define BIO_SW_PORT1_MASK       ((1UL<<9) | (1<<10) | (1<<14) | (1<<15))
/** @} */



/** @{ Functions to chip-select, de-select devices and read pin signals */
static inline char board_io_flash_cs(void)  { LPC_GPIO0->FIOCLR = (1 << BIO_FLASH_CS_P0PIN);   return 1; }
static inline char board_io_flash_ds(void)  { LPC_GPIO0->FIOSET = (1 << BIO_FLASH_CS_P0PIN);   return 0; }
static inline char board_io_sd_cs(void)     { LPC_GPIO1->FIOCLR = (1 << BIO_SD_CARD_CS_P1PIN); return 1; }
static inline char board_io_sd_ds(void)     { LPC_GPIO1->FIOSET = (1 << BIO_SD_CARD_CS_P1PIN); return 0; }

static inline char board_io_nordic_cs(void)      { LPC_GPIO0->FIOCLR = (1 << BIO_NORDIC_CS_P0PIN); return 1; }
static inline char board_io_nordic_ds(void)      { LPC_GPIO0->FIOSET = (1 << BIO_NORDIC_CS_P0PIN); return 0; }
static inline char board_io_nordic_irq_sig(void) { return !!(LPC_GPIO0->FIOPIN & (1 << BIO_NORDIC_IRQ_P0PIN)); }
static inline char board_io_sd_card_cd_sig(void) { return !!(LPC_GPIO1->FIOPIN & (1 << BIO_SD_CARD_CD_P1PIN)); }
static inline void board_io_nordic_ce_high(void) { LPC_GPIO1->FIOSET = (1 << BIO_NORDIC_CE_P1PIN); }
static inline void board_io_nordic_ce_low (void) { LPC_GPIO1->FIOCLR = (1 << BIO_NORDIC_CE_P1PIN); }
/** @} */



/**
 * This initializes all the Board pins that are connected to peripherals.
 */
static inline void board_io_pins_initialize(void)
{
    /* Important to deselect all spi devices before configuring their direction*/
    board_io_flash_ds();
    board_io_sd_ds();
    board_io_nordic_ds();

    /* Initialize Uart0 pins on P0.2 and P0.3 */
    LPC_PINCON->PINSEL0 &= ~(0xF << 4);
    LPC_PINCON->PINSEL0 |=  (0x5 << 4);

    /* Initialize Flash CS on P0.6*/
    LPC_PINCON->PINSEL0 &= ~(0x3 << 12);
    LPC_GPIO0->FIODIR   |=  (1 << BIO_FLASH_CS_P0PIN);

    /* Initialize SSP1 on P0.7, P0.8, and P0.9 */
    LPC_PINCON->PINSEL0 &= ~(0x3F << 14);
    LPC_PINCON->PINSEL0 |=  (0x2A << 14);

    /* Initialize I2C2 pins on P0.10 and P0.11 */
    LPC_PINCON->PINSEL0 &= ~(0x0F << 20);
    LPC_PINCON->PINSEL0 |=  (0x0A << 20);

    /* Initialize SSP0 on P0.15, P0.17, and P0.18 */
    LPC_PINCON->PINSEL0 &= ~(0x3 << 30);
    LPC_PINCON->PINSEL0 |=  (0x2 << 30);
    LPC_PINCON->PINSEL1 &= ~(0xF << 2);
    LPC_PINCON->PINSEL1 |=  (0xA << 2);

    /* Initialize Nordic signals: P0.16: CS, P0.22: IRQ, P1.24: CE */
    LPC_PINCON->PINSEL1 &= ~(0x3 << 0);
    LPC_GPIO0->FIODIR   |=  (1 << BIO_NORDIC_CS_P0PIN);
    LPC_PINCON->PINSEL1 &= ~(0x3 << 12);
    LPC_GPIO0->FIODIR   &= ~(1 << BIO_NORDIC_IRQ_P0PIN);
    LPC_PINCON->PINSEL3 &= ~(0x3 << 16);
    LPC_GPIO1->FIODIR   |=  (1 << BIO_NORDIC_CE_P1PIN);

    /* Initialize light sensor ADC pin on P0.25 */
    LPC_PINCON->PINSEL1 &= ~(0x3 << 18);
    LPC_PINCON->PINSEL1 |=  (0x1 << 18);

    /* Initialize LED0-3 on P1.0, P1.1, P1.4, P1.8 */
    LPC_PINCON->PINSEL2 &= ~(0x3030F << 0);
    LPC_GPIO1->FIODIR   |=  ( BIO_LED_PORT1_MASK );

    /* Initialize SW0-3 on P1.9, P1.10, P1.14, P1.15 */
    LPC_PINCON->PINSEL2 &= ~(0xF << 18);
    LPC_PINCON->PINSEL2 &= ~(0xF << 28);
    LPC_GPIO1->FIODIR   &= ~( BIO_SW_PORT1_MASK );

    /* Initialize IR pin to be used as capture pin (not GPIO) */
    LPC_PINCON->PINSEL3 &= ~(0x3 << 4);
    LPC_PINCON->PINSEL3 |=  (0x3 << 4);

    /* Initialize SD card CS(P1.25) and CD (P1.26 - card detect) signals*/
    LPC_PINCON->PINSEL3 &= ~(0xF << 18);
    LPC_GPIO1->FIODIR |=  (1 << BIO_SD_CARD_CS_P1PIN);
    LPC_GPIO1->FIODIR &= ~(1 << BIO_SD_CARD_CD_P1PIN);
}



#ifdef __cplusplus
}
#endif
#endif /* BIO_H__ */
