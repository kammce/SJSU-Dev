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

#include "uart2.hpp"
#include "LPC17xx.h"     // LPC_UART2_BASE
#include "sys_config.h"  // sys_get_cpu_clock()



/**
 * IRQ Handler needs to be enclosed in extern "C" because this is C++ file, and
 * we don't want C++ to "mangle" our function name.
 * This ISR Function need needs to be named precisely to override "WEAK" ISR
 * handler defined at startup.cpp
 */
extern "C"
{
    void UART2_IRQHandler()
    {
        Uart2::getInstance().handleInterrupt();
    }
}

bool Uart2::init(unsigned int baudRate, int rxQSize, int txQSize)
{
    // Configure PINSEL for UART2.
    // UART2 RX/TX is at P0.10 and P0.11 or P2.8 and P2.9
    // SJ One Board uses P2.8 and P2.9
    LPC_PINCON->PINSEL4 &= ~(0xF << 16); // Clear values
    LPC_PINCON->PINSEL4 |=  (0xA << 16); // Set values for UART2 Rx/Tx

    // Set UART2 Peripheral Clock divider to 1
    lpc_pclk(pclk_uart2, clkdiv_1);
    const unsigned int pclk = sys_get_cpu_clock();

    return UartDev::init(pclk, baudRate, rxQSize, txQSize);
}

Uart2::Uart2() : UartDev((unsigned int*)LPC_UART2_BASE)
{
    // Nothing to do here other than handing off LPC_UART2_Base address to UART_Base
}
