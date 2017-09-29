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
 
#ifndef LED_DISPLAY_HPP__
#define LED_DISPLAY_HPP__
#include "i2c2_device.hpp"  // I2C Device Base class


/**
 * LED Display class to manipulate the on-board 2 digit LED display
 *
 * @ingroup BoardIO
 */
class LED_Display : public i2c2_device, public SingletonTemplate<LED_Display>
{
    public:
        bool init(); ///< Initializes this device, @returns true if successful

        /**
         * Clears the display
         */
        void clear();

        /**
         * Sets the number on LED display
         * @param num   A number less than 100
         */
        void setNumber(char num);

        /**
         * @{ Single Digit Manipulation Functions
         * Sets the left and right alpha-numeric display individually
         * @param alpha The alpha to set, such as "B" or "9"
         * @note  Note all characters can be displayed on this 7-segment display
         */
        void setLeftDigit(char alpha);
        void setRightDigit(char alpha);
        /** @} */

    private:
        char mNumAtDisplay[2]; ///< The number currently being displayed

        /// Private constructor of this Singleton class
        LED_Display() : i2c2_device(I2CAddr_LED_Display)
        {
        }
        friend class SingletonTemplate<LED_Display>;  ///< Friend class used for Singleton Template

        /// Enumeration of the register map
        typedef enum {
            inputPort0, inputPort1,
            outputPort0, outputPort1,
            polarityPort0, polarityPort1,
            cfgPort0, cfgPort1
        } __attribute__ ((packed)) RegisterMap;
};




#endif /* LED_DISPLAY_HPP__ */
