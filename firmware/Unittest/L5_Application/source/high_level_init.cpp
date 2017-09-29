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

#include <stdio.h>          // printf
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "lpc_sys.h"
#include "sys_config.h"     // CPU Clock Configuration
#include "utilities.h"
#include "chip_info.h"

#include "fat/disk/sd.h"        // Initialize SD Card Pins for CS, WP, and CD
#include "fat/disk/spi_flash.h" // Initialize Flash CS pin

#include "rtc.h"             // RTC init
#include "i2c2.hpp"          // I2C2 init
#include "adc0.h"            // ADC0 init
#include "ssp0.h"            // SPI-0 init
#include "ssp1.h"            // SPI-1 init

#include "file_logger.h"
#include "storage.hpp"       // Mount Flash & SD Storage
#include "bio.h"             // Init io signals
#include "io.hpp"            // Board IO peripherals

#include "wireless.h"
#include "fault_registers.h"
#include "c_tlm_comp.h"



/// Just prints a line to separate the output printed by high level initialization
static void hl_print_line() { puts("----------------------------------------------------------"); }

/**
 * Mounts the storage drive.
 * @param [in] drive     The reference to the drive to mount
 * @param [in] pDescStr  The description of the storage to print out while mounting
 * @returns true if the drive was successfully mounted
 */
static bool hl_mount_storage(FileSystemObject& drive, const char* pDescStr);

/// Prints out the boot information
static void hl_print_boot_info(void);

/// Initializes the board input/output devices and returns true if successful
static bool hl_init_board_io(void);

/// Reads the node address file and sets the wireless node address from the file data
static void hl_wireless_set_addr_from_file(void);

/**
 * Prints out board ID information and if no board ID is found, gives option
 * to the user to program the permanent board ID
 */
static void hl_handle_board_id(void);

/// Prints out the board programming info (how many times the board was programmed etc.)
static void hl_show_prog_info(void);



/**
 * Initializes the High Level System such as IO Pins and Drivers
 */
