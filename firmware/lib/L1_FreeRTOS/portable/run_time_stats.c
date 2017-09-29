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

#include "FreeRTOSConfig.h"
#include "lpc_sys.h"



#if (1 == configGENERATE_RUN_TIME_STATS)
/**
 * This stores the "start" time value.  The system timer keeps running, and this
 * variable provides us the capability to reset the FreeRTOS timer without having
 * to reset the source or originating timer.
 */
static uint64_t g_freertos_runtime_timer_start = 0;



// Init the run time counter that is not used by the full trace
void rts_not_full_trace_init( void )
{
    /* Nothing to do, system timer should already be setup by high_level_init.cpp */
    g_freertos_runtime_timer_start = sys_get_uptime_us();
}

unsigned int rts_not_full_trace_get()
{
    return (sys_get_uptime_us() - g_freertos_runtime_timer_start);
}
void rts_not_full_trace_reset()
{
    g_freertos_runtime_timer_start = sys_get_uptime_us();
}
#endif
