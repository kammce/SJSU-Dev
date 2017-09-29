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

#include <stdlib.h>   // malloc()
#include <stdio.h>    // sprintf()
#include <string.h>   // strlen()
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "file_logger.h"
#include "lpc_sys.h"
#include "rtc.h"
#include "ff.h"



#if (FILE_LOGGER_KEEP_FILE_OPEN)
static FIL *gp_file_ptr = NULL;                     ///< The pointer to the file object
#endif

static uint16_t g_blocked_calls = 0;                ///< Number of logging calls that blocked
static uint16_t g_buffer_watermark = 0;             ///< The watermark of the number of buffers consumed
static uint16_t g_highest_file_write_time = 0;      ///< Highest time spend while trying to write file buffer
static char * gp_file_buffer = NULL;                ///< Pointer to local buffer space before it is written to file
static QueueHandle_t g_write_buffer_queue = NULL;   ///< Log message pointers are written to this queue
static QueueHandle_t g_empty_buffer_queue = NULL;   ///< Log message pointers are available from this queue
static uint32_t g_logger_calls[log_last] = { 0 };   ///< Number of logged messages of each severity

/**
 * Chooses severity levels that are printed on stdio and logged
 * By default, the debug log will be printed to stdio
 */
static uint8_t g_logger_printf_mask = (1 << log_debug);

/**
 * Writes the buffer to the file.
 * @param [in] buffer   The data pointer to write from
 * @param [in] bytes_to_write  The number of bytes to write
 */
static bool logger_write_to_file(const void * buffer, const uint32_t bytes_to_write)
{
    bool success = false;
    FRESULT err = 0;
    UINT bytes_written = 0;
    const UINT bytes_to_write_uint = bytes_to_write;
    const uint32_t start_time = sys_get_uptime_ms();

    #if (!FILE_LOGGER_KEEP_FILE_OPEN)
    FIL fatfs_file = { 0 };
    #endif

    if (0 == bytes_to_write_uint) {
        success = true;
    }
    /* File already open, so just write the data */
    #if (FILE_LOGGER_KEEP_FILE_OPEN)
    else if (FR_OK == (err = f_write(gp_file_ptr, buffer, bytes_to_write_uint, &bytes_written)))
    {
        f_sync(gp_file_ptr);
    }
    #else
    /* File not opened, open it, seek it, and then write it */
    else if(FR_OK == (err = f_open(&fatfs_file, FILE_LOGGER_FILENAME, FA_OPEN_ALWAYS | FA_WRITE)))
    {
        if (FR_OK == (err = f_lseek(&fatfs_file, f_size(&fatfs_file))))
        {
            err = f_write(&fatfs_file, buffer, bytes_to_write_uint, &bytes_written);
        }
        f_close(&fatfs_file);
    }
    #endif
    else {
        printf("Failed file write: ");
    }

    /* Capture the time */
    const uint32_t diff_time = sys_get_uptime_ms() - start_time;
    if (diff_time > g_highest_file_write_time) {
        g_highest_file_write_time = diff_time;
    }

    /* To be successful, bytes written should be the same count as the bytes intended to be written */
    success = (bytes_to_write_uint == bytes_written);

    /* We don't want to silently fail, so print a message in case an error occurs */
    if (!success) {
        printf("Error %u writing logfile. %u/%u written. Fptr: %u\n",
#if (FILE_LOGGER_KEEP_FILE_OPEN)
                (unsigned)err, (unsigned)bytes_written, (unsigned)bytes_to_write, (unsigned) gp_file_ptr->fptr);
#else
                (unsigned)err, (unsigned)bytes_written, (unsigned)bytes_to_write, (unsigned) fatfs_file.fptr);
#endif
    }

    return success;
}

/**
 * @returns a logger buffer pointer
 * @param [in] os_running If FreeRTOS is running, this will grab from the queue while potentially blocking
 *             otherwise it will return a buffer pointer without blocking since buffer will always be
 *             available if OS is not running since no multi-threaded operation is going on.
 */