void high_level_init(void)
{
    // Initialize all board pins (early) that are connected internally.
    board_io_pins_initialize();

    /**
     * Initialize the Peripherals used in the system.
     * I2C2 : Used by LED Display, Acceleration Sensor, Temperature Sensor
     * ADC0 : Used by Light Sensor
     * SPI0 : Used by Nordic
     * SPI1 : Used by SD Card & External SPI Flash Memory
     */
    adc0_init();
    ssp1_init();
    ssp0_init(SYS_CFG_SPI0_CLK_MHZ);

    if (!I2C2::getInstance().init(SYS_CFG_I2C2_CLK_KHZ)) {
        puts("ERROR: Possible short on SDA or SCL wire (I2C2)!");
    }

    /**
     * This timer does several things:
     *      - Makes delay_ms() and delay_us() methods functional.
     *      - Provides run-time counter for FreeRTOS task statistics (cpu usage)
     *      - Provides timer needed by SD card init() and mesh network: nordic driver uses delay_us()
     *
     * The slightly tricky part is that the mesh networking task will start to be serviced every
     * one millisecond, so initialize this and immediately initialize the wireless (mesh/nordic)
     * so by the time 1ms elapses, the pointers are initialized (and not NULL).
     */
    lpc_sys_setup_system_timer();

    /* After the Nordic SPI is initialized, initialize the wireless system asap otherwise
     * the background task may access NULL pointers of the mesh networking task.
     *
     * @warning Need SSP0 init before initializing nordic wireless.
     * @warning Nordic uses timer delay, so we need the timer setup.
     */
    if (!wireless_init()) {
        puts("ERROR: Failed to initialize wireless");
    }

    /* Add default telemetry components if telemetry is enabled */
    #if SYS_CFG_ENABLE_TLM
        tlm_component_add(SYS_CFG_DISK_TLM_NAME);
        tlm_component_add(SYS_CFG_DEBUG_TLM_NAME);
    #endif

    /**
     * User configured startup delay to close Hyperload COM port and re-open it at Hercules serial window.
     * Since the timer is setup that is now resetting the watchdog, we can delay without a problem.
     */
    delay_ms(SYS_CFG_STARTUP_DELAY_MS);
    hl_print_line();

    /* Print out CPU speed and what caused the system boot */
    hl_print_boot_info();

    /**
     * If Flash is not mounted, it is probably a new board and the flash is not
     * formatted so format it, so alert the user, and try to re-mount it.
     */
    if (!hl_mount_storage(Storage::getFlashDrive(), " Flash "))
    {
        printf("Erasing and formatting SPI flash, this can take a while ... ");

        flash_chip_erase();
        printf("%s\n", FR_OK == Storage::getFlashDrive().format() ? "Done" : "Error");

        if (!hl_mount_storage(Storage::getFlashDrive(), " Flash "))
        {
            printf("SPI FLASH is possibly damaged!\n");
            printf("Page size: %u\n", (unsigned) flash_get_page_size());
            printf("Mem  size: %u (raw bytes)\n", (unsigned) (flash_get_page_count() * flash_get_page_size()));
        }
    }

    hl_mount_storage(Storage::getSDDrive(), "SD Card");

	/* SD card initialization modifies the SPI speed, so after it has been initialized, reset desired speed for spi1 */
    ssp1_set_max_clock(SYS_CFG_SPI1_CLK_MHZ);
    hl_print_line();

    /* Initialize all sensors of this board and display "--" on the LED display if an error has occurred. */
    if(!hl_init_board_io()) {
        hl_print_line();
        LD.setLeftDigit('-');
        LD.setRightDigit('-');
        LE.setAll(0xFF);
    }
    else {
        LD.setNumber(TS.getFarenheit());
    }

    /* After Flash memory is mounted, try to set node address from a file */
    hl_wireless_set_addr_from_file();

    /* Feed the random seed to get random numbers from the rand() function */
    srand(LS.getRawValue() + time(NULL));

    /* Print memory information before we call main() */
    do {
        char buff[512] = { 0 };
        sys_get_mem_info_str(buff);
        printf("%s", buff);
        hl_print_line();
    } while(0);

    /* Print miscellaneous info */
    hl_handle_board_id();
    hl_show_prog_info();
    hl_print_line();

    /* File I/O is up, so log the boot message if chosen by the user */
    #ifdef SYS_CFG_LOG_BOOT_INFO_FILENAME
    log_boot_info(__DATE__);
    #endif

    /* File I/O is up, so initialize the logger if user chose the option */
    #if SYS_CFG_INITIALIZE_LOGGER
    logger_init(SYS_CFG_LOGGER_TASK_PRIORITY);
    #endif

    /* and finally ... call the user's main() method */
    puts("Calling your main()");
    hl_print_line();
}

static bool hl_mount_storage(FileSystemObject& drive, const char* pDescStr)
{
    unsigned int totalKb = 0;
    unsigned int availKb = 0;
    const char st = drive.mount();
    bool mounted = (0 == st);

    if(mounted && FR_OK == drive.getDriveInfo(&totalKb, &availKb))
    {
        const unsigned int maxBytesForKbRange = (32 * 1024);
        const char *size = (totalKb < maxBytesForKbRange) ? "KB" : "MB";
        unsigned int div = (totalKb < maxBytesForKbRange) ? 1 : 1024;

        printf("%s: OK -- Capacity %-5d%s, Available: %-5u%s\n",
               pDescStr, totalKb/div, size, availKb/div, size);
    }
    else {
        printf("%s: Error or not present.  Error #%i, Mounted: %s\n", pDescStr, st, mounted ? "Yes" : "No");
        mounted = false;
    }

    return mounted;
}

