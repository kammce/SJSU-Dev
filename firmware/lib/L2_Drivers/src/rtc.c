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

#include <stdio.h>
#include <string.h>     // memcmp()
#include <time.h>

#include "rtc.h"
#include "LPC17xx.h"
#include "sys_config.h" // SYS_CFG_RTC_VALID_YEARS_RANGE



void rtc_init (void)
{
    lpc_pconp(pconp_rtc, true);
    LPC_RTC->CCR = 1;           // Enable RTC

    const uint16_t years[] = SYS_CFG_RTC_VALID_YEARS_RANGE;
    rtc_t time = rtc_gettime();

    /* Check for invalid time */
    if(time.year < years[0] || time.year > years[1] ||
       time.min >= 60 || time.sec >= 60 || time.hour >= 24 ||
       time.doy > 365 || time.month > 12 || time.day > 31)
    {
        time.day = 1;
        time.month = 1;
        time.year = years[0];

        time.dow  = 0;
        time.hour = 0;
        time.min  = 0;
        time.sec  = 0;

        rtc_settime(&time);
    }
}

rtc_t rtc_gettime (void)
{
    // Read the struct from LPC Memory Map
    rtc_t t1 = *(rtc_t*) (&LPC_RTC->CTIME0) ;
    rtc_t t2 = *(rtc_t*) (&LPC_RTC->CTIME0) ;

    // Two times read must be the same to make sure we don't read
    // the different registers in the middle of time update.
    while (0 != memcmp(&t1, &t2, sizeof(t1))) {
        t1 = *(rtc_t*) (&LPC_RTC->CTIME0) ;
        t2 = *(rtc_t*) (&LPC_RTC->CTIME0) ;
    }

    return t1;
}

void rtc_settime (const rtc_t *rtc)
{
    /* Disable the RTC first */
    LPC_RTC->CCR = 0;

    /*
     * Update RTC registers :
     * Note: Cannot write to consolidated registers since they are read-only
     */
    LPC_RTC->SEC   = rtc->sec;
	LPC_RTC->MIN   = rtc->min;
	LPC_RTC->HOUR  = rtc->hour;
	LPC_RTC->DOW   = rtc->dow;
	LPC_RTC->DOM   = rtc->day;
	LPC_RTC->MONTH = rtc->month;
	LPC_RTC->YEAR  = rtc->year;
	LPC_RTC->DOY   = rtc->doy;

	/* Restart RTC */
	LPC_RTC->CCR = 1;
}

const char* rtc_get_date_time_str(void)
{
    time_t rawtime;
    time (&rawtime);

    return ctime (&rawtime);
}
