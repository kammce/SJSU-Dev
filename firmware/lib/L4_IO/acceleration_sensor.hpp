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
 
#ifndef ACCELERATION_SENSOR_HPP_
#define ACCELERATION_SENSOR_HPP_
#include <stdint.h>
#include "i2c2_device.hpp"  // I2C Device Base Class



/**
 * Acceleration Sensor class used to get acceleration data reading from the on-board sensor.
 * Acceleration data reading can provide absolute tilt of the board (if under no movement),
 * and it can also provide the movement activity of the board.
 *
 * @ingroup BoardIO
 */
class Acceleration_Sensor : private i2c2_device, public SingletonTemplate<Acceleration_Sensor>
{
    public:
        bool init();   ///< Initializes the sensor

        int16_t getX();  ///< @returns X-Axis value
        int16_t getY();  ///< @returns Y-Axis value
        int16_t getZ();  ///< @returns Z-Axis value

    private:
        /// Private constructor of this Singleton class
        Acceleration_Sensor() : i2c2_device(I2CAddr_AccelerationSensor)
        {
        }
        friend class SingletonTemplate<Acceleration_Sensor>;  ///< Friend class used for Singleton Template



        /// Expected value of Sensor's "WHO AM I" register
        static const unsigned char mWhoAmIExpectedValue = 0x2A;
        typedef enum {
            status=0,
            X_MSB=1, X_LSB=2,
            Y_MSB=3, Y_LSB=4,
            Z_MSB=5, Z_LSB=6,

            SysMod=0xb, IntSource=0xc,WhoAmI=0xd,
            XYZDataCfg=0xe,HPFilterCutoff=0xf,

            PL_Status=0x10, PL_Cfg=0x11,    PL_Count=0x12,  PL_BF_ZComp=0x13, PL_THS_Reg=0x14,
            FF_MT_Cfg=0x15, FF_MT_Src=0x16, FF_MT_THS=0x17, FF_MT_Count=0x18,

            Transient_Cfg=0x1d, Transient_Src=0x1e, Transient_THS=0x1f, Transient_Count=0x20,
            Pulse_Cfg=0x21, Pulse_Src=0x22, Pulse_THSX=0x23, Pulse_THSY=0x24, Pulse_THSZ=0x25,
            Pulse_TMLT=0x26, Pulse_LTCY=0x27, Pulse_Wind=0x28,

            ASLP_Count=0x29,
            Ctrl_Reg1=0x2A, Ctrl_Reg2=0x2B, Ctrl_Reg3=0x2C, Ctrl_Reg4=0x2D, Ctrl_Reg5=0x2E,
            OffsetX=0x2F, OffsetY=0x30, OffsetZ=0x31

        } __attribute__ ((packed)) RegisterMap;

};


#endif /* ACCELERATION_SENSOR_HPP_ */
