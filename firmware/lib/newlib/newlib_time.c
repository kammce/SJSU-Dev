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
 
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

#include "rtc.h"



/// Read the time from RTC to C-library struct tm
static void rtc_read_to_struct (struct tm *time_ptr, unsigned int *milliseconds)
{
    rtc_t t = rtc_gettime();
    time_ptr->tm_sec   = t.sec;
    time_ptr->tm_min   = t.min;
    time_ptr->tm_hour  = t.hour;
    time_ptr->tm_mday  = t.day;
    time_ptr->tm_mon   = t.month - 1;
    time_ptr->tm_year  = t.year - 1900;
    time_ptr->tm_wday  = t.dow;
    time_ptr->tm_yday  = t.doy - 1;
    time_ptr->tm_isdst = 0;

    // TO DO Also calculate milliseconds somehow ?
}

/// Write the time from C-library struct tm to RTC
static void rtc_write_from_struct (struct tm *time_ptr)
{
    rtc_t t = { 0 };
    t.sec = time_ptr->tm_sec;
    t.min = time_ptr->tm_min;
    t.hour= time_ptr->tm_hour;
    t.day = time_ptr->tm_mday;
    t.month = time_ptr->tm_mon;
    t.year = time_ptr->tm_year + 1900;
    t.dow = time_ptr->tm_wday;
    t.doy = time_ptr->tm_yday;
    rtc_settime(&t);
}

/// Read the milliseconds of the system
static time_t rtc_get_epoch (unsigned int *milliseconds)
{
    struct tm tm;
    rtc_read_to_struct (&tm, milliseconds);
    return mktime (&tm);
}

/// GCC C libraries call this function to get time of day
__attribute__ ((used)) int _gettimeofday (struct timeval *tp, void *tzp)
{
    if (tp)
    {
        unsigned int milliseconds = 0;
        tp->tv_sec = rtc_get_epoch (&milliseconds);
        tp->tv_usec = milliseconds * 1000;
    }

    return 0;
}

/// GCC C libraries call this function to set time of day
__attribute__ ((used)) int _settimeofday (struct timeval *tp, void *tzp)
{
    if (tp)
    {
        struct tm * timeinfo = localtime(&(tp->tv_sec));
        rtc_write_from_struct(timeinfo);
    }

    return 0;
}
