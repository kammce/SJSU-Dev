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
 */

#ifndef i2c2_device_HPP_
#define i2c2_device_HPP_

#include "i2c2.hpp"



/**
 * I2C Device Base Class
 * This class can be inherited by an I2C Device to be able to read and write registers
 * more easily as this class will puts an abstraction layer on I2C and provides simple
 * functionality to read and write registers over I2C Bus.
 *
 * @ingroup BoardIO
 *
 */
class i2c2_device
{
protected:
    /// Constructor of this base class that takes addr as a parameter
    i2c2_device(uint8_t addr) : mI2C(I2C2::getInstance()), mOurAddr(addr)
    {
    }

    /// @returns the register content of this device
    inline uint8_t readReg(unsigned char reg)
    {
        return mI2C.readReg(mOurAddr, reg);
    }

    /// Writes a register of this device
    inline void writeReg(unsigned char reg, unsigned char data)
    {
        mI2C.writeReg(mOurAddr, reg, data);
    }

    /// @returns true if the device responds to its address
    inline bool checkDeviceResponse()
    {
        return mI2C.checkDeviceResponse(mOurAddr);
    }

    /**
     * Reads 16-bit register from reg and reg+1 granted that reg has MSB
     */
    uint16_t get16BitRegister(unsigned char reg)
    {
        uint8_t buff[2] = {0};
        mI2C.readRegisters(mOurAddr, reg, &buff[0], sizeof(buff));

        const uint16_t MSB = buff[0];
        const uint16_t LSB = buff[1];
        return ((MSB << 8) | (LSB & 0xFF));
    }

private:
    I2C_Base& mI2C; /// Instance of I2C Bus used for communication
    const uint8_t mOurAddr; ///< I2C Address of this device

    // Do not use this constructor
    i2c2_device();
};

#endif /* i2c2_device_HPP_ */
