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
 * @brief This file simply provides registers that are used to detect faults.
 *        These registers should be defined to a RAM region not used by the program
 */
#ifndef FAULT_REGS_H__
#define FAULT_REGS_H__
#ifdef __cplusplus
extern "C" {
#endif



#include "LPC17xx.h"
#define FAULT_PRESENT_VAL               0xDEADBEEF          ///< Value loaded to FAULT_EXISTS upon a crash
#define FAULT_LAST_RUNNING_TASK_NAME    LPC_RTC->GPREG0     ///< FreeRTOS crash info register
#define FAULT_EXISTS                    LPC_RTC->GPREG1     ///< Fault flag is stored here
#define FAULT_PC                        LPC_RTC->GPREG2     ///< CPU PC counter after the crash
#define FAULT_LR                        LPC_RTC->GPREG3     ///< CPU Link register after the crash
#define FAULT_PSR                       LPC_RTC->GPREG4     ///< CPU PSR register



#ifdef __cplusplus
}
#endif
#endif
