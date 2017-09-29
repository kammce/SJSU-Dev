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

#include <stdlib.h>

#include "rtc_alarm.h"
#include "rtc.h"
#include "c_list.h"
#include "LPC17xx.h"



/**
 * Structure that holds the time alarm will trigger, and
 * the semaphore that will be given when the alarm triggers
 */
typedef struct {
    SemaphoreHandle_t *pAlarm;  ///< Semaphore that is given when alarm is triggered
    alarm_time_t time;          ///< The time that triggers the alarm
} sem_alarm_t;

static c_list_ptr g_list_timed_alarms = 0;        ///< Alarms for a specified time
static c_list_ptr g_list_recur_alarms[4] = { 0 }; ///< Recurring alarms, such as "every second"

static void rtc_enable_intr(void)
{
    LPC_RTC->CIIR |= (1 << 0);
    vTraceSetISRProperties(RTC_IRQn, "RTC", IP_rtc);
    NVIC_EnableIRQ(RTC_IRQn);
}

static bool for_each_recur_alarm_callback(void *item, void *arg1, void *arg2, void *arg3)
{
    long *do_yield = arg1;
    long yield_required = 0;

    SemaphoreHandle_t *signal = (SemaphoreHandle_t*)item;
    xSemaphoreGiveFromISR(*signal, &yield_required);
    if (yield_required) {
        *do_yield |= 1;
    }

    return 1;
}

static bool for_each_alarm_callback(void *item, void *arg1, void *arg2, void *arg3)
{
    const rtc_t time = rtc_gettime();
    sem_alarm_t *a = (sem_alarm_t*)item;

    if(a->time.hour == time.hour &&
       a->time.min == time.min &&
       a->time.sec == time.sec)
    {
        long *do_yield = arg1;
        long switch_required = 0;
        xSemaphoreGiveFromISR(*(a->pAlarm), &switch_required);
        if (switch_required) {
            *do_yield |= 1;
        }
    }
    return 1;
}




void rtc_alarm_create_recurring(alarm_freq_t freq, SemaphoreHandle_t *pAlarm)
{
    if(pAlarm && freq >= everySecond && freq <= everyDay)
    {
        if (!g_list_recur_alarms[freq]) {
            g_list_recur_alarms[freq] = c_list_create();
            rtc_enable_intr();
        }
        if (NULL != pAlarm) {
            c_list_insert_elm_end(g_list_recur_alarms[freq], pAlarm);
        }
    }
}

alarm_time_t* rtc_alarm_create(alarm_time_t time, SemaphoreHandle_t *pAlarm)
{
    if (NULL == pAlarm) {
        return NULL;
    }

    if (NULL == g_list_timed_alarms) {
        g_list_timed_alarms = c_list_create();
        rtc_enable_intr();
    }
    if (NULL == g_list_timed_alarms) {
        return NULL;
    }

    sem_alarm_t *pNewAlarm = (sem_alarm_t*) malloc(sizeof(sem_alarm_t));
    if (NULL == pNewAlarm) {
        return NULL;
    }

    pNewAlarm->time.hour = time.hour;
    pNewAlarm->time.min  = time.min;
    pNewAlarm->time.sec  = time.sec;
    pNewAlarm->pAlarm = pAlarm;

    if (!c_list_insert_elm_end(g_list_timed_alarms, pNewAlarm)) {
        free(pNewAlarm);
        return NULL;
    }

    return &(pNewAlarm->time);
}

#ifdef __cplusplus
extern "C" {
#endif
void RTC_IRQHandler(void)
{
    LPC_RTC->ILR |= (1 << 0); // Clear Increment Interrupt
    long do_yield = 0;

    const rtc_t time = rtc_gettime();
    c_list_for_each_elm(g_list_recur_alarms[everySecond], for_each_recur_alarm_callback, &do_yield, NULL, NULL);
    if(0 == time.sec) {
        c_list_for_each_elm(g_list_recur_alarms[everyMinute], for_each_recur_alarm_callback, &do_yield, NULL, NULL);
        if(0 == time.min) {
            c_list_for_each_elm(g_list_recur_alarms[everyHour], for_each_recur_alarm_callback, &do_yield, NULL, NULL);
            if(0 == time.hour) {
                c_list_for_each_elm(g_list_recur_alarms[everyDay], for_each_recur_alarm_callback, &do_yield, NULL, NULL);
            }
        }
    }

    c_list_for_each_elm(g_list_timed_alarms, for_each_alarm_callback, &do_yield, NULL, NULL);
    portEND_SWITCHING_ISR(do_yield);
}
#ifdef __cplusplus
}
#endif
