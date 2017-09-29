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

#include "lpc_pwm.hpp"
#include "LPC17xx.h"
#include "sys_config.h"



/// Static variable of this class
unsigned int PWM::msTcMax = 0;

PWM::PWM(pwmType pwm, unsigned int frequencyHz) :
    mPwm(pwm)
{
    /* Avoid divide by zero */
    if (0 == frequencyHz) {
        frequencyHz = 100;
    }

    if (0 == msTcMax) {
        msTcMax = (sys_get_cpu_clock() / frequencyHz);

        lpc_pconp(pconp_pwm1, true);
        lpc_pclk(pclk_pwm1, clkdiv_1);

        // Reset on PWMMR0: the PWMTC will be reset if PWMMR0 matches it.
        LPC_PWM1->MCR |= (1 << 1);

        // Suppose CPU = 48,000,000 (48Mhz)
        // frequency desired = 50   (50Hz)
        // PWM1MR0 will be ~ 960,000 to produce 50hz PWM
        LPC_PWM1->MR0 = msTcMax;

        LPC_PWM1->TCR = (1 << 0) | (1 << 3); // Enable PWM1
        LPC_PWM1->CTCR &= ~(0xF << 0);
    }

    // Pinsel the PWM
    LPC_PINCON->PINSEL4 &= ~(3 << (mPwm*2));
    LPC_PINCON->PINSEL4 |=  (1 << (mPwm*2));

    // Enable the PWM (bits 9-14)
    LPC_PWM1->PCR |= (1 << (mPwm + 9));
}

PWM::~PWM()
{
    LPC_PWM1->PCR &= ~(1 << (mPwm + 9));
    LPC_PINCON->PINSEL4 &= ~(3 << (mPwm*2));
}

bool PWM::set(float percent)
{
    if(percent > 100) {
        return false;
    }

    // Get the value from the percent
    // If percent = 50, mTcMax = 1000, then value will be 50*1000 / 100 = 500
    const unsigned int valueNeeded = (percent * msTcMax) / 100;

    switch(mPwm)
    {
        /* No tricks here, MR1-MR6 are not contiguous memory location */
        case pwm1: LPC_PWM1->MR1 = valueNeeded;  break;
        case pwm2: LPC_PWM1->MR2 = valueNeeded;  break;
        case pwm3: LPC_PWM1->MR3 = valueNeeded;  break;
        case pwm4: LPC_PWM1->MR4 = valueNeeded;  break;
        case pwm5: LPC_PWM1->MR5 = valueNeeded;  break;
        case pwm6: LPC_PWM1->MR6 = valueNeeded;  break;
        default : return false;
    }

    // Enable the latch
    LPC_PWM1->LER |= (1 << (mPwm+1));
    return true;
}
