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
 * @brief Provides UART Base class functionality for UART peripherals
 *
 *  12012013 : Split functionality to char_dev.hpp and inherited this object
 *  10102013 : Make init() public, and protect from re-init leaking memory through xQueueCreate()
 *  05122013 : Added version history
 */
#ifndef UART_DEV_HPP_
#define UART_DEV_HPP_

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "char_dev.hpp"
#include "LPC17xx.h"



/**
 * UART Base class that can be used to write drivers for all UART peripherals.
 * Steps needed to write a UART driver:
 *  - Inherit this class
 *  - Call init() and configure PINSEL to select your UART pins
 *  - When your UART(#) hardware interrupt occurs, call handleInterrupt()
 *
 *  To connect UART Interrupt with your UART, reference this example:
 *  @code
 *  extern "C"
*   {
 *      void UART0_IRQHandler()
 *      {
 *          UART0::getInstance().handleInterrupt();
 *      }
 *   }
 *  @endcode
 *
 *  @warning This class hasn't been tested for UART1 due to different memory map.
 *  @ingroup Drivers
 */
class UartDev : public CharDev
{
    public:

        /// Reset the baud-rate after UART has been initialized
        void setBaudRate(unsigned int baudRate);

        /**
         * @returns a character from the UART input
         * @param   pInputChar  The pointer to input char to store received character
         * @param   timeout Optional parameter which defaults to maximum value that
         *          will allow you to wait forever for a character to be received
         * @returns true if a character was obtained within the given timeout
         */
        bool getChar(char* pInputChar, unsigned int timeout=portMAX_DELAY);

        /**
         * Outputs a char given by @param out
         * @param   timeout Optional parameter which defaults to maximum value that
         *          will allow you to wait forever for a character to be sent
         * @returns true if the output char was successfully written to Queue, or
         *          false if the output queue was full within the given timeout
         */
        bool putChar(char out, unsigned int timeout=portMAX_DELAY);

        /// Flushed all pending transmission of the uart queue
        bool flush(void);

        /**
         * @{ Get the Rx and Tx queue information
         * Watermarks provide the queue's usage to access the capacity usage
         */
        inline unsigned int getRxQueueSize() const { return uxQueueMessagesWaiting(mRxQueue); }
        inline unsigned int getTxQueueSize() const { return uxQueueMessagesWaiting(mTxQueue); }
        inline unsigned int getRxQueueWatermark() const { return mRxQWatermark; }
        inline unsigned int getTxQueueWatermark() const { return mTxQWatermark; }
        /** @} */

        /**
         * @{ Recent activity api
         * Check to see if this UART received any Rx/Tx activity within the last #X OS ticks.
         * @param ms Optional Parameter: Defaults to 3000 milliseconds
         */
        bool recentlyActive(unsigned int ms=3000) const;
        inline TickType_t getLastActivityTime(void) const { return mLastActivityTime; }
        inline void resetActivity(void) { mLastActivityTime = xTaskGetMsCount(); }
        /** @} */

        /**
         * When the UART interrupt occurs, this function should be called to handle
         * future action to take due to the interrupt cause.
         */
        void handleInterrupt();

    protected:
        /**
         * Initializes the UART register including Queues, baudrate and hardware.
         * Parent class should call this method before initializing Pin-Connect-Block
         * @param pclk      The system peripheral clock for this UART
         * @param baudRate  The baud rate to set
         * @param rxQSize   The receive queue size
         * @param txQSize   The transmit queue size
         * @post    Sets 8-bit mode, no parity, no flow control.
         * @warning This will not initialize the PINS, so user needs to do pin
         *          selection because LPC's same UART hardware, such as UART2
         *          is available on multiple pins.
         * @note If the txQSize is too small, functions performing printf will start to block.
         */
        bool init(unsigned int pclk, unsigned int baudRate, int rxQSize=32, int txQSize=32);

        /**
         * Protected constructor that requires parent class to provide UART's
         * base register address for which to operate this UART driver
         */
        UartDev(unsigned int* pUARTBaseAddr);
        ~UartDev() { } /** Nothing to clean up */

    private:
        UartDev(); /** Disallowed constructor */

        LPC_UART_TypeDef* mpUARTRegBase;///< Pointer to UART's memory map
        QueueHandle_t mRxQueue;         ///< Queue for UARTs receive buffer
        QueueHandle_t mTxQueue;         ///< Queue for UARTs transmit buffer
        uint32_t mPeripheralClock;      ///< Peripheral clock as given by constructor
        uint16_t mRxQWatermark;         ///< Watermark of Rx Queue
        uint16_t mTxQWatermark;         ///< Watermark of Tx Queue
        TickType_t mLastActivityTime;   ///< updated each time last rx interrupt occurs
};



#endif /* UART_DEV_HPP_ */
