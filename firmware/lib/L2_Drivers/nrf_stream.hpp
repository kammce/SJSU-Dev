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
 * @brief Nordic stream
 * @ingroup Drivers
 */
#ifndef NRF_STREAM_HPP_
#define NRF_STREAM_HPP_

#include <stdint.h>
#include "wireless.h"
#include "char_dev.hpp"            // Base class
#include "singleton_template.hpp"  // Singleton Template



/**
 * Nordic char device driver
 * @ingroup Drivers
 *
 * This class provides a way to use the nordic like a char device.
 * You can use printf(), scanf() etc, and all the data will be sent to the
 * address set by setDestAddr().  If the destination address is not set, then
 * it will be sent to the last source that sent us data on nordic.
 *
 */
class NordicStream : public CharDev, public SingletonTemplate<NordicStream>
{
    public:
        /**
         * Set the destination address.
         * If address is set to zero, then the packets will be sent to the last
         * source that sent us the packet.
         */
        inline void setDestAddr(uint8_t address) { mDestAddr = address; }
        inline void setPktHops(uint8_t hops)     { mHops = hops;        }

        /// Flush all buffered data or send any pending data immediately.
        bool flush(void);

        /** @{ Virtual function overrides for the base class to work */
        bool getChar(char* pInputChar, unsigned int timeout=portMAX_DELAY);
        bool putChar(char out, unsigned int timeout=portMAX_DELAY);
        /** @} */

    private:
        typedef struct {
            mesh_packet_t pkt;  ///< actual wireless mesh packet
            uint8_t dataPtr;    ///< The data pointer of pkt.data[]
        } nrfPktBuffer_t;

        nrfPktBuffer_t mRxBuffer;   ///< The receive buffer
        nrfPktBuffer_t mTxBuffer;   ///< The transmit buffer
        uint8_t mDestAddr;          ///< The destination address
        uint8_t mHops;              ///< The hops to use for sending the data

        NordicStream();                                ///< Private constructor of this Singleton class
        friend class SingletonTemplate<NordicStream>;  ///< Friend class used for Singleton Template
};



#endif /* NRF_STREAM_HPP_ */
