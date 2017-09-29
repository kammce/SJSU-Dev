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
 * @brief  Provides the following system services :
 *              - Memory info of the system.
 *              - Configure function to use for printf/scanf
 *              - Get boot type and time
 *              - Watchdog
 *
 * 12082013 : Added functionality to see next heap pointer from mem_info_t
 */
#ifndef LPC_SYS_H__
#define LPC_SYS_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "LPC17xx.h"
#include "fault_registers.h"
#include "rtc.h"
#include "sys_config.h"



/**
 * Enumeration of the reboot type
 */
typedef enum {
    boot_unknown  = 0,
    boot_power_on = 1,      ///< Cold boot (power on)
    boot_reset    = 2,      ///< Boot after reset condition
    boot_watchdog = 4,      ///< Boot after watchdog reset (intentional)
    boot_watchdog_recover,  ///< Boot after watchdog reset after an error (or crash)
    boot_brown_out          ///< Boot after under-voltage
} sys_boot_t;

/**
 * This is the memory structure that is returned from sys_get_mem_info()
 * Heap memory obtains pool of memory from System, so the memory pool
 * not obtained by the Heap is listed by the systemAvailable variable.
 */
typedef struct
{
    uint32_t used_global; ///< Global Memory allocated
    uint32_t used_heap;   ///< Memory granted by Heap (malloc, new etc.)
    uint32_t avail_heap;  ///< Memory available at Heap
    uint32_t avail_sys;   ///< Memory available to Heap (from sbrk function)

    uint32_t num_sbrk_calls;  ///< Number of calls to the sbrk() function
    uint32_t last_sbrk_size;  ///< Last size requested from the sbrk() function
    void*    last_sbrk_ptr;   ///< Last pointer given by the sbrk() function
    void*    next_malloc_ptr; ///< The next pointer that will be returned to malloc() from sbrk()
} sys_mem_t;

/** Void function pointer */
typedef void (*void_func_t)(void);

/** Function pointer of a function returning a char and taking a char as parameter */
typedef char (*char_func_t)(char);



/** @{ Defined at system_init.c */
sys_boot_t sys_get_boot_type();  ///< @returns the reboot reason as detected during system startup
rtc_t      sys_get_boot_time();  ///< @returns the boot-time recorded in the system
/** @} */



/** @{ Defined at syscalls.c */
/**
 * Sets the function used to output a char by printf() or stdio output functions
 * @param func  The function pointer to use to output a char
 */
void sys_set_outchar_func(char_func_t func);

/**
 * Sets the function used to input a char by scanf() or stdio input functions
 * @param func  The function pointer to use to get a char
 */
void sys_set_inchar_func(char_func_t func);
/** @} */



/**
 * Sets up the system timer that drives the time needed to get uptime in ms and us
 * along with some background services.
 * @see SYS_CFG_SYS_TIMER at sys_config.h
 */
void lpc_sys_setup_system_timer(void);

/// @returns the system up time in microseconds
uint64_t sys_get_uptime_us(void);

/// @returns the system up time in milliseconds.
static inline uint64_t sys_get_uptime_ms(void) { return sys_get_uptime_us() / 1000; }



/**
 * Gets System Memory information
 * The information includes Global Memory usage, and dynamic memory usage.
 * This function is defined at memory.cpp
 * @returns MemoryInfoType structure
 */
sys_mem_t sys_get_mem_info();

/**
 * Prints memory information to the given buffer
 * The buffer needs to be at least 280 bytes
 */
void sys_get_mem_info_str(char buffer[280]);



/**
 * Watchdog feed resets the watchdog timeout.
 * Periodic feeds should keep the system running, if the  watchdog
 * feeds stop, system will reset after watchdog timeout.
 */
static inline void sys_watchdog_feed()
{
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDFEED = 0x55;
}

/**
 * Resets the system immediately.
 */
static inline void sys_reboot()
{
    /**
     * From the datasheet:
     * After writing 0xAA to WDFEED, access to any Watchdog register other than writing
     * 0x55 to WDFEED causes an immediate reset/interrupt when the Watchdog is enabled.
     */
    LPC_WDT->WDFEED = 0xAA;
    LPC_WDT->WDMOD = 0;
}

/**
 * Restarts the system immediately and marks abnormal restart
 */
static inline void sys_reboot_abnormal(void)
{
    FAULT_EXISTS = FAULT_PRESENT_VAL;
    sys_reboot();
}

/**
 * Enables watchdog with reset mode according to SYS_CFG_WATCHDOG_TIMEOUT_MS macro's value
 */
static inline void sys_watchdog_enable()
{
    /**
     * 1 / (4Mhz/4) = Watchdog clock by default (WCLK)
     * Therefore watchdog clock = 1Mhz = 1uS
     * Each value of WDTC means 1uS
     */
    LPC_WDT->WDTC = SYS_CFG_WATCHDOG_TIMEOUT_MS * 1000;

    /**
     * Enable Watchdog and Watchdog reset enable
     * Once enabled, this cannot be cleared
     */
    #if(DEBUG)
        LPC_WDT->WDMOD = 2;
    #else
        LPC_WDT->WDMOD = 3;
    #endif
    sys_watchdog_feed();
}



#ifdef __cplusplus
}
#endif
#endif /* LPC_SYS_H__ */
