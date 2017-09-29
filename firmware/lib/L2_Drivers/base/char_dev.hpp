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
 * @brief Provides a 'char' device base class functionality for stream oriented char devices
 *
 * 20140420 : Reverted back to non-static members
 * 20131201 : Initial version
 */
#ifndef CHAR_DEV_HPP_
#define CHAR_DEV_HPP_

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"



/**
 * Char device base class.  You shouldn't use this directly because this is an
 * abstract class and must be inherited and used by a parent object.
 *
 *  @ingroup Drivers
 */
class CharDev
{
    public:
        /**
         * @returns a character from the UART input
         * @param   pInputChar  The pointer to input char to store received character
         * @param   timeout Optional parameter which defaults to maximum value that
         *          will allow you to wait forever for a character to be received
         * @returns true if a character was obtained within the given timeout
         */
        virtual bool getChar(char* pInputChar, unsigned int timeout=portMAX_DELAY) = 0;

        /**
         * Outputs a char given by @param out
         * @param   timeout Optional parameter which defaults to maximum value that
         *          will allow you to wait forever for a character to be sent
         * @returns true if the output char was successfully written to Queue, or
         *          false if the output queue was full within the given timeout
         */
        virtual bool putChar(char out, unsigned int timeout=portMAX_DELAY) = 0;

        /**
         * Optional flush to flush out all the data
         */
        virtual bool flush(void) { return true; }

        /**
         * @{ Output a null-terminated string
         * puts() will also output newline chars "\r\n" at the end of the string
         */
        bool put  (const char* pString, unsigned int timeout=0xffffffff);
        void putline(const char* pBuff, unsigned int timeout=0xffffffff);
        /** @} */

        /**
         * Get a string of input up to maxLen
         * @param pBuff The buffer to store data to
         * @param maxLen The maximum chars to get
         * @param timeout The timeout in ticks to wait
         */
        bool gets(char* pBuff, int maxLen, unsigned int timeout=0xffffffff);

        /**
         * Just like printf, except it will print to this output interface
         * @returns the number of characters printed
         */
        int printf(const char *format, ...);

        /**
         * Just like scanf, except this will perform scanf after receiving a line
         * of input using the gets() method.
         * @warning  scanf() requires a \n (newline) terminated char to work.
         */
        int scanf(const char *format, ...);

        /**
         * Get the size for the printf() memory used by printf() function
         */
        inline uint16_t getPrintfMemSize(void) const { return mPrintfMemSize; }

        /**
         * @{  This API just provides a means to set a flag if UART is ready or not
         *     This doesn't cause any change to the way UART functions.
         */
        bool isReady(void) { return mReady; }
        void setReady(bool r) { mReady = r; }
        /** @} */

    protected:
        CharDev();
        virtual ~CharDev();

    private:
        char *mpPrintfMem;                  ///< Heap pointer used by printf()
        uint16_t mPrintfMemSize;            ///< Size of heap used by printf()
        SemaphoreHandle_t mPrintfSemaphore; ///< Semaphore to lock printf()
        bool mReady;                        ///< Marker if device is ready or not
};



#endif /* CHAR_DEV_HPP_ */
