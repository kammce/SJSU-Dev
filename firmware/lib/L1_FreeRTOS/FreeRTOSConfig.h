/*
    FreeRTOS V8.0 - Copyright (C) 2011 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H



/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/
#include "sys_config.h"
#define configUSE_PREEMPTION		            1   ///< One task can preempt another task (if equal or higher priority)
#define configUSE_IDLE_HOOK			            1   ///< Using IDLE task can put CPU to low power mode
#define configUSE_TICK_HOOK 		            0   ///< Every timer interrupt calls the tick function
#define configUSE_MALLOC_FAILED_HOOK            1   ///< If memory runs out, the hook function is called

#define configCPU_CLOCK_HZ			            (SYS_CFG_DESIRED_CPU_CLK)
#define configTICK_RATE_HZ			            ( 1000 )
#define configENABLE_BACKWARD_COMPATIBILITY     0

/* Avoid using IDLE priority since I have found that the logger task corrupts the file system at IDLE priority.
 * This probably has to do with SPI(with DMA) bus not functioning correctly when IDLE task puts the CPU to sleep.
 *
 * configMAX_PRIORITIES should include +1 for idle task priority, and + periodic scheduler priority.
 */
#define PERIODIC_SCH_PRIORITIES                 (5)
#define configMAX_PRIORITIES                    (1 + 4 + PERIODIC_SCH_PRIORITIES)

// Idle priority of 0 should not be used
#define PRIORITY_LOW        1
#define PRIORITY_MEDIUM     2
#define PRIORITY_HIGH       3
// Critical priority is the highest priority before the periodic scheduler priorities start
#define PRIORITY_CRITICAL 	(configMAX_PRIORITIES - PERIODIC_SCH_PRIORITIES - 1)


/**
 * @{ FreeRTOS Memory configuration
 * 1 - Get from the pool defined by configTOTAL_HEAP_SIZE.  No free()
 * 2 - Get from the pool defined by configTOTAL_HEAP_SIZE with free()
 * 3 - Just redirect FreeRTOS memory to malloc() and free()
 * 4 - Same as 2, but coalescencent blocks can be combined.
 *
 * configTOTAL_HEAP_SIZE only matters when scheme 1, 2 or 4 is used above.
 */
#define configMEM_MANG_TYPE             3
#define configTOTAL_HEAP_SIZE           ( ( size_t ) ( 24 * 1024 ) )
/** @} */

/* Stack size and utility functions */
#define configMINIMAL_STACK_SIZE		( ( unsigned short ) 128 )	 ///< Do not change this
#define STACK_BYTES(x)					((x)/4)	                     ///< 32-bit CPU use 4 bytes per stack unit
#define MS_PER_TICK()					( 1000 / configTICK_RATE_HZ) ///< tick to millisecond factor
#define OS_MS(x)						( x / MS_PER_TICK() )        ///< Ticks to millisecond conversion
#define vTaskDelayMs(x)                 vTaskDelay(OS_MS((x)))       ///< Sleep for ms instead of ticks
#define xTaskGetMsCount()               (xTaskGetTickCount() * MS_PER_TICK())

/* General config */
#define configMAX_TASK_NAME_LEN		            ( 8 )
#define configUSE_16_BIT_TICKS                  0       ///< Use 16-bit ticks vs. 32-bits
#define configIDLE_SHOULD_YIELD                 1       ///< See FreeRTOS documentation
#define configCHECK_FOR_STACK_OVERFLOW          2       ///< 0=OFF, 1=Simple, 2=Complex
#define configUSE_ALTERNATIVE_API               0       ///< No need to use deprecated API
#define configQUEUE_REGISTRY_SIZE               0       ///< See FreeRTOS documentation

