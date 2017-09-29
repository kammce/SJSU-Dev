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
#include "semphr.h"



static SemaphoreHandle_t mSpi0Lock = 0;



void spi1_lock(void)
{
    if(!mSpi0Lock) {
        mSpi0Lock = xSemaphoreCreateMutex();
        // Optional: Provide names of the FreeRTOS objects for the Trace Facility
        vTraceSetMutexName(mSpi0Lock, "SPI-0 Mutex");
    }


    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        xSemaphoreTake(mSpi0Lock, portMAX_DELAY);
    }
}

void spi1_unlock(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        xSemaphoreGive(mSpi0Lock);
    }
}
