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
 
#ifndef LPC_GPIO_H__
#define LPC_GPIO_H__

#include <stdint.h>
#include "LPC17xx.h"


/// The "Port Number Start Bit" which is used to get value of port number from LPC1758_GPIO_Type
#define __PNSB  5

/**
 * Enumeration of GPIO IDs.
 * The pins are also labeled with popular alternate options just for reference.
 * @warning Do not change the value(s) or the order of the enumerations.
 *          The enum value is a combination of port number and the pin number
 */
typedef enum
{
    P0_0  = (0 << __PNSB) | 0,  /**< Uart, I2C, CAN */
    P0_1  = (0 << __PNSB) | 1,  /**< Uart, I2C, CAN */
    P0_26 = (0 << __PNSB) | 26, /**< AD0.3, AOUT, U3-Tx */
    P0_29 = (0 << __PNSB) | 29, /**< USB D+ */
    P0_30 = (0 << __PNSB) | 30, /**< USB D- */

    P1_19 = (1 << __PNSB) | 19, /** CAP1.1, MCOA0 */
    P1_20 = (1 << __PNSB) | 20, /** MCI0, PWM1.2  */
    P1_22 = (1 << __PNSB) | 22, /** MCOB0, USB_PWRD, MAT1.0 */
    P1_23 = (1 << __PNSB) | 23, /** MCI1, PWM1.4  */
    P1_28 = (1 << __PNSB) | 28, /** MAT0.0, PCAP1.0, MCOA2 */
    P1_29 = (1 << __PNSB) | 29, /** MAT0.1, PCAP1.2, MCOB2 */
    P1_30 = (1 << __PNSB) | 30, /**< AD0.4, VBUS */
    P1_31 = (1 << __PNSB) | 31, /**< AD0.5 */

    P2_0 = (2 << __PNSB) | 0, /**< PWM1.1, U1-TX */
    P2_1 = (2 << __PNSB) | 1, /**< PWM1.2, U1-RX */
    P2_2 = (2 << __PNSB) | 2, /**< PWM1.3, U1-CTS */
    P2_3 = (2 << __PNSB) | 3, /**< PWM1.4, U1-DCD */
    P2_4 = (2 << __PNSB) | 4, /**< PWM1.5, U1-DSR */
    P2_5 = (2 << __PNSB) | 5, /**< PWM1.6, U1-DTR */
    P2_6 = (2 << __PNSB) | 6, /**< PCAP1.0 */
    P2_7 = (2 << __PNSB) | 7, /**< CAN2.Rx */
    P2_8 = (2 << __PNSB) | 8, /**< CAN2.Tx */
    P2_9 = (2 << __PNSB) | 9, /**< USB Connect */

    P4_28 = (4 << __PNSB) | 28, /** Uart3 Tx, MAT2.0 */
    P4_29 = (4 << __PNSB) | 29, /** Uart3 Rx, MAT2.1 */
} LPC1758_GPIO_Type;

/**
 * GPIO class for SJ-One Board or any LPC17xx CPU
 * The pins on the SJ-One Board can be used as General Purpose Input/Output
 * pins which can be attached to for example an LED or a switch externally.
 *
 * Please see the pin labels on the board and then use this class to
 * manipulate the pin(s).  See the next examples :
 *
 * Output pin example :
 * @code
 *      GPIO myPin(P1_19);   // Control P1.19
 *      myPin.setAsOutput(); // Use the pin as output pin
 *      myPin.setHigh();     // Pin will now be at 3.3v
 *      myPin.setLow();      // Pin will now be at 0.0v
 * @endcode
 *
 * Input pin example :
 * @code
 *      GPIO myPin(P1_20);   // Control P1.20
 *      myPin.setAsInput();  // Use the pin as output pin
 *      bool value = myPin.read(); // Read value of the pin
 * @endcode
 */
class GPIO
{
    public:
        GPIO(LPC1758_GPIO_Type gpioId); ///< Constructor to choose the pin
        ~GPIO(); ///< Destructor that will destroy the pin configuration

        void setAsInput(void);   ///< Sets pin as input pin
        void setAsOutput(void);  ///< Sets pin as output pin

        bool read(void) const;   ///< Reads logical value of the pin
        void setHigh(void);      ///< Sets the pin to logical HIGH (3.3v)
        void setLow(void);       ///< Sets the pin to logical LOW (0.0v)
        void set(bool on);       ///< Set high/low based on boolean value
        void toggle(void);       ///< Toggles the state of the GPIO

        void enablePullUp();            ///< Enables pull-up resistor
        void enablePullDown();          ///< Enables pull-down resistor
        void disablePullUpPullDown();   ///< Disables pull-up/down resistor
        void enableOpenDrainMode(bool openDrain=true); ///< Enables open drain mode

    protected:
    private:
        GPIO(); ///< Do not use this constructor

        const uint8_t mPortNum;                 ///< This GPIOs port number
        const uint8_t mPinNum;                  ///< This GPIOs pin number
        volatile LPC_GPIO_TypeDef *mpOurGpio;   ///< This GPIO's pointer
};



#endif /* GPIO_H__ */