static char * logger_get_buffer_ptr(const bool os_running)
{
    char * buffer = NULL;

    /* Get an available buffer to write the data to, and if OS is not running, we will always get a buffer */
    if (!os_running) {
        xQueueReceive(g_empty_buffer_queue, &buffer, 0);
    }
    else if (!xQueueReceive(g_empty_buffer_queue, &buffer, OS_MS(FILE_LOGGER_BLOCK_TIME_MS))) {
        ++g_blocked_calls;

        /* This time, just block forever until we get a buffer */
        xQueueReceive(g_empty_buffer_queue, &buffer, portMAX_DELAY);
    }

    return buffer;
}

/**
 * Writes the buffer pointer to either file buffer, or directly to file if OS is not running
 * @param [in] buffer The buffer pointer to write
 * @param [in] os_running If OS is running, it will just queue the pointer, and not write directly to the file.
 */
static void logger_write_log_message(char * buffer, const bool os_running)
{
    /* Send the buffer to the queue for the logger task to write */
    if (os_running) {
        xQueueSend(g_write_buffer_queue, &buffer, portMAX_DELAY);
    }
    /* No logging task to write the data, so we need to do it ourselves */
    else {
        size_t len = strlen(buffer);
        buffer[len] = '\n';
        buffer[++len] = '\0';
        logger_write_to_file(buffer, len);

        /* OS not running, so put the buffer back to the empty queue instead of write queue */
        xQueueSend(g_empty_buffer_queue, &buffer, 0);
    }
}

/**
 * This is the actual FreeRTOS logger task responsible for:
 *      - Retrieve a log message written to the write queue
 *      - Copy it to our local buffer
 *      - If local buffer is full, write it to the file
 *      - Put-back the log message buffer to available queue
 */
static void logger_task(void *p)
{
    /* Use "char * const" to disallow this pointer to be moved.
     * We don't want to use "const char *" because we do want to be able to
     * write to this pointer instead of using excessive casts later.
     */
    char * const start_ptr = gp_file_buffer;
    char * const end_ptr = start_ptr + FILE_LOGGER_BUFFER_SIZE;

    char * log_msg = NULL;
    char * write_ptr = start_ptr;
    size_t len = 0;
    size_t buffer_overflow_cnt = 0;

    while (1)
    {
        /* Receive the log message we wish to write to our buffer.
         * Timeout or NULL pointer received is the signal to flush the data.
         */
        log_msg = NULL;
        if (!xQueueReceive(g_write_buffer_queue, &log_msg, OS_MS(1000 * FILE_LOGGER_FLUSH_TIME_SEC)) ||
            NULL == log_msg)
        {
            logger_write_to_file(start_ptr, (write_ptr - start_ptr));
            write_ptr = start_ptr;
            continue;
        }

        /* Update the watermark of the number of messages we see in the queue and note that we
         * add one to account for the message we just dequeued.
         */
        if ((len = (1 + uxQueueMessagesWaiting(g_write_buffer_queue)) ) > g_buffer_watermark) {
            g_buffer_watermark = len;
        }

        /* Get the length and append the newline character */
        len = strlen(log_msg);
        log_msg[len] = '\n';
        log_msg[++len] = '\0';

        /* This is test code to immediately write data to the file.  It is highly in-efficient and is
         * included here just for reference or test purposes.
         */
        #if 0
        {
            logger_write_to_file(log_msg, len);
            xQueueSend(g_empty_buffer_queue, &log_msg, portMAX_DELAY);
            continue;
        }
        #endif

        /* If we will overflow our buffer we need to write the full buffer and do partial copy */
        if (len + write_ptr >= end_ptr)
        {
            /* This could be zero when we write the last byte in the buffer */
            buffer_overflow_cnt = (len + write_ptr - end_ptr);

            /* Copy the partial message up until the end of the buffer */
            memcpy(write_ptr, log_msg, (end_ptr - write_ptr));

            /* Write the entire buffer to the file */
            logger_write_to_file(start_ptr, (end_ptr - start_ptr));

            /* Optional: Zero out the buffer space */
            // memset(start_ptr, '\0', buffer_size);

            /* Copy the left-over message to the start of "fresh" buffer space (after writing to the file) */
            if (buffer_overflow_cnt > 0) {
                memcpy(start_ptr, (log_msg + len - buffer_overflow_cnt), buffer_overflow_cnt);
            }
            write_ptr = start_ptr + buffer_overflow_cnt;
        }
        /* Buffer has enough space, write the entire message to the buffer */
        else {
            memcpy(write_ptr, log_msg, len);
            write_ptr += len;
        }

        /* Put the data pointer back to the available buffers */
        xQueueSend(g_empty_buffer_queue, &log_msg, portMAX_DELAY);
    }
}