/* FreeRTOS Co-routine */
#define configUSE_CO_ROUTINES 		            0       ///< No need for co-routines, just use tasks
#define configMAX_CO_ROUTINE_PRIORITIES         ( 1 )   ///< No need if not using co-routines

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           1       ///< CPU usage utilities
#define configUSE_TRACE_FACILITY                0       ///< FreeRTOS + Percepio Tracealyzer trace facility
#define configUSE_STATS_FORMATTING_FUNCTIONS    0       ///< Older FreeRTOS functions
#define INCLUDE_eTaskGetState                   1       ///< Was needed by the "info" command-line handler


/* Features config */
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         0
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_QUEUE_SETS                1
#define INCLUDE_vTaskPrioritySet			0
#define INCLUDE_uxTaskPriorityGet			0
#define INCLUDE_vTaskDelete					0   ///< A task should never be deleted in an RTOS (why create it only to delete it later?)
#define INCLUDE_vTaskCleanUpResources		0   ///< Ditto ^^
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_uxTaskGetStackHighWaterMark	1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_xTaskGetIdleTaskHandle      1

/* FreeRTOS Timer or daemon task configuration */
#define configUSE_TIMERS                0                   ///< Enable or disable the FreeRTOS timer task
#define configTIMER_TASK_PRIORITY       PRIORITY_HIGH       ///< Priority at which the timer task should run (use highest)
#define configTIMER_QUEUE_LENGTH        10                  ///< See FreeRTOS documentation
#define configTIMER_TASK_STACK_DEPTH    STACK_BYTES(2048)   ///< Stack size for the timer task
#define INCLUDE_xTimerPendFunctionCall  0                   ///< Uses timer daemon task, so needs configUSE_TIMERS to 1



/* Use the system definition, if there is one */
#ifdef __NVIC_PRIO_BITS
	#define configPRIO_BITS       __NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       5        /* 32 priority levels */
#endif

#include "lpc_isr.h"
/* The lowest priority. */
#define configKERNEL_INTERRUPT_PRIORITY 	    ( IP_KERNEL <<   (8 - configPRIO_BITS) )
/* Priority 5, or 160 as only the top three bits are implemented. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( IP_SYSCALL <<  (8 - configPRIO_BITS) )
/* ARM Cortex M3 has hardware instruction to count leading zeroes */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1



#if (0 == configUSE_TRACE_FACILITY)
    /*
    * If trace facility is enabled, also track the last running task.
    * We do this by copying the name of the last task that got switched in
    * to an auxiliary memory location.
    *
    * Poor man's trace just records the last task that was running before a potential system crash ;(
    */
    #include "fault_registers.h"
    #define traceTASK_SWITCHED_IN()                                                  \
                 do {                                                                \
                     uint32_t *pTaskName = (uint32_t*)(pxCurrentTCB->pcTaskName);    \
                     FAULT_LAST_RUNNING_TASK_NAME = *pTaskName;                      \
                 } while (0)

    #ifdef __cplusplus
    extern "C" {
    #endif
        void rts_not_full_trace_init( void );
        unsigned int rts_not_full_trace_get();
        void rts_not_full_trace_reset();
    #ifdef __cplusplus
    }
    #endif

    #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()    rts_not_full_trace_init()
    #define portGET_RUN_TIME_COUNTER_VALUE()            rts_not_full_trace_get()
    #define portRESET_TIMER_FOR_RUN_TIME_STATS()        rts_not_full_trace_reset()

    // Stub out macros for the trace facility to avoid conditional compilation everywhere
    #include "trace/trcUser.h"

// The real trace facility
#else
    unsigned trace_get_run_time_counter(void);
    #define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()    /* Sys tick is initted by vPortSetupTimerInterrupt() */
    #define portGET_RUN_TIME_COUNTER_VALUE()            trace_get_run_time_counter()
    #define portRESET_TIMER_FOR_RUN_TIME_STATS()        /* Resetting not supported */
    #include "trace/trcKernelPort.h"                    /* Must be included last */
#endif  /* (1 == configUSE_TRACE_FACILITY) */

#endif /* FREERTOS_CONFIG_H */
