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
#include <string.h>         // memset() etc.

#include "FreeRTOS.h"
#include "semphr.h"

#include "uart0.hpp"        // Interrupt driven UART0 driver
#include "nrf_stream.hpp"

#include "lpc_sys.h"        // Set input/output char functions
#include "utilities.h"      // PRINT_EXECUTION_SPEED()
#include "handlers.hpp"     // Command-line handlers

#include "file_logger.h"
#include "io.hpp"           // Board IO
#include "tasks.hpp"

#include "c_tlm_var.h"
#include "c_tlm_comp.h"
#include "c_tlm_stream.h"
#include "c_tlm_binary.h"



#define MAX_COMMANDLINE_INPUT   128              ///< Max characters for command-line input
#define CMD_TIMEOUT_DISK_VARS   (2 * 60 * 1000)  ///< Disk variables are saved if no command comes in for this duration



terminalTask::terminalTask(uint8_t priority) :
        scheduler_task("terminal", 1024*4, priority),
        mCmdIface(2), /* 2 interfaces can be added without memory reallocation */
        mCmdProc(24), /* 24 commands can be added without memory reallocation */
        mCommandCount(0), mDiskTlmSize(0), mpBinaryDiskTlm(NULL),
        mCmdTimer(CMD_TIMEOUT_DISK_VARS)
{
    /* Nothing to do */
}

bool terminalTask::regTlm(void)
{
    #if SYS_CFG_ENABLE_TLM
    return (TLM_REG_VAR(tlm_component_get_by_name(SYS_CFG_DEBUG_TLM_NAME), mCommandCount, tlm_uint) &&
            TLM_REG_VAR(tlm_component_get_by_name(SYS_CFG_DEBUG_TLM_NAME), mDiskTlmSize, tlm_uint));
    #else
    return true;
    #endif
}

