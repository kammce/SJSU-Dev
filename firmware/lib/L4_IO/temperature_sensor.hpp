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
 *
 */
#ifndef TEMPERATURE_SENSOR_HPP_
#define TEMPERATURE_SENSOR_HPP_

#include "singleton_template.hpp"
#include "i2c2_device.hpp"  // I2C Device base class


/**
 * Base class of an I2C temperature sensor
 */
class I2C_Temp : private i2c2_device
{
    public:
        I2C_Temp(char addr) : i2c2_device(addr), mOffsetCelcius(0) {}
        bool init();

        float getCelsius();   ///< @returns floating-point reading of temperature in Celsius
        float getFarenheit(); ///< @returns floating-point reading of temperature in Farenheit
        float mOffsetCelcius; ///< Temperature offset
};

/**
 * Temperature Sensor class used to get temperature reading from the on-board sensor
 *
 * @ingroup BoardIO
 */
class TemperatureSensor : public I2C_Temp, public SingletonTemplate<TemperatureSensor>
{
    public:
        /* API is at I2C_Temp */

    private:
        /// Private constructor of this Singleton class
        TemperatureSensor() :  I2C_Temp(I2CAddr_TemperatureSensor)
        {
        }

        friend class SingletonTemplate<TemperatureSensor>;  ///< Friend class used for Singleton Template
};


#endif /* TEMPERATURE_SENSOR_HPP_ */
