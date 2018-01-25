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

#include <stdio.h>      // printf
#include <string.h>     // memset() etc.

#include "tasks.hpp"
#include "c_tlm_var.h"
#include "io.hpp"

#include "shared_handles.h"
#include "scheduler_task.hpp"



remoteTask::remoteTask(uint8_t priority) :
        scheduler_task("remote", 512*3, priority),
        mIrNumber(0),
        mLearnSem(NULL)
{
    memset(mNumCodes, 0, sizeof(mNumCodes));
}

bool remoteTask::init(void)
{
    setRunDuration(100);
    mLearnSem = xSemaphoreCreateBinary();
    vTraceSetSemaphoreName(mLearnSem, "IR-snsr Sem");

    /**
     * For success: Created semaphore must not be NULL, we must be able to
     * take it, and finally, add it to the shared handles.
     */
    return (NULL != mLearnSem) && addSharedObject(shared_learnSemaphore, mLearnSem);
}

bool remoteTask::regTlm(void)
{
    bool success = true;
    #if SYS_CFG_ENABLE_TLM
        tlm_component *disk = tlm_component_get_by_name(SYS_CFG_DISK_TLM_NAME);
        if(success) {
            success = TLM_REG_ARR(disk, mNumCodes, tlm_uint);
        }
    #endif
    return success;
}
bool remoteTask::taskEntry(void)
{
    // LD.clear();
    return true;
}
bool remoteTask::run(void *p)
{
    uint32_t number = 0;
    STR_ON_STACK(temp, 64);

    if(xSemaphoreTake(mLearnSem, 0))
    {
        puts("IR Codes will be learned.  Press buttons 0-9 on the remote");
        LD.setLeftDigit('-');
        LD.setRightDigit('-');

        for(int i=0; i < 10; i++)
        {
            while(!IS.isIRCodeReceived()) {
                vTaskDelayMs(100);
            }

            unsigned int code = IS.getLastIRCode();
            temp.printf("Learned: #%i = %x", i, code);
            puts(temp());

            mNumCodes[i] = code;
            LD.setNumber(i);
        }

        puts("Learned all numbers!");
        vTaskDelayMs(2000);
    }

    /**
     * If the timer is running, we are expecting 2nd digit to be entered through IR code.
     * If the timeout occurs, we clear the LED display and throw away the IR numbers.
     */
    if (mIrNumTimer.isRunning()) {
        if(IS.isIRCodeReceived() && getNumberFromCode(IS.getLastIRCode(), number))
        {
            mIrNumber += number;
            LD.setRightDigit(number + '0');

            handleUserEntry(mIrNumber);
            vTaskDelayMs(2000);

            // Discard if any code came in within the delay above
            (void) IS.getLastIRCode();
            mIrNumTimer.stop();
        }
        else if (mIrNumTimer.expired()) {
            mIrNumTimer.stop();
            LD.clear();
        }
    }
    else {
        /**
         * If we got an IR code, we store the left digit, and start the timer to expect
         * the 2nd IR code to be entered.
         */
        if(IS.isIRCodeReceived() && getNumberFromCode(IS.getLastIRCode(), number)) {
            LD.setLeftDigit(number + '0');
            LD.setRightDigit('-');

            mIrNumber = 10 * number; // Left digit

            // If 2nd digit not entered soon enough, go back to this state.
            mIrNumTimer.reset(10 * 1000);
        }
        else {
            /* User can control LED display here if no IR code is being decoded */
        }
    }

    return true;
}

void remoteTask::handleUserEntry(int num)
{
    /* TODO Handle the IR number here for your project */
}

bool remoteTask::getNumberFromCode(uint32_t code, uint32_t& num)
{
    for(int i = 0; i < 10; i++)
    {
        if(mNumCodes[i] == code)
        {
            num = i;
            return true;
        }
    }

    return false;
}
