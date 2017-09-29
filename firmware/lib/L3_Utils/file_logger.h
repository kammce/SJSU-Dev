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
 * @brief This is a logger that logs data to a file on the system such as an SD Card.
 * @ingroup Utilities
 *
 * 20140714: Fixed bugs and added more API
 * 20140529: Changed completely to C and FreeRTOS based logger
 * 20120923: modified flush() to use semaphores
 * 20120619: Initial
 */
#ifndef FILE_LOGGER_HPP__
#define FILE_LOGGER_HPP__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>



/**
 * @{
 * The main parameters are the buffer size, and number of message buffers. The buffer size controls how
 * much data we can cache before we are forced to write it to the output file.
 *
 * The message buffers are the maximum number of LOGGING calls (macros) that can occur at once after which
 * the caller to LOG macro will be blocked.  Note that even a small number like 5 is good enough because
 * as soon as there is time for the logging task to run, it will transfer the LOGGED message into the
 * file buffer immediately.  So this number is kind of like a double buffer.  On the other hand, please
 * note that while the file is being written, the number of buffers need to be enough because the logger
 * task will be busy writing the buffer to the file.
 *
 * For example, if we anticipate logging every 10ms, and 1K of data takes 100ms to write, then we should
 * configure the number of logging buffers greater than 10 because while the logger task is busy writing
 * 1K of data for 100ms, we need to be able to write for 100ms, which will take at least 10 buffers.
 *
 * The flush timeout is the timeout after which point we are forced to flush the data buffer to the file.
 * So in an event when no logging calls occur and there is data in the buffer, we will write it to the
 * file after this time.
 */
#define FILE_LOGGER_BUFFER_SIZE      (1 * 1024)     ///< Recommend multiples of 512
#define FILE_LOGGER_NUM_BUFFERS      10             ///< Number of buffers (need to have enough while file is being written)
#define FILE_LOGGER_LOG_MSG_MAX_LEN  150            ///< Max length of a log message
#define FILE_LOGGER_FILENAME         "0:log.csv"    ///< Destination filename (0: for SPI flash, 1: for SD card)
#define FILE_LOGGER_STACK_SIZE       (3 * 512 / 4)  ///< Stack size in 32-bit (1 = 4 bytes for 32-bit CPU)
#define FILE_LOGGER_FLUSH_TIME_SEC   (1 * 60)       ///< Logs are flushed after this time
#define FILE_LOGGER_BLOCK_TIME_MS    (10)           ///< If no buffer available within this time, block time counter will increment
#define FILE_LOGGER_KEEP_FILE_OPEN   (0)            ///< If non-zero, the file will be kept open
/** @} */


/**
 * Enumeration of the type of the log message.
 * You should not use this directly for logging call since the macros pass it to logger_log()
 * But these can be passed to logger_get_logged_call_count()
 */
typedef enum {
    log_debug,  ///< Debug logs are printed to stdio (printf) unless disabled by logger_set_printf()
    log_info,
    log_warn,
    log_error,
    log_last, ///< Marks the last entry, do not use
} logger_msg_t;

/**
 * Initializes the logger; this must be done before further logging calls are used.
 * @param [in] logger_priority The priority at which logger should buffer user data and then write to file.
 */
void logger_init(uint8_t logger_priority);

/**
 * Enables the given logging severity call to be also printed on printf()
 * @param [in] type    The severity for which the logging call will be printed
 * @param [in] enable  If true, the severity level will also get printed to the screen
 *
 * Note that if a lot of data is being printed, the logger call may take a while until
 * the stdio printf() returns back.
 */
void logger_set_printf(logger_msg_t type, bool enable);

/**
 * @{ Macros to log a message using printf() style API
 * @note If FreeRTOS is not running, the message is immediately output to file.  If FreeRTOS is running,
 *       the data will be written to buffer using the logger task and eventually flushed out to the file
 *       either after the timeout or when the buffer is full.
 *
 * @code
 *      LOG_INFO("Error %i encountered", error_number);
 * @endcode
 */
#define LOG_ERROR(msg, p...)  logger_log (log_error, __FILE__, __FUNCTION__, __LINE__, msg, ## p)
#define LOG_WARN(msg, p...)   logger_log (log_warn,  __FILE__, __FUNCTION__, __LINE__, msg, ## p)
#define LOG_INFO(msg, p...)   logger_log (log_info,  __FILE__, __FUNCTION__, __LINE__, msg, ## p)
#define LOG_DEBUG(msg, p...)  logger_log (log_debug, __FILE__, __FUNCTION__, __LINE__, msg, ## p)
/** @} */


/**
 * This macro will log INFO message without filename, function name, and line number.
 * This can save space to log simple messages when you don't want to know the function
 * name, line number and filename of where the logger function was called from.
 *
 * @note If FreeRTOS is not running, the message is immediately output to file.  If FreeRTOS is running,
 *       the data will be written to buffer using the logger task and eventually flushed out to the file
 *       either after the timeout or when the buffer is full.
 */
#define LOG_SIMPLE_MSG(msg, p...)       logger_log (log_info, NULL, NULL, 0, msg, ## p)

/**
 * Logs a raw message without any header such as the timestamp.
 *
 * @note If FreeRTOS is not running, the message is immediately output to file.  If FreeRTOS is running,
 *       the data will be written to buffer using the logger task and eventually flushed out to the file
 *       either after the timeout or when the buffer is full.
 */
#define LOG_RAW_MSG(msg, p...)          logger_log_raw(msg, ## p)

/**
 * Macro to flush the logs.
 * You can flush it using logger_send_flush_request() but the MACRO is provided
 * just to be consistent with the other logger macros where a function is not used.
 *
 * @note Flushing is not needed when the OS is running.
 */
#define LOG_FLUSH()                     logger_send_flush_request()



/**
 * Flushes the cached log data to the file
 * @post  This will send a special request on the logger queue to flush the data, so
 *        the actual flushing will finish by the logger task at a later time.
 *
 * @note Flushing is not needed when the OS is running.
 */
void logger_send_flush_request(void);

/**
 * @returns the number of logged messages for the given severity.
 * @param [in] severity  The severity for which to get the number of calls.
 */
uint32_t logger_get_logged_call_count(logger_msg_t severity);

/**
 * @returns the number of logging calls that ended up blocking or sleeping the task
 *          waiting for a logger buffer to be available.
 *
 * If the number is greater than zero, it indicates that you either need to slow
 * down logger calls, or increase the number of log buffers.
 */
uint16_t logger_get_blocked_call_count(void);

/**
 * @returns the highest time that was spend writing the logger buffer to file.
 * This can be useful to assess how many FILE_LOGGER_NUM_BUFFERS we need because we only
 * need enough buffers available while the file buffer is being written.
 */
uint16_t logger_get_highest_file_write_time_ms(void);

/**
 * @returns the highest watermark of the number of buffers available to the logger
 * This can be useful to assess how many FILE_LOGGER_NUM_BUFFERS we need in the worst case.
 */
uint16_t logger_get_num_buffers_watermark(void);





/* Do not use rest of the API after this line */

/**
 * Logs a message.
 * You should not use this directly, the macros pass the arguments to this function.
 */
void logger_log(logger_msg_t type, const char * filename, const char * func_name, unsigned line_num,
                const char * msg, ...);

/**
 * @see LOG_RAW_MSG()
 * You should not use this directly, the macros pass the arguments to this function.
 */
void logger_log_raw(const char * msg, ...);



#ifdef __cplusplus
}
#endif
#endif /* FILE_LOGGER_HPP__ */
