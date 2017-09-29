/*     SocialLedge.com - Copyright (C) 2013
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
#include "FreeRTOSConfig.h"

#if 1 == configMEM_MANG_TYPE
    #include "heap_1.c.inc"
#elif 2 == configMEM_MANG_TYPE
    #include "heap_2.c.inc"
#elif 3 == configMEM_MANG_TYPE
    #include "heap_3.c.inc"
#elif 4 == configMEM_MANG_TYPE
    #include "heap_4.c.inc"
#elif 5 == configMEM_MANG_TYPE
    #include "heap_5.c.inc"
#else
    #error "configMEM_MANG_TYPE is not defined correctly"
#endif
