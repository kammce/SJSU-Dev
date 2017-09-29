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
 * @brief UART2 Interrupt driven IO driver
 * @ingroup Drivers
 */
#ifndef UART2_HPP__
#define UART2_HPP__

#include "uart_dev.hpp"            // Base class
#include "singleton_template.hpp"  // Singleton Template



/**
 * UART2 Interrupt Driven Driver
 * This simply uses UartDev, and forwards the UART3 IRQ to UartDev::handleInterrupt()
 * This enforces Singleton class, otherwise it's a thin wrapper around UartDev class.
 *
 * @ingroup Drivers
 */
class Uart2 : public UartDev, public SingletonTemplate<Uart2>
{
    public:
        /**
         * Initializes UART2 at the given @param baudRate
         * @param rxQSize   The size of the receive queue  (optional, defaults to 32)
         * @param txQSize   The size of the transmit queue (optional, defaults to 64)
         */
        bool init(unsigned int baudRate, int rxQSize=32, int txQSize=64);

    private:
        Uart2();  ///< Private constructor of this Singleton class
        friend class SingletonTemplate<Uart2>;  ///< Friend class used for Singleton Template
};


#endif /* UART2_HPP__ */
