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
 * @brief This file provides API to enable real-time clock FreeRTOS signals or alarms
 * @ingroup Utilities
 */

#ifndef RTC_SEM_HPP_
#define RTC_SEM_HPP_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"



/**
 * Frequency of a recurring alarm
 */
typedef enum {
    everySecond = 0,
    everyMinute = 1,
    everyHour   = 2,
    everyDay    = 3,
} alarm_freq_t;

/**
 * rtc_alarm_create() returns the pointer to this structure.
 * This structure can be used to change the alarm time after
 * it has been created.
 */
typedef struct {
    uint8_t hour, min, sec;
} alarm_time_t;



/**
 * Enables a permanent recurring alarm every second, minute, or hour
 * @param pAlarm  The signal (semaphore) to give when the alarm triggers
 * @param freq    The frequency of the alarm  @see alarm_freq_t
 * @note This is different from rtc_alarm_create() because once you enable
 *       this recurring alarm, there is no way to disable it or change it later.
 */
void rtc_alarm_create_recurring(alarm_freq_t freq, SemaphoreHandle_t *pAlarm);

/**
 * Enables alarm at the given @param time
 * @post pAlarm semaphore will be given when RTC time matches the given time.
 * @return alarm_time_t that can be used to modify alarm time
 *
 * @code
 *      alarm_time_t my_time = { 12, 30, 0 }; // Alarm at 12:30 PM
 *      alarm_time_t *my_alarm_time = rtc_alarm_create(my_time, &my_sem);
 *
 *      // Check for alarm trigger :
 *      if (xSemaphoreTask(my_sem, portMAX_DELAY)) {
 *          // Do something
 *      }
 *
 *      // You can change the time any time :
 *      my_alarm_time->hour = 13
 * @endcode
 */
alarm_time_t* rtc_alarm_create(alarm_time_t time, SemaphoreHandle_t *pAlarm);

/**
 * Turns off an alarm that was created by rtc_alarm_create()
 * Nothing special here, the hour is set to 25, which will never occur.
 */
static inline void rtc_alarm_off(alarm_time_t *p) { p->hour = 25; p->min = p->sec=0; }



#ifdef __cplusplus
}
#endif
#endif /* RTC_SEM_HPP_ */
