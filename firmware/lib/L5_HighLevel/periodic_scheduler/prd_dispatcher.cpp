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

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "tasks.hpp"
#include "storage.hpp"
#include "lpc_sys.h"
#include "periodic_callback.h"



enum {
    prd_1Hz = 0,
    prd_10Hz,
    prd_100Hz,
    prd_1000Hz,
    prd_total
};

/// Semaphores that the period tasks wait upon
static SemaphoreHandle_t sems[prd_total];

/// @{ These are the actual FreeRTOS tasks that call the period functions
void period_task_1Hz(void *p)    { uint32_t count = 0; while (xSemaphoreTake(sems[prd_1Hz],    portMAX_DELAY)) period_1Hz(++count);    }
void period_task_10Hz(void *p)   { uint32_t count = 0; while (xSemaphoreTake(sems[prd_10Hz],   portMAX_DELAY)) period_10Hz(++count);   }
void period_task_100Hz(void *p)  { uint32_t count = 0; while (xSemaphoreTake(sems[prd_100Hz],  portMAX_DELAY)) period_100Hz(++count);  }
void period_task_1000Hz(void *p) { uint32_t count = 0; while (xSemaphoreTake(sems[prd_1000Hz], portMAX_DELAY)) period_1000Hz(++count); }
/// @}

periodicSchedulerTask::periodicSchedulerTask(bool kHz_enabled) :
    scheduler_task("dispatcher", PERIOD_DISPATCHER_TASK_STACK_SIZE_BYTES, PRIORITY_CRITICAL + PRIORITY_CRITICAL + 5),
    mKHz(kHz_enabled)
{
    setRunDuration(mKHz ? 1 : 10);
    setStatUpdateRate(0);

    // Create the semaphores first before creating the actual periodic tasks
    sems[prd_1Hz] = xSemaphoreCreateBinary();
    sems[prd_10Hz] = xSemaphoreCreateBinary();
    sems[prd_100Hz] = xSemaphoreCreateBinary();
    if (mKHz) {
        sems[prd_1000Hz] = xSemaphoreCreateBinary();
    }

    // Optional: Provide names of the FreeRTOS objects for the Trace Facility
    vTraceSetSemaphoreName(sems[prd_1Hz], "1Hz_Sem");
    vTraceSetSemaphoreName(sems[prd_10Hz], "10Hz_Sem");
    vTraceSetSemaphoreName(sems[prd_100Hz], "100Hz_Sem");
    if (mKHz) {
        vTraceSetSemaphoreName(sems[prd_1000Hz], "1000Hz_Sem");
    }

    // Create the FreeRTOS tasks, these will only run once we start giving their semaphores
    xTaskCreate(period_task_1Hz, "1Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 1, NULL);
    xTaskCreate(period_task_10Hz, "10Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 2, NULL);
    xTaskCreate(period_task_100Hz, "100Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 3, NULL);
    if (mKHz) {
        xTaskCreate(period_task_1000Hz, "1000Hz", PERIOD_TASKS_STACK_SIZE_BYTES/4, NULL, PRIORITY_CRITICAL + 4, NULL);
    }
}

bool periodicSchedulerTask::init(void)
{
    return period_init();
}

bool periodicSchedulerTask::regTlm(void)
{
    return period_reg_tlm();
}


bool periodicSchedulerTask::run(void *p)
{
    // Run 1000hz each if the scheduling was enabled
    if (mKHz) {
        if (handlePeriodicSemaphore(prd_1000Hz, 1)) {
            if (handlePeriodicSemaphore(prd_100Hz, 10)) {       // Run 100Hz every 10th time of 1000Hz
                if (handlePeriodicSemaphore(prd_10Hz, 10)) {    // Run 10Hz every 10th time of 100Hz
                    if (handlePeriodicSemaphore(prd_1Hz, 10)) { // Run 1Hz every 10th time of 10Hz
                        ; // 1Hz task ran; nothing to do
                    }
                }
            }
        }
    }
    else {
        if (handlePeriodicSemaphore(prd_100Hz, 1)) {        // Run 100Hz each time since we get unblocked at 10Hz
            if (handlePeriodicSemaphore(prd_10Hz, 10)) {    // Run 10Hz every 10th time of 100Hz
                if (handlePeriodicSemaphore(prd_1Hz, 10)) { // Run 1Hz every 10th time of 10Hz
                    ; // 1Hz task ran; nothing to do
                }
            }
        }
    }

    return true;
}

bool periodicSchedulerTask::handlePeriodicSemaphore(const uint8_t index, const uint8_t frequency)
{
    bool semGiven = false;
    SemaphoreHandle_t sem = sems[index];
    static uint8_t counters[prd_total] = { 0 };
    static const char * overrunMsg[] = { "1Hz task overrun", "10Hz task overrun", "100Hz task overrun", "1000Hz task overrun" };

    if (++counters[index] == frequency) {
        counters[index] = 0;
        semGiven = true;

        // If we can take the semaphore before giving, then the periodic task did
        // not take the semaphore within its allocated time
        if (xSemaphoreTake(sem, 0)) {

            // Write a message to a file for indication
            puts(overrunMsg[index]);
            Storage::append("restart.txt", overrunMsg[index], strlen(overrunMsg[index]), 0);

            // Reboot
            sys_reboot_abnormal();
        }
        else {
            xSemaphoreGive(sem);
        }
    }

    return semGiven;
}
