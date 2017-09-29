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
 
#include "gpio.hpp"



GPIO::GPIO(LPC1758_GPIO_Type gpioId) :
    mPortNum(gpioId >> __PNSB),
    mPinNum(gpioId & (0xFF >> (8 - __PNSB))) // Should result in (gpioId & 0x1F) if __PNSB is value of 5
{
    uint32_t gpioMemBases[] = { LPC_GPIO0_BASE, LPC_GPIO1_BASE, LPC_GPIO2_BASE, LPC_GPIO3_BASE, LPC_GPIO4_BASE};
    mpOurGpio = (LPC_GPIO_TypeDef*) gpioMemBases[mPortNum];

    // Select GPIO configuration
    volatile uint32_t *pinsel = &(LPC_PINCON->PINSEL0);
    pinsel += (2 * mPortNum);
    *pinsel &= ~(3 << (2*mPinNum));
}

GPIO::~GPIO()
{
    setAsInput();
    enableOpenDrainMode(false);
    enablePullUp();
}

/** @{ Simple functions*/
void GPIO::setAsInput(void)   {   mpOurGpio->FIODIR &= ~(1 << mPinNum);          }
void GPIO::setAsOutput(void)  {   mpOurGpio->FIODIR |= (1 << mPinNum);           }
bool GPIO::read(void) const   {   return !!(mpOurGpio->FIOPIN & (1 << mPinNum)); }
void GPIO::setHigh(void)      {   mpOurGpio->FIOSET = (1 << mPinNum);            }
void GPIO::setLow(void)       {   mpOurGpio->FIOCLR = (1 << mPinNum);            }
void GPIO::set(bool on)       {   (on) ? setHigh() : setLow();                   }
void GPIO::toggle(void)       {   this->read() ? setLow() : setHigh();           }
/** @} */

void GPIO::enablePullUp()
{
    volatile uint32_t *pinmode = &(LPC_PINCON->PINMODE0);
    pinmode += (2 * mPortNum);
    *pinmode &= ~(3 << (2*mPinNum));
}
void GPIO::enablePullDown()
{
    volatile uint32_t *pinmode = &(LPC_PINCON->PINMODE0);
    pinmode += (2 * mPortNum);
    *pinmode |= (3 << (2*mPinNum));
}
void GPIO::disablePullUpPullDown()
{
    volatile uint32_t *pinmode = &(LPC_PINCON->PINMODE0);
    pinmode += (2 * mPortNum);
    *pinmode &= ~(3 << (2*mPinNum));
    *pinmode |=  (2 << (2*mPinNum));
}
void GPIO::enableOpenDrainMode(bool openDrain)
{
    volatile uint32_t *pinmode_od = &(LPC_PINCON->PINMODE_OD0);
    pinmode_od += mPortNum;
    if(openDrain) {
        *pinmode_od |= (1 << mPinNum);
    }
    else {
        *pinmode_od &= ~(1 << mPinNum);
    }
}
