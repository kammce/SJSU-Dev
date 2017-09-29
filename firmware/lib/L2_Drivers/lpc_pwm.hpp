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
 * @ingroup Drivers
 */
#ifndef LPC_PWM_HPP__
#define LPC_PWM_HPP__



/**
 * LPC PWM class.
 * Example:
 * @code
 *      PWM pwm2(PWM::pwm2, 50);
 *      pwm2.set(10);
 *      pwm2.set(5.0);
 * @endcode
 */
class PWM
{
    public :
        typedef enum {
            pwm1=0, ///< P2.0
            pwm2=1, ///< P2.1
            pwm3=2, ///< P2.2
            pwm4=3, ///< P2.3
            pwm5=4, ///< P2.4
            pwm6=5  ///< P2.5
        } pwmType;

        /**
         * Initialize a PWM
         * @param frequencyHz  The frequency of the PWM
         * @note Frequency is only initialized the first time this class is constructed.
         *       Once it is initialized, it cannot be set or changed when more objects are constructed.
         */
        PWM(pwmType pwm, unsigned int frequencyHz=50);

        /// Destructor that will destroy PWM configuration
        ~PWM();

        /**
         * Sets the PWM based on the percentage.
         * If 50Hz Servo PWM was setup, then you can use the following :
         *      - Left    : set(5.0);  // 5.0 % of 20ms = 1.0ms
         *      - Neutral : set(7.5);  // 7.5 % of 20ms = 1.5ms
         *      - Right   : set(10.0); // 10  % of 20ms = 2.0ms
         *
         * You can use micro-steps to position the servo motor by using
         * set(5.1), set(5.2) ... set(7.4) etc.
         */
        bool set(float percent);

    private:
        PWM();                          ///< Disallow default constructor
        const pwmType mPwm;             ///< The PWM channel number set by constructor
        static unsigned int msTcMax;    ///< PWM TC max (this controls the frequency)
};



#endif /* LPC_PWM_HPP__ */
