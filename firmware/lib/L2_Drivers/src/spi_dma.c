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

#include "LPC17xx.h"



#define SPI_DMA_TX_NUM      0    ///< DMA Channel number for SSP Tx
#define SPI_DMA_RX_NUM      1    ///< DMA Channel number for SSP Rx
#define SSP1_TX_CHAN        2UL  ///< DMA source for TX of SSP1
#define SSP1_RX_CHAN        3UL  ///< DMA source for RX of SSP1



#if !(SPI_DMA_TX_NUM>=0 && SPI_DMA_TX_NUM<=7)
#error "SPI_DMA_TX_NUM must be between 0 and 7"
#endif
#if !(SPI_DMA_RX_NUM>=0 && SPI_DMA_RX_NUM<=7)
#error "SPI_DMA_RX_NUM must be between 0 and 7"
#endif


enum {
    err_Dma = 0,
    err_Len = 1,
    err_busy = 2,
    err_spiFifo = 3
};


void ssp1_dma_init()
{
    // Power up and enable GPDMA
    lpc_pconp(pconp_gpdma, true);
    LPC_GPDMA->DMACConfig = 1;
    while (!(LPC_GPDMA->DMACConfig & 1));
}

unsigned ssp1_dma_transfer_block(unsigned char* pBuffer, uint32_t num_bytes, char is_write_op)
{
    uint8_t errorMask = 0;

    uint32_t dummyBuffer = 0xffffffff;
    LPC_GPDMACH_TypeDef *pDmaRxChannel = (LPC_GPDMACH_TypeDef *)
                                          (LPC_GPDMACH0_BASE + SPI_DMA_RX_NUM*0x20);
    LPC_GPDMACH_TypeDef *pDmaTxChannel = (LPC_GPDMACH_TypeDef *)
                                          (LPC_GPDMACH0_BASE + SPI_DMA_TX_NUM*0x20);

    // DMA is limited to 12-bit transfer size
    if(num_bytes >= 0x1000) {
        errorMask |= err_Len;
        return 1;
    }
    // DMA channels should not be busy
    if( (pDmaRxChannel->DMACCConfig & 1) || (pDmaTxChannel->DMACCConfig & 1) ) {
        errorMask |= err_busy;
        return 2;
    }
    while( LPC_SSP1->SR & (1<<2)) {
        errorMask |= err_spiFifo;
        char dummy = LPC_SSP1->DR;
        (void)dummy;
    }

    /**
     * TO DO : Optimize SSP1 DMA
     *  - Try setting source and destination burst size to 4
     *  - Try setting 16-bit SPI, and source and destination width to 1 WORD
     *
     * LPC_SSP1->CR0 : B3:B0. 0b0111 = 8-bit and 0b1111 = 16-bit
     * LPC_SSP1->CR0 |=  (1<<3);  // 16-bit
     * LPC_SSP1->CR0 &= ~(1<<3);  // 8-bit
     *
     * Bits of DMACCControl:
     * Transfer size: B11:B0
     * Source Burst Size: B14:B12   - 0:1, 1:4, 2:8, 3:16 bytes
     * Dest.  Burst Size: B17:B15   - 0:1, 1:4, 2:8, 3:16 bytes
     * Source Width: B20:B18        - 0:Byte, 1:Word, 2:DWORD
     * Dest.  Width: B23:B21        - 0:Byte, 1:Word, 2:DWORD
     * Source increment: B26
     * Destination increment: B27
     * Terminal count interrupt enable: B31
     *
     * Bits of DMACCConfig:
     * Enable: B0
     * Source Peripheral: B5:B1   (ignored if source is memory)
     * Dest.  Peripheral: B10:B6  (ignored if dest. is memory)
     * Transfer type: B13:B11 - See below
     * Interrupt Error Mask: B14
     * Terminal Count Interrupt : B15
     *
     * Bits for transfer type:
     * 000 - Memory to Memory
     * 001 - Memory to Peripheral
     * 010 - Peripheral to Memory
     * 011 - Source peripheral to destination peripheral
     */
    #define SRC_INCR_BIT        (1 << 26)
    #define DST_INCR_BIT        (1 << 27)
    #define TCIE_BIT            (1 << 31)   // Terminal count interrupt
    #define SRC_BURST_4_BYTES   (1 << 12)
    #define DST_BURST_4_BYTES   (1 << 15)
    #define SRC_WIDTH_2_BYTES   (1 << 18)
    #define DST_WIDTH_2_BYTES   (1 << 21)

    #define M_TO_P_BIT      (1 << 11)   // Memory to Peripheral
    #define P_TO_M_BIT      (2 << 11)   // Peripheral to Memory
    #define ER_INTR_BIT     (1 << 14)   // Error interrupt enable
    #define TC_INTR_BIT     (1 << 15)   // Terminal count interrupt enable

    /**
     * Clear existing terminal count and error interrupts otherwise
     * DMA will not start.
     */
    LPC_GPDMA->DMACIntTCClear = (1 << SPI_DMA_RX_NUM) | (1 << SPI_DMA_TX_NUM);
    LPC_GPDMA->DMACIntErrClr  = (1 << SPI_DMA_RX_NUM) | (1 << SPI_DMA_TX_NUM);

    /**
     * From SPI to buffer:
     * For write operation :
     *      - Receive data into dummy buffer
     *      - Don't increment destination
     * For read operation:
     *      - Read data into pBuffer
     *      - Increment destination
     */
    pDmaRxChannel->DMACCSrcAddr  = (uint32_t)(&(LPC_SSP1->DR));
    if(is_write_op) {
        pDmaRxChannel->DMACCDestAddr = (uint32_t)(&dummyBuffer);
        pDmaRxChannel->DMACCControl = num_bytes | TCIE_BIT;
    }
    else {
        pDmaRxChannel->DMACCDestAddr = (uint32_t)pBuffer;
        pDmaRxChannel->DMACCControl = num_bytes | DST_INCR_BIT | TCIE_BIT;
    }
    pDmaRxChannel->DMACCLLI = 0;
    pDmaRxChannel->DMACCConfig = (SSP1_RX_CHAN << 1) | P_TO_M_BIT;

    /**
     * From buffer to SPI :
     * For write operation :
     *      - Source data is pBuffer
     *      - Increment source data
     * For read operation:
     *      - Source data is buffer with 0xFF
     *      - Don't increment source data
     */
    if(is_write_op) {
        pDmaTxChannel->DMACCSrcAddr = (uint32_t)(pBuffer);
        pDmaTxChannel->DMACCControl = num_bytes | SRC_INCR_BIT;
    }
    else {
        pDmaTxChannel->DMACCSrcAddr = (uint32_t)(&dummyBuffer);
        pDmaTxChannel->DMACCControl = num_bytes;
    }
    pDmaTxChannel->DMACCDestAddr = (uint32_t)(&(LPC_SSP1->DR));
    pDmaTxChannel->DMACCLLI = 0;
    pDmaTxChannel->DMACCConfig = (SSP1_TX_CHAN << 6) | M_TO_P_BIT;

    /**
     * Channel must be fully configured and then enabled separately.
     * Setting DMACR's Rx/Tx bits should trigger the DMA
     */
    pDmaRxChannel->DMACCConfig |= 1;
    pDmaTxChannel->DMACCConfig |= 1;
    LPC_SSP1->DMACR |= 3; // RX: B0, TX: B1

    while( (pDmaRxChannel->DMACCControl & 0xfff) );
    LPC_SSP1->DMACR &= ~3;

    return 0;
}