/**
 * @returns true if logger has been initialized
 */
static bool logger_initialized(void)
{
    return (NULL != gp_file_buffer);
}

/**
 * Allocates the memory used for the logger.
 * @param [in] logger_priority  The priority at which the logger task will run.
 * @returns true if memory allocation succeeded.
 */
static bool logger_internal_init(UBaseType_t logger_priority)
{
    uint32_t i = 0;
    char * ptr = NULL;
    const bool success = true;

    /* Create the buffer space we write the logged messages to (before we flush it to the file) */
    gp_file_buffer = (char*) malloc(FILE_LOGGER_BUFFER_SIZE);
    if (NULL == gp_file_buffer) {
        goto failure;
    }

    /* Create the queues that keep track of the written buffers, and available buffers */
    g_write_buffer_queue = xQueueCreate(FILE_LOGGER_NUM_BUFFERS, sizeof(char*));
    g_empty_buffer_queue = xQueueCreate(FILE_LOGGER_NUM_BUFFERS, sizeof(char*));
    if (NULL == g_write_buffer_queue || NULL == g_empty_buffer_queue) {
        goto failure;
    }

    vTraceSetQueueName(g_write_buffer_queue, "Logger WR-Q");
    vTraceSetQueueName(g_empty_buffer_queue, "Logger EP-Q");

    /* Create the actual buffers for log messages */
    for (i = 0; i < FILE_LOGGER_NUM_BUFFERS; i++)
    {
        ptr = (char*) malloc(FILE_LOGGER_LOG_MSG_MAX_LEN);

        if (NULL == ptr) {
            goto failure;
        }

        /* DO NOT USE xQueueSendFromISR().
         * It causes weird file write errors and corrupts the entire file system
         * when the logger task is running.
         */
        xQueueSend(g_empty_buffer_queue, &ptr, 0);
    }

#if (FILE_LOGGER_KEEP_FILE_OPEN)
    gp_file_ptr = malloc (sizeof(*gp_file_ptr));
    if(FR_OK != f_open(gp_file_ptr, FILE_LOGGER_FILENAME, FA_OPEN_ALWAYS | FA_WRITE))
    {
        goto failure;
    }
#endif

#if BUILD_CFG_MPU
    logger_priority |= portPRIVILEGE_BIT;
#endif

    if (!xTaskCreate(logger_task, "logger", FILE_LOGGER_STACK_SIZE, NULL, logger_priority, NULL))
    {
        goto failure;
    }

    return success;

    /* failure case to delete allocated memory */
    failure:
        if (gp_file_buffer) {
            free(gp_file_buffer);
            gp_file_buffer = NULL;
        }

        if (g_empty_buffer_queue) {
            for (i = 0; i < FILE_LOGGER_NUM_BUFFERS; i++) {
                if (xQueueReceive(g_empty_buffer_queue, &ptr, 0)) {
                    free (ptr);
                }
            }
        }

        /* Delete g_write_buffer_queue */
        /* Delete g_empty_buffer_queue */

        return (!success);
}

void logger_send_flush_request(void)
{
    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState() && logger_initialized())
    {
        char * null_ptr_to_flush = NULL;
        xQueueSend(g_write_buffer_queue, &null_ptr_to_flush, portMAX_DELAY);
    }
}

uint32_t logger_get_logged_call_count(logger_msg_t severity)
{
    return (severity < log_last) ? g_logger_calls[severity] : 0;
}

uint16_t logger_get_blocked_call_count(void)
{
    return g_blocked_calls;
}