static void hl_print_boot_info(void)
{
    /* Print boot info regardless of the printf options (if it prints float or not) */
    #if SYS_CFG_REDUCED_PRINTF
        const unsigned int cpuClock = sys_get_cpu_clock();
        const unsigned int sig = cpuClock / (1000 * 1000);
        const unsigned int fraction = (cpuClock - (sig*1000*1000)) / 1000;
        printf("System Boot @ %u.%u Mhz\n", sig, fraction);
    #else
        printf("System Boot @ %.3f Mhz\n", sys_get_cpu_clock() / (1000 * 1000.0f));
    #endif

    if(boot_watchdog_recover == sys_get_boot_type()) {
        char taskName[sizeof(FAULT_LAST_RUNNING_TASK_NAME) * 2] = { 0 };
        memcpy(&taskName[0], (void*) &(FAULT_LAST_RUNNING_TASK_NAME), sizeof(FAULT_LAST_RUNNING_TASK_NAME));

        hl_print_line();
        printf("System rebooted after crash.  Relevant info:\n"
               "PC: 0x%08X.  LR: 0x%08X.  PSR: 0x%08X\n"
               "Possible last running OS Task: '%s'\n",
                (unsigned int)FAULT_PC, (unsigned int)FAULT_LR, (unsigned int)FAULT_PSR,
                taskName);
        hl_print_line();
        delay_ms(SYS_CFG_CRASH_STARTUP_DELAY_MS);
    }
}

static bool hl_init_board_io(void)
{
    bool success = true;

    if(!AS.init()) { puts("ERROR: Acceleration Sensor"); success = false; }
    if(!TS.init()) { puts("ERROR: Temperature Sensor"); success = false; }
    if(!LD.init()) { puts("ERROR: 7-Segment Display"); success = false; }

    /* These devices don't have a way to check if init() failed */
    IS.init(); // IR sensor
    LS.init(); // Light sensor
    LE.init(); // LEDs
    SW.init(); // Switches

    /* Turn off all LEDs */
    LE.setAll(0);

    return success;
}

static void hl_wireless_set_addr_from_file(void)
{
    uint8_t wireless_node_addr = WIRELESS_NODE_ADDR;
    char nAddrStr[16] = { 0 };

    if (FR_OK == Storage::read(WIRELESS_NODE_ADDR_FILE, nAddrStr, sizeof(nAddrStr)-1, 0)) {
        wireless_node_addr = atoi(nAddrStr);
        bool ok = mesh_set_node_address(wireless_node_addr);
        printf("Set wireless node address to %i from '%s' file: %s\n",
                    wireless_node_addr, WIRELESS_NODE_ADDR_FILE, ok ? "Done!" : "FAILED");
    }
}

static void hl_handle_board_id(void)
{
    const uint8_t buttons_to_program_id = (1 << 3) | (1 << 0);
    const char notProgrammed = 0xFF;
    char board_id_on_spi_flash[128] = { 0 };
    flash_read_permanent_id(board_id_on_spi_flash);

    if (notProgrammed != board_id_on_spi_flash[0]) {
        printf("Board ID is: '%s' (0x%02X)\n", board_id_on_spi_flash, (board_id_on_spi_flash[0] & 0xFF));
    }
    else if (SW.getSwitchValues() == buttons_to_program_id) {
        printf("Enter a board ID (64 chars max): \n");
        scanf("%64s", &board_id_on_spi_flash[0]);

        printf("Board ID to program: '%s'\n", board_id_on_spi_flash);
        printf("Enter 'Y' to confirm.  BOARD ID CANNOT BE CHANGED AND IS PERMANENT\n");

        char confirm[4] = { 0 };
        scanf("%3s", confirm);

        if ('Y' == confirm[0]) {
            flash_write_permanent_id(board_id_on_spi_flash);
        }
        else {
            puts("Board ID not programmed");
        }
    }
    else {
        puts("You can program a PERMANENT ID of your board.");
        puts("To do this, hold SW1 and SW4 and reset the board.");
    }
}

static void hl_show_prog_info(void)
{
    const unsigned int prog_count   = chip_get_prog_count();
    const unsigned int prog_modify  = chip_get_prog_modify_count();
    const unsigned int prog_max_kb  = chip_get_prog_max() / 1024;
    const unsigned int prog_min_kb  = chip_get_prog_min() / 1024;
    const unsigned int prog_time_ms = chip_get_prog_time_ms();

    printf("CPU flash altered/programmed counts: %u/%u\n", prog_modify, prog_count);
    printf("CPU programmed flash (min/max): %uKb - %uKb\n", prog_min_kb, prog_max_kb);
    printf("Last programming took %u ms\n", prog_time_ms);
}