bool terminalTask::taskEntry()
{
    /* remoteTask() creates shared object in its init(), so we can get it now */
    CommandProcessor &cp = mCmdProc;

    // System information handlers
    cp.addHandler(taskListHandler, "info",    "Task/CPU Info.  Use 'info 200' to get CPU during 200ms");
    cp.addHandler(memInfoHandler,  "meminfo", "See memory info");
    cp.addHandler(healthHandler,   "health",  "Output system health");
    cp.addHandler(timeHandler,     "time",    "'time' to view time.  'time set MM DD YYYY HH MM SS Wday' to set time");

    // File I/O handlers:
    cp.addHandler(catHandler,    "cat",   "Read a file.  Ex: 'cat 0:file.txt' or "
                                          "'cat 0:file.txt -noprint' to test if file can be read");
    cp.addHandler(cpHandler,     "cp",    "Copy files from/to Flash/SD Card.  Ex: 'cp 0:file.txt 1:file.txt'");
    cp.addHandler(dcpHandler,    "dcp",   "Copy all files of a directory to another directory.  Ex: 'dcp 0:src 1:dst'");
    cp.addHandler(lsHandler,     "ls",    "Use 'ls 0:' for Flash, or 'ls 1:' for SD Card");
    cp.addHandler(mkdirHandler,  "mkdir", "Create a directory. Ex: 'mkdir test'");
    cp.addHandler(mvHandler,     "mv",    "Rename a file. Ex: 'rm 0:file.txt 0:new.txt'");
    cp.addHandler(newFileHandler,"nf",    "Write a new file. Ex: 'nf <file.txt>");
    cp.addHandler(rmHandler,     "rm",    "Remove a file. Ex: 'rm 0:file.txt'");

    // Misc. handlers
    cp.addHandler(i2cIoHandler,   "i2c",   "'i2c read 0x01 0x02 <count>' : Reads <count> registers of device 0x01 starting from 0x02\n"
                                           "'i2c write 0x01 0x02 0x03'   : Writes 0x03 to device 0x01, reg 0x02\n"
                                           "'i2c discover' : Discovers all I2C devices on the BUS");
#if TERMINAL_USE_CAN_BUS_HANDLER
    CMD_HANDLER_FUNC(canBusHandler);
    cp.addHandler(canBusHandler,  "canbus", "'canbus init' : initialize CAN-1\n"
                                            "'canbus filter <id>' : Add 29-bit ID fitler\n"
                                            "'canbus tx <msg id> <len> <byte0> <byte1> ...' : Send CAN Message\n"
                                            "'canbus rx <timeout in ms>' : Receive a CAN message\n"
                                            "'canbus registers' : See some of CAN BUS registers");
#endif

    cp.addHandler(storageHandler,  "storage",  "Parameters: 'format sd', 'format flash', 'mount sd', 'mount flash'");
    cp.addHandler(rebootHandler,   "reboot",   "Reboots the system");
    cp.addHandler(logHandler,      "log",      "'log <hello>': log an info message\n"
                                               "'log flush'  : flush the logs\n"
                                               "'log status' : get status of the logger\n"
                                               "'log enable print debug/info/warn/error' : Enables logger calls to printf\n"
                                               "'log disable print debug/info/warn/error': Disables logger calls to printf\n"
                                               );
    cp.addHandler(learnIrHandler,  "learn",    "Begin to learn IR codes for numbers 0-9");
    cp.addHandler(wirelessHandler, "wireless", "Use 'wireless' to see the nested commands");

    /* Firmware upgrade handlers
     * Please read "netload_readme.txt" at ref_and_datasheets directory.
     */
    CMD_HANDLER_FUNC(getFileHandler);
    CMD_HANDLER_FUNC(flashProgHandler);
    cp.addHandler(getFileHandler,   "file",  "Get a file using netload.exe or by using the following protocol:\n"
                                             "Write buffer: buffer <offset> <num bytes> ...\n"
                                             "Write buffer to file: commit <filename> <file offset> <num bytes from buffer>");
    cp.addHandler(flashProgHandler, "flash", "'flash <filename>' Will flash CPU with this new binary file");

    #if (SYS_CFG_ENABLE_TLM)
    cp.addHandler(telemetryHandler, "telemetry", "Outputs registered telemetry: "
                                                 "'telemetry save' : Saves disk tel\n"
                                                 "'telemetry ascii' : Prints all telemetry in human readable format\n"
                                                 "'telemetry <comp. name> <name> <value>' to set a telemetry variable\n"
                                                 "'telemetry get <comp. name> <name>' to get variable value\n");
    #endif

    // Initialize Interrupt driven version of getchar & putchar
    Uart0& uart0 = Uart0::getInstance();
    bool success = uart0.init(SYS_CFG_UART0_BPS, 32, SYS_CFG_UART0_TXQ_SIZE);
    uart0.setReady(true);
    sys_set_inchar_func(uart0.getcharIntrDriven);
    sys_set_outchar_func(uart0.putcharIntrDriven);

    /* Add UART0 to command input/output */
    addCommandChannel(&uart0, true);

    #if TERMINAL_USE_NRF_WIRELESS
    do {
        NordicStream& nrf = NordicStream::getInstance();
        nrf.setReady(true);
        addCommandChannel(&nrf, false);
    } while(0);
    #endif

    #if SYS_CFG_ENABLE_TLM
    /* Telemetry should be registered at this point, so initialize the binary
     * telemetry space that we periodically check to save data to disk
     */
    tlm_component *disk = tlm_component_get_by_name(SYS_CFG_DISK_TLM_NAME);
    mDiskTlmSize = tlm_binary_get_size_one(disk);
    mpBinaryDiskTlm = new char[mDiskTlmSize];
    if (success) {
        success = (NULL != mpBinaryDiskTlm);
    }

    /* Now update our copy of disk telemetry */
    tlm_binary_get_one(disk, mpBinaryDiskTlm);
    #endif

    /* Display "help" command on UART0 */
    STR_ON_STACK(help, 8);
    help = "help";
    mCmdProc.handleCommand(help, uart0);

    return success;
}

