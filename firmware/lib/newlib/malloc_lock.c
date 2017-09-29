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
 
#include "FreeRTOS.h"
#include "task.h"



/**
 * @file
 * @brief This file defines the GCC functions for malloc lock and unlock.
 *        These are mainly needed when using FreeRTOS, and are harmless if FreeRTOS is not running.
 *        GCC calls these functions before and after calling the malloc() functions.
 */

__attribute__ ((used)) void __malloc_lock( void *_r )
{
    vPortEnterCritical();
}

__attribute__ ((used)) void __malloc_unlock( void *_r )
{
    vPortExitCritical();
}
