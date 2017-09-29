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



#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "char_dev.hpp"
#include "utilities.h"      // system_get_timer_ms();



bool CharDev::put(const char* pString, unsigned int timeout)
{
    if (!pString) {
        return false;
    }

    while(*pString) {
        if(!putChar(*pString++, timeout)) {
            return false;
        }
    }

    return true;
}

void CharDev::putline(const char* pBuff, unsigned int timeout)
{
    this->put(pBuff, timeout);
    this->put("\r\n", timeout);
}

bool CharDev::gets(char* pBuff, int maxLen, unsigned int timeout)
{
    char c = 0;
    int charCount = 0;
    bool success = false;

    while(getChar(&c, timeout)) {
        if ('\r' != c && '\n' != c) {
            *pBuff++ = c;
        }
        if(++charCount >= maxLen) {
            break;
        }
        if('\n' == c) {
            success = true;
            break;
        }
    }

    // If we didn't get any characters, don't null terminate the string
    if(charCount > 0) {
        *pBuff = '\0';
    }

    return success;
}

int CharDev::printf(const char *format, ...)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        xSemaphoreTake(mPrintfSemaphore, portMAX_DELAY);
    }

        int len = 0;
        va_list args;
        va_start(args, format);

        do {
            va_list args_copy;
            va_copy(args_copy, args);
            len = vsnprintf(mpPrintfMem, mPrintfMemSize, format, args_copy);
            va_end(args_copy);

            if (len >= mPrintfMemSize) {
                const int align = 16;
                mPrintfMemSize = (align + ((len/align) * align));
                /* TO DO :
                 * Do not know why realloc() doesn't work.  It is a combination of C++ class
                 * use combined with va_args and realloc itself.  It seems to work in vector
                 * and str classes though.
                 */
                if (0) {
                    if (mpPrintfMem) {
                        free(mpPrintfMem);
                    }
                    mpPrintfMem = (char*) malloc(mPrintfMemSize);
                }
                else {
                    mpPrintfMem = (char*) realloc(mpPrintfMem, mPrintfMemSize);
                }
            }
            else {
                break;
            }
        } while (mpPrintfMem);

        va_end(args);
        this->put(mpPrintfMem);

    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState()) {
        xSemaphoreGive(mPrintfSemaphore);
    }

    return len;
}

int CharDev::scanf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int parsed = 0;
    char buff[256] = { 0 };

    if (this->gets(buff, sizeof(buff))) {
        parsed = vsscanf(buff, format, args);
    }
    va_end(args);

    return parsed;
}

CharDev::CharDev() : mpPrintfMem(NULL), mPrintfMemSize(0), mReady(false)
{
    mPrintfSemaphore = xSemaphoreCreateMutex();
    vTraceSetMutexName(mPrintfSemaphore, "printf sem");
}

CharDev::~CharDev()
{
    if (mpPrintfMem) {
        free(mpPrintfMem);
    }
}
