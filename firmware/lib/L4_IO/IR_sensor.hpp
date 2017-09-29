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
 * @ingroup BoardIO
 */
 
#ifndef IR_SENSOR_HPP_
#define IR_SENSOR_HPP_
#include <stdint.h>



/**
 * IR Sensor class used to get signals from the on-board IR Sensor
 * This sensor can decode a remote's IR signals, such as a TV remote control.
 *
 * To make sure no decoded signals are over-written, or you do not get a decoded signal that is very
 * obsolete, the user should periodically check isIRCodeReceived() every 50ms, and buffer signals externally.
 *
 * If a signal is decoded, getLastIRCode() will provide it, and clear out the buffer to receive future signal.
 * If getLastIRCode() is not called regularly, it is possible that this may return an older signal, even if
 * it was many hours ago.
 *
 * @ingroup BoardIO
 */
class IR_Sensor : public SingletonTemplate<IR_Sensor>
{
    public:
        bool init(); ///< Initializes this device, @returns true if successful

        /// @returns true if an IR signal was decoded and is available to read
        bool isIRCodeReceived();

        /// @returns The last IR signal that was decoded or 0 if nothing was decoded
        uint32_t getLastIRCode();

        /** @{ Don't use these functions */
        void storeIrCode(uint32_t);
        void decodeIrCode(void);
        /** @} */

    private:
        /// Private constructor of this Singleton class
        IR_Sensor() {}
        friend class SingletonTemplate<IR_Sensor>;  ///< Friend class used for Singleton Template
};

#endif /* IR_SENSOR_HPP_ */
