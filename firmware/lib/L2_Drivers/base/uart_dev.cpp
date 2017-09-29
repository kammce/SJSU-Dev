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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "uart_dev.hpp"
#include "LPC17xx.h"
#include "utilities.h"      // system_get_timer_ms();
#include "lpc_sys.h"



bool UartDev::getChar(char* pInputChar, unsigned int timeout)
{
    if (!pInputChar || !mRxQueue) {
        return false;
    }
    else if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        return xQueueReceive(mRxQueue, pInputChar, timeout);
    }
    else {
        unsigned int timeout_of_char = sys_get_uptime_ms() + timeout;
        while (! xQueueReceive(mRxQueue, pInputChar, 0)) {
            if (sys_get_uptime_ms() > timeout_of_char) {
                return false;
            }
        }
    }
    return true;
}

bool UartDev::putChar(char out, unsigned int timeout)
{
    /* If OS not running, just send data using polling and return */
    if (taskSCHEDULER_RUNNING != xTaskGetSchedulerState()) {
        mpUARTRegBase->THR = out;
        while(! (mpUARTRegBase->LSR & (1 << 6)));
        return true;
    }

    /* FreeRTOS running, so send to queue and if queue is full, return false */
    if(! xQueueSend(mTxQueue, &out, timeout)) {
        return false;
    }

    /* If transmitter is not busy, send out the oldest char from the queue,
     * and let the transmitter empty interrupt empty out the queue thereafter.
     */
    const int uart_tx_is_idle = (1 << 6);
    if (mpUARTRegBase->LSR & uart_tx_is_idle)
    {
        if (xQueueReceive(mTxQueue, &out, 0)) {
            mpUARTRegBase->THR = out;
        }
    }

    return true;
}

bool UartDev::flush(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        while (getTxQueueSize() > 0) {
            vTaskDelay(1);
        }
    }

    return true;
}

bool UartDev::recentlyActive(unsigned int ms) const
{
    TickType_t lastTimeStampMs = MS_PER_TICK() * mLastActivityTime;
    TickType_t currentTimeMs = xTaskGetMsCount();
    return (currentTimeMs - lastTimeStampMs) < ms;
}

void UartDev::setBaudRate(unsigned int baudRate)
{
    /* Adding 0.5 to perform rounding correctly since we do not want
     * 1.9 to round down to 1, but we want it to round-up to 2.
     */

    mpUARTRegBase->LCR = (1 << 7); // Enable DLAB to access DLM, DLL, and IER
    {
        uint16_t bd = (mPeripheralClock / (16 * baudRate)) + 0.5;
        mpUARTRegBase->DLM = (bd >> 8);
        mpUARTRegBase->DLL = (bd >> 0);
    }
    mpUARTRegBase->LCR = 3; // Disable DLAB and set 8bit per char
}

void UartDev::handleInterrupt()
{
    /**
     * Bit Masks of IIR register Bits 3:1 that contain interrupt reason.
     * Bits are shifted left because reasonForInterrupt contains Bits 3:0
     */
    const uint16_t transmitterEmpty = (1 << 1);
    const uint16_t dataAvailable    = (2 << 1);
    const uint16_t dataTimeout      = (6 << 1);

    long higherPriorityTaskWoken = 0;
    long switchRequired = 0;
    char c = 0;
    unsigned charsSent = 0;

    uint16_t reasonForInterrupt = (mpUARTRegBase->IIR & 0xE);
    {
        /**
         * If multiple sources of interrupt arise, let this interrupt exit, and re-enter
         * for the new source of interrupt.
         */
        switch (reasonForInterrupt)
        {
            case transmitterEmpty:
            {
                if(uxQueueMessagesWaitingFromISR(mTxQueue) > mTxQWatermark) {
                    mTxQWatermark = uxQueueMessagesWaitingFromISR(mTxQueue);
                }

                /**
                 * When THRE (Transmit Holding Register Empty) interrupt occurs,
                 * we can send as many bytes as the hardware FIFO supports (16)
                 */
                const unsigned char hwTxFifoSize = 16;
                for(charsSent=0;
                        charsSent < hwTxFifoSize && xQueueReceiveFromISR(mTxQueue, &c, &higherPriorityTaskWoken);
                        charsSent++)
                {
                    mpUARTRegBase->THR = c;
                    if(higherPriorityTaskWoken) {
                        switchRequired = 1;
                    }
                }
            }
            break;

            case dataAvailable:
            case dataTimeout:
            {
                mLastActivityTime = xTaskGetTickCountFromISR();
                /**
                 * While receive Hardware FIFO not empty, keep queuing the data.
                 * Even if xQueueSendFromISR() Fails (Queue is full), we still need to
                 * read RBR register otherwise interrupt will not clear
                 */
                while (0 != (mpUARTRegBase->LSR & (1 << 0)))
                {
                    c = mpUARTRegBase->RBR;
                    xQueueSendFromISR(mRxQueue, &c, &higherPriorityTaskWoken);
                    if(higherPriorityTaskWoken) {
                        switchRequired = 1;
                    }
                }

                if(uxQueueMessagesWaitingFromISR(mRxQueue) > mRxQWatermark) {
                    mRxQWatermark = uxQueueMessagesWaitingFromISR(mRxQueue);
                }
            }
            break;

            default:
                /* Read LSR register to clear Line Status Interrupt */
                reasonForInterrupt = mpUARTRegBase->LSR;
                break;
        }
    }

    portEND_SWITCHING_ISR(switchRequired);
}

