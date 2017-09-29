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
#include <stdlib.h>       // malloc(), realloc()
#include <string.h>       // strlen()

#include "printf_lib.h"
#include "uart0_min.h"
#include "sys_config.h"
#include "FreeRTOS.h"



/// Un-used function that uses assembly symbols to bring in support for floating-point printf() / scanf()
__attribute__((used)) void ____bring_in_floats____(void)
{
    /* By including ASM symbols, we can bring in floating point support from the newlib nano.
     * We can also give it an option for linker argument, but it is more inconvenient to do
     * it that way (in project properties).
     */
    #if (0 == SYS_CFG_REDUCED_PRINTF)
    asm (".global _printf_float");
    asm (".global _scanf_float");
    #endif
}

int u0_dbg_printf(const char *format, ...)
{
    int len = 0;
    char buff[256] = { 0 };

    va_list args;
    va_start(args, format);
    len = vsnprintf(buff, sizeof(buff), format, args);
    va_end(args);

    u0_dbg_put(buff);

    return len;
}

void u0_dbg_put(const char *string)
{
    char* p = (char*) string;

    vPortEnterCritical();
    while(*p)
    {
        uart0_putchar(*p);
        p++;
    }
    vPortExitCritical();
}

char* mprintf(const char *format, ...)
{
    int len = 0;
    const int align = 16;
    char *str_ptr = NULL;

    // Allocate chunk of "align" sized block with a little extra space for parameter printing (align * 2)
    int mem = ((strlen(format) / align) * align) + (align * 2);
    str_ptr = (char*) malloc(mem);

    va_list args;
    va_start(args, format);

    while (NULL != str_ptr)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        len = vsnprintf(str_ptr, mem, format, args_copy);
        va_end(args_copy);

        // If we could not print, reallocate and try again.
        if (len >= mem) {
            mem = ((len / align) * align) + align;
            str_ptr = realloc(str_ptr, mem);
        }
        else {
            break;
        }
    };

    va_end(args);
    return str_ptr;
}
