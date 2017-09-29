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
 * @brief This file provides the configurable parameters for your project.
 */
#ifndef SYSCONFIG_H_
#define SYSCONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif



/** @{ Nordic wireless configuration
 * More settings can be configured at mesh_config.h
 * @warning
 *      The AIR data rate, and channel number must be consistent for your wireless
 *      nodes to talk to each other.  It seems that 2000kbps works better than 250kbps
 *      although slower data rate is supposed to get longer range.
 *
 * @warning Go to   "PROJECT" --> "Clean"   if you change the settings here.
 */
#define WIRELESS_NODE_ADDR              106    ///< Any value from 1-254
#define WIRELESS_CHANNEL_NUM            2499   ///< 2402 - 2500 to avoid collisions among 2+ mesh networks
#define WIRELESS_AIR_DATARATE_KBPS      2000   ///< Air data rate, can only be 250, 1000, or 2000 kbps
#define WIRELESS_NODE_NAME             "node"  ///< Wireless node name (ping response contains this name)
#define WIRELESS_RX_QUEUE_SIZE          3      ///< Number of payloads we can queue
#define WIRELESS_NODE_ADDR_FILE         "naddr"///< Node address can be read from this file and this can override WIRELESS_NODE_ADDR
/** @} */



#define TERMINAL_USE_NRF_WIRELESS       0             ///< Terminal command can be sent through nordic wireless
#define TERMINAL_END_CHARS              {3, 3, 4, 4}  ///< The last characters sent after processing a terminal command
#define TERMINAL_USE_CAN_BUS_HANDLER    0             ///< CAN bus terminal command



#define SYS_CFG_SPI1_CLK_MHZ            24          ///< Max speed of SPI1 for SD Card and Flash memory
#define SYS_CFG_SPI0_CLK_MHZ            8           ///< Nordic wireless requires 1-8Mhz max
#define SYS_CFG_I2C2_CLK_KHZ            100         ///< 100Khz is standard I2C speed

/// If defined, a boot message is logged to this file
//#define SYS_CFG_LOG_BOOT_INFO_FILENAME        "boot.csv"

#define SYS_CFG_STARTUP_DELAY_MS        2000        ///< Start-up delay in milliseconds
#define SYS_CFG_CRASH_STARTUP_DELAY_MS  5000        ///< Start-up delay in milliseconds if a crash occurred previously.
#define SYS_CFG_INITIALIZE_LOGGER       1           ///< If non-zero, the logger is initialized (@see file_logger.h)
#define SYS_CFG_LOGGER_TASK_PRIORITY    1           ///< The priority of the logger task (do not use 0, logger will run into issues while writing the file)
#define SYS_CFG_ENABLE_TLM              1           ///< Enable telemetry system. C_FILE_IO forced enabled if enabled
#define SYS_CFG_DISK_TLM_NAME           "disk"      ///< Filename to save "disk" telemetry variables
#define SYS_CFG_DEBUG_TLM_NAME          "debug"     ///< Name of the debug telemetry component
#define SYS_CFG_ENABLE_CFILE_IO         0           ///< Allow stdio fopen() fclose() to redirect to ff.h
#define SYS_CFG_MAX_FILES_OPENED        3           ///< Maximum files that can be opened at once



/**
 * Define the timer that will be used to run the background timer service. This drives the
 * lpc_sys_get_uptime_ms(), lpc_sys_get_uptime_us() and periodically resets the watchdog timer
 * along with running the mesh networking task if FreeRTOS is running.
 *
 * Timer 1 is required if you wish to have an operational IR remote control decoding.
 */
#define SYS_CFG_SYS_TIMER               1

/**
 * Watchdog timeout in milliseconds
 * Value cannot be greater than 1,000,000 which is too large of a value
 * to set for a useful watchdog timer anyway.
 */
#define SYS_CFG_WATCHDOG_TIMEOUT_MS     (3 * 1000)

/**
 * @returns actual System clock as calculated from PLL and Oscillator selection
 * @note The SYS_CFG_DESIRED_CPU_CLK macro defines "Desired" CPU clock, and doesn't guarantee
 *          this clock rate.  This function returns actual CPU clock of the system.
 */
unsigned int sys_get_cpu_clock();


/**
 * @{   Select the clock source:
 * - Internal Clock: 4Mhz  1% Tolerance
 * - External Clock: External Crystal
 * - RTC Clock     : 32.768Khz
 *
 * If the RTC clock is chosen as an input, then sys_clock.cpp will use the closest
 * PLL settings to get you the desired clock rate.  Due to PLL calculations, the
 * RTC PLL setting may delay your startup time so be patient.
 * 36864000 (36.864Mhz) is a good frequency to derive from RTC PLL since it
 * offers a perfect UART divider.
 */
#define CLOCK_SOURCE_INTERNAL   0                       ///< Just a constant, do not change
#define CLOCK_SOURCE_EXTERNAL   1                       ///< Just a constant, do not change
#define CLOCK_SOURCE_RTC        2                       ///< Just a constant, do not change
#define SYS_CFG_CLOCK_SOURCE    CLOCK_SOURCE_INTERNAL   ///< Select the clock source from above
/** @} */

#define INTERNAL_CLOCK		    (4  * 1000 * 1000UL)    ///< Do not change, this is the same on all LPC17XX
#define EXTERNAL_CLOCK          (12 * 1000 * 1000UL)    ///< Change according to your board specification
#define RTC_CLOCK               (32768UL)               ///< Do not change, this is the typical RTC crystal value

#define SYS_CFG_DESIRED_CPU_CLK	(48 * 1000 * 1000UL)    ///< Define the CPU speed you desire, must be between 1-100Mhz
#define SYS_CFG_DEFAULT_CPU_CLK (24 * 1000 * 1000UL)    ///< Do not change.  This is the fall-back CPU speed if SYS_CFG_DESIRED_CPU_CLK cannot be attained



/**
 * @{ Set printf & scanf options - Do a clean build after changing this option
 *
 *  - 0 : Full printf from stdio.h --> Supports floating-point, but uses 15K more
 *        flash memory and about 300 bytes more RAM with newlib nano libraries.
 *  - 1 : printf from stdio.h without floating point printf/scanf
 */
#define SYS_CFG_REDUCED_PRINTF      0     ///< If non-zero, floating-point printf() and scanf() is not supported
#define SYS_CFG_UART0_BPS           38400 ///< UART0 is configured at this BPS by start-up code - before main()
#define SYS_CFG_UART0_TXQ_SIZE      256   ///< UART0 transmit queue size before blocking starts to occur
/** @} */



/**
 * Valid years for RTC.
 * If RTC year is not found to be in between these, RTC will reset to 1/1/2000 00:00:00
 */
#define SYS_CFG_RTC_VALID_YEARS_RANGE   {2010, 2025}



/**
 * Do not change anything here, Telemetry C-File I/O is force enabled if telemetry system is in use.
 */
#if (SYS_CFG_ENABLE_TLM)
#undef SYS_CFG_ENABLE_CFILE_IO
#define SYS_CFG_ENABLE_CFILE_IO 1
#endif



#ifdef __cplusplus
}
#endif
#endif /* SYSCONFIG_H_ */
