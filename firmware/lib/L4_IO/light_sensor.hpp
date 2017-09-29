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
 
#ifndef LIGHT_SENSOR_HPP_
#define LIGHT_SENSOR_HPP_
#include <stdint.h>



/**
 * Light Sensor class used to get light reading from the Board's Light Sensor
 *
 * @ingroup BoardIO
 */
class Light_Sensor : public SingletonTemplate<Light_Sensor>
{
    public:
        bool init(); ///< Initializes this device, @returns true if successful

        uint16_t getRawValue();       ///< @returns light sensor reading
        uint8_t  getPercentValue();   ///< @returns light sensor reading as percentage

    private:
        Light_Sensor() { }  ///< Private constructor of this Singleton class
        friend class SingletonTemplate<Light_Sensor>;  ///< Friend class used for Singleton Template
};




#endif /* LIGHT_SENSOR_HPP_ */
