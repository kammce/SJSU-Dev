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
 *  @file
 *  @brief This file provides access to all IO devices of the board
 *  @ingroup BoardIO
 *
 *  Version: 06192012    Initial
 */
 
#ifndef IO_SHORTCUTS_HPP_
#define IO_SHORTCUTS_HPP_



#include "acceleration_sensor.hpp"
#include "IR_sensor.hpp"
#include "LED_Display.hpp"
#include "LED.hpp"
#include "light_sensor.hpp"
#include "switches.hpp"
#include "temperature_sensor.hpp"


/**
 * @{ Short-hand notation of getting to the on-board IO Devices
 * @ingroup BoardIO
 */
#define AS      Acceleration_Sensor::getInstance() ///< Acceleration Sensor
#define IS      IR_Sensor::getInstance()           ///< IR input sensor
#define LS      Light_Sensor::getInstance()        ///< Light Sensor
#define TS      TemperatureSensor::getInstance()   ///< Temperature Sensor
#define LD      LED_Display::getInstance()         ///< 2-Digit LED Display
#define LE      LED::getInstance()                 ///< 4 On-board LEDs
#define SW      Switches::getInstance()            ///< 4 On-board Switches
/** @} */



#endif /* IO_SHORTCUTS_HPP_ */
