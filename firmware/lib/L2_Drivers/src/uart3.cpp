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

#include "uart3.hpp"
#include "LPC17xx.h"     // LPC_UART3_BASE
#include "sys_config.h"  // sys_get_cpu_clock()



/**
 * IRQ Handler needs to be enclosed in extern "C" because this is C++ file, and
 * we don't want C++ to "mangle" our function name.
 * This ISR Function need needs to be named precisely to override "WEAK" ISR
 * handler defined at startup.cpp
 */
extern "C"
{
    void UART3_IRQHandler()
    {
        Uart3::getInstance().handleInterrupt();
    }
}

bool Uart3::init(unsigned int baudRate, int rxQSize, int txQSize)
{
    // Configure PINSEL for UART3.
    // UART3 RX/TX is at P4.28 and P4.29
    LPC_PINCON->PINSEL9 &= ~(0xF << 24); // Clear values
    LPC_PINCON->PINSEL9 |=  (0xF << 24); // Set values for UART3 Rx/Tx

    // Set UART3 Peripheral Clock divider to 1
    lpc_pclk(pclk_uart3, clkdiv_1);
    const unsigned int pclk = sys_get_cpu_clock();

    return UartDev::init(pclk, baudRate, rxQSize, txQSize);
}

Uart3::Uart3() : UartDev((unsigned int*)LPC_UART3_BASE)
{
    // Nothing to do here other than handing off LPC_UART3_Base address to UART_Base
}