bool terminalTask::run(void* p)
{
    printf("LPC: ");
    cmdChan_t cmdChannel = getCommand();

    // If no command, try to save disk data (persistent variables)
    if (!cmdChannel.iodev) {
        if (saveDiskTlm()) {
            /* Disk variables saved to disk */
        }
        else {
            puts("");
        }
    }
    else {
        // Set our references to the IO channel and the command str (just for covenience sake)
        CharDev& io = *(cmdChannel.iodev);
        str& cmd = *(cmdChannel.cmdstr);

        if (cmd.getLen() > 0)
        {
            PRINT_EXECUTION_SPEED()
            {
                ++mCommandCount;
                mCmdProc.handleCommand(cmd, io);

                /* Send special chars to indicate end of command output
                 * Usually, serial terminals will ignore these chars
                 */
                const char endOfTx[] = TERMINAL_END_CHARS;
                for (unsigned i = 0; i < sizeof(endOfTx); i++) {
                    io.putChar(endOfTx[i]);
                }
            }

            cmd.clear();
            io.flush();
        }
    }

    return true;
}

bool terminalTask::saveDiskTlm(void)
{
    bool changed = false;

    #if SYS_CFG_ENABLE_TLM
    tlm_component *disk = tlm_component_get_by_name(SYS_CFG_DISK_TLM_NAME);

    /* Size of telemetry shouldn't change */
    if (0 == mDiskTlmSize || mDiskTlmSize != tlm_binary_get_size_one(disk)) {
        return changed;
    }

    if (!tlm_binary_compare_one(disk, mpBinaryDiskTlm))
    {
        changed = true;
        puts("Disk variables changed...");

        FILE *file = fopen(SYS_CFG_DISK_TLM_NAME, "w");
        if (file) {
            // Only update variables if we could open the file
            tlm_binary_get_one(disk, mpBinaryDiskTlm);

            tlm_stream_one_file(disk, file);
            fclose(file);

            puts("Changes saved to disk...");
            LOG_SIMPLE_MSG("Disk variables saved to disk");
        }
    }
    #endif

    return changed;
}


void terminalTask::handleEchoAndBackspace(cmdChan_t *io, char newChar)
{
    /* Pointers to reduce too many -> references */
    const bool echo = io->echo;
    str *pStr = io->cmdstr;
    CharDev *iodev = io->iodev;

    // Backspace 1 char @ terminal and erase last char of string
    if (echo && '\b' == newChar) {
        if(pStr->getLen() > 0) {
            iodev->put("\b ");
            pStr->eraseLast(1);
        }
        else {
            // Send "Alert" sound to terminal because we can't backspace
            const char bellSound = '\a';
            newChar = bellSound;
        }
    }
    else if ('\n' != newChar && '\r' != newChar) {
        *(pStr) += newChar;
    }

    if (echo) {
        iodev->putChar(newChar);
    }
}

void terminalTask::addCommandChannel(CharDev *channel, bool echo)
{
    cmdChan_t input;
    input.iodev = channel;
    input.echo = echo;
    input.cmdstr = new str(MAX_COMMANDLINE_INPUT);
    mCmdIface += input;
}

terminalTask::cmdChan_t terminalTask::getCommand(void)
{
    if (0 == mCmdIface.size()) {
        vTaskDelayMs(1000);
        cmdChan_t noIface = { NULL, NULL };
        return noIface;
    }

    unsigned int idx = 0;
    cmdChan_t ret = mCmdIface[0];
    char c = 0;

    do
    {
        /* Get a single char from one of the input sources */
        const TickType_t ticksBefore = xTaskGetTickCount();
        bool gotChar = false;

        for (idx = 0; idx < mCmdIface.size(); idx++)
        {
            if (mCmdIface[idx].iodev->isReady() && mCmdIface[idx].iodev->getChar(&c, 0))
            {
                ret = mCmdIface[idx];
                handleEchoAndBackspace(&ret, c);

                mCmdTimer.reset();
                gotChar = true;
                break;
            }
        }

        /* If no interfaces are ready, we will not sleep in the previous loop,
         * so we don't want to hog the CPU, so just delay here by one tick
         */
        if (!gotChar && xTaskGetTickCount() == ticksBefore) {
            vTaskDelay(2);
        }

        /* Guard against command length too large */
        if (ret.cmdstr->getLen() >= ret.cmdstr->getCapacity() - 1) {
            break;
        }

        /* If no command and timer expires, then use this time to do something else
         * and return back to the caller but set iodev to NULL
         */
        if (mCmdTimer.expired()) {
            mCmdTimer.reset();
            ret.iodev = NULL;
            break;
        }
    } while (c != '\n');

    return ret;
}
