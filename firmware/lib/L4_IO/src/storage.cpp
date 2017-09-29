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

#include "lpc_sys.h"
#include "storage.hpp"
#include "ff.h"



FRESULT Storage::copy(const char* pExistingFile, const char* pNewFile,
                        unsigned int* pReadTime,
                        unsigned int* pWriteTime,
                        unsigned int* pBytesTransferred)
{
    FRESULT status;
    FIL srcFile;
    FIL dstFile;
    unsigned int readTimeMs = 0;
    unsigned int writeTimeMs = 0;

    // Open Existing file
    if (FR_OK != (status = f_open(&srcFile, pExistingFile, FA_OPEN_EXISTING | FA_READ))) {
        return status;
    }

    // Open new file - overwrite if one exists
    if (FR_OK != (status = f_open(&dstFile, pNewFile, FA_CREATE_ALWAYS | FA_WRITE))) {
        f_close(&srcFile);
        return status;
    }

    /* Buffer should be at least he size of the sector */
    char buffer[_MAX_SS];
    unsigned int bytesRead = 0;
    unsigned int bytesWritten = 0;
    unsigned int totalBytesTransferred = 0;

    for (;;)
    {
        unsigned int startTime = sys_get_uptime_ms();
        if(FR_OK != (status = f_read(&srcFile, buffer, sizeof(buffer), &bytesRead)) ||
           0 == bytesRead) {
            break;
        }
        readTimeMs += sys_get_uptime_ms() - startTime;

        startTime = sys_get_uptime_ms();
        if(FR_OK != (status = f_write(&dstFile, buffer, bytesRead, &bytesWritten)) ||
           bytesWritten != bytesRead) {
            break;
        }
        writeTimeMs += sys_get_uptime_ms() - startTime;

        totalBytesTransferred += bytesRead;
    }

    if(0 != pReadTime) {
        *pReadTime = readTimeMs;
    }
    if(0 != pWriteTime) {
        *pWriteTime = writeTimeMs;
    }
    if(0 != pBytesTransferred) {
        *pBytesTransferred = totalBytesTransferred;
    }

    f_close(&srcFile);
    f_close(&dstFile);

    return status;
}

FRESULT Storage::read(const char* pFilename,  void* pData, unsigned int bytesToRead, unsigned int offset)
{
    FRESULT status = FR_INT_ERR;
    FIL file;
    unsigned int bytesRead = 0;

    // Open Existing file
    if (FR_OK == (status = f_open(&file, pFilename, FA_OPEN_EXISTING | FA_READ)))
    {
        if(offset) {
            f_lseek(&file, offset);
        }
        status = f_read(&file, pData, bytesToRead, &bytesRead);
        f_close(&file);
    }

    return status;
}

FRESULT Storage::write(const char* pFilename, void* pData, unsigned int bytesToWrite, unsigned int offset)
{
    FRESULT status = FR_INT_ERR;
    FIL file;
    unsigned int bytesWritten = 0;

    status = f_open(&file, pFilename, FA_CREATE_ALWAYS | FA_WRITE);
    if(FR_OK == status)
    {
        if(offset) {
            f_lseek(&file, offset);
        }
        status = f_write(&file, pData, bytesToWrite, &bytesWritten);
        f_close(&file);
    }

    return status;
}

FRESULT Storage::append(const char* pFilename, const void* pData, unsigned int bytesToAppend, unsigned int offset)
{
    FRESULT status = FR_INT_ERR;
    FIL file;
    unsigned int bytesWritten = 0;

    // Open Existing file
    if (FR_OK == (status = f_open(&file, pFilename, FA_OPEN_ALWAYS | FA_WRITE)))
    {
        if(offset > 0) {
            f_lseek(&file, offset);
        }
        else {
            f_lseek(&file, f_size(&file));
        }

        status = f_write(&file, pData, bytesToAppend, &bytesWritten);

        f_close(&file);
    }

    return status;
}