uint16_t logger_get_highest_file_write_time_ms(void)
{
    return g_highest_file_write_time;
}

uint16_t logger_get_num_buffers_watermark(void)
{
    return g_buffer_watermark;
}

void logger_init(uint8_t logger_priority)
{
    /* Prevent double init */
    if (!logger_initialized())
    {
        if (!logger_internal_init(logger_priority)) {
            printf("ERROR: logger initialization failure\n");
        }
    }
}

void logger_set_printf(logger_msg_t type, bool enable)
{
    const uint8_t mask = (1 << type);
    if (enable) {
        g_logger_printf_mask |= mask;
    }
    else {
        g_logger_printf_mask &= ~mask;
    }
}

void logger_log(logger_msg_t type, const char * filename, const char * func_name, unsigned line_num,
                const char * msg, ...)
{
    if (!logger_initialized()) {
        return;
    }

    uint32_t len = 0;
    char * buffer = NULL;
    char * temp_ptr = NULL;
    const rtc_t time = rtc_gettime();
    const unsigned int uptime = sys_get_uptime_ms();
    const bool os_running = (taskSCHEDULER_RUNNING == xTaskGetSchedulerState());

    /* This must match up with the logger_msg_t enumeration */
    const char * const type_str[] = { "debug", "info", "warn", "error" };

    // Find the back-slash or forward-slash to get filename only, not absolute or relative path
    if(0 != filename) {
        temp_ptr = strrchr(filename, '/');
        // If forward-slash not found, find back-slash
        if(0 == temp_ptr) temp_ptr = strrchr(filename, '\\');
        if(0 != temp_ptr) filename = temp_ptr+1;
    }
    else {
        filename = "";
    }

    if (0 == func_name) {
        func_name = "";
    }

    /* Get an available buffer */
    buffer = logger_get_buffer_ptr(os_running);

    do {
        int mon = time.month;
        int day = time.day;
        int hr = time.hour;
        int min = time.min;
        int sec = time.sec;
        unsigned int up = uptime;
        const char *log_type_str = type_str[type];
        const char *func_parens  = func_name[0] ? "()" : "";

        /* Write the header including time, filename, function name etc */
        len = sprintf(buffer, "%d/%d,%02d:%02d:%02d,%u,%s,%s,%s%s,%u,",
                      mon, day, hr, min, sec, up, log_type_str, filename, func_name, func_parens, line_num);
    } while (0);

    /* Append actual user message, and leave one space for \n to be appended by the logger task.
     * There is no efficient way to append \n here since we will have to use strlen(),
     * but since the logger task will take strlen() anyway, it can append it there.
     *
     * Example: max length = 10, and say we printed 5 chars so far "hello"
     *          we will sprintf "world" to "hello" where n = 10-5-1 = 4
     *          So, this sprintf will append and make it: "hellowor\0" leaving space to add \n
     *
     * Note: You cannot use returned value from vsnprintf() because snprintf() returns:
     *       "number of chars that would've been printed if n was sufficiently large"
     *
     * Note: "size" of snprintf() includes the NULL character
     */
    do {
        va_list args;
        va_start(args, msg);
        vsnprintf(buffer + len, FILE_LOGGER_LOG_MSG_MAX_LEN-len-1, msg, args);
        va_end(args);
    } while (0);

    ++g_logger_calls[type];
    logger_write_log_message(buffer, os_running);

    /* Print the message out if the printf mask was set */
    if (g_logger_printf_mask & (1 << type)) {
        puts(buffer);
    }
}

void logger_log_raw(const char * msg, ...)
{
    if (!logger_initialized()) {
        return;
    }

    const bool os_running = (taskSCHEDULER_RUNNING == xTaskGetSchedulerState());
    char * buffer = logger_get_buffer_ptr(os_running);

    /* Print the actual user message to the buffer */
    do {
        va_list args;
        va_start(args, msg);
        vsnprintf(buffer, FILE_LOGGER_LOG_MSG_MAX_LEN-1, msg, args);
        va_end(args);
    } while (0);

    logger_write_log_message(buffer, os_running);
}
