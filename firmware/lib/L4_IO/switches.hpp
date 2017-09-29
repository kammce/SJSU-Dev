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
#ifndef SWITCHES_HPP__
#define SWITCHES_HPP__
#include <stdint.h>
#include "singleton_template.hpp"



/**
 * Switches class used to get switch values from on-board switches
 *
 * @ingroup BoardIO
 */
class Switches : public SingletonTemplate<Switches>
{
    public:
        bool init(); ///< Initializes this device, @returns true if successful

        /// @returns The 8-bit char containing bit values of the 4 switches, 1 bit per switch
        uint8_t getSwitchValues();

        /**
         * @param num   The switch number to test from 1-4
         * @returns true if the switch is active, or false otherwise
         */
        bool getSwitch(int num);

    private:
        Switches() {}   ///< Private constructor of this Singleton class
        friend class SingletonTemplate<Switches>;  ///< Friend class used for Singleton Template
};

#endif /* SWITCHES_HPP__ */