///////////////
// Protected //
///////////////
UartDev::UartDev(unsigned int* pUARTBaseAddr) : CharDev(),
        mpUARTRegBase((LPC_UART_TypeDef*) pUARTBaseAddr),
        mRxQueue(0),
        mTxQueue(0),
        mPeripheralClock(0),
        mRxQWatermark(0),
        mTxQWatermark(0),
        mLastActivityTime(0)
{

}

bool UartDev::init(unsigned int pclk, unsigned int baudRate,
                   int rxQSize, int txQSize)
{
    mPeripheralClock = pclk;

    // Configure UART Hardware: Baud rate, FIFOs etc.
    if (LPC_UART0_BASE == (unsigned int) mpUARTRegBase)
    {
        lpc_pconp(pconp_uart0, true);
        vTraceSetISRProperties(UART0_IRQn, "U0", IP_uart);
        NVIC_EnableIRQ(UART0_IRQn);
    }
    /*
     else if(LPC_UART1_BASE == (unsigned int)mpUARTRegBase)
     {
     lpc_pconp(pconp_uart1, true);
     NVIC_EnableIRQ(UART1_IRQn);
     }
     */
    else if (LPC_UART2_BASE == (unsigned int) mpUARTRegBase)
    {
        lpc_pconp(pconp_uart2, true);
        vTraceSetISRProperties(UART2_IRQn, "U2", IP_uart);
        NVIC_EnableIRQ(UART2_IRQn);
    }
    else if (LPC_UART3_BASE == (unsigned int) mpUARTRegBase)
    {
        lpc_pconp(pconp_uart3, true);
        vTraceSetISRProperties(UART3_IRQn, "U3", IP_uart);
        NVIC_EnableIRQ(UART3_IRQn);
    }
    else
    {
        return false;
    }

    // Enable & Reset FIFOs and set 4 char timeout for Rx
    mpUARTRegBase->FCR = (1 << 0) | (1 << 6);
    mpUARTRegBase->FCR |= (1 << 1) | (1 << 2);

    setBaudRate(baudRate);

    // Set minimum queue size?
    if (rxQSize < 9) rxQSize = 8;
    if (txQSize < 9) txQSize = 8;

    // Create the receive and transmit queues
    if (!mRxQueue) mRxQueue = xQueueCreate(rxQSize, sizeof(char));
    if (!mTxQueue) mTxQueue = xQueueCreate(txQSize, sizeof(char));

    // Optional: Provide names of the FreeRTOS objects for the Trace Facility
    vTraceSetQueueName(mRxQueue, "UART RX-Q");
    vTraceSetQueueName(mTxQueue, "UART TX-Q");

    // Enable Rx/Tx and line status Interrupts:
    mpUARTRegBase->IER = (1 << 0) | (1 << 1) | (1 << 2); // B0:Rx, B1: Tx

    return (0 != mRxQueue && 0 != mTxQueue);
}
