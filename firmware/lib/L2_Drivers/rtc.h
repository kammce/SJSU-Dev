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
 * @file rtc.h
 * @ingroup Drivers
 * @brief   This file provides access to the System's Real-time Clock that maintains time
 *          even through power-loss granted that the RTC Backup battery is installed.
 */
#ifndef RTC_H
#define RTC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>



/**
 * This struct aligns with the LPC Hardware's consolidated RTC registers.
 * The un-named chars are just for padding according to LPC CTIME registers.
 */
typedef struct {
    /*    - Data -----     - Padding --- */
    uint32_t sec  : 6;      uint32_t : 2;
    uint32_t min  : 6;      uint32_t : 2;
    uint32_t hour : 5;      uint32_t : 3;
    uint32_t dow  : 3;      uint32_t : 5;

    uint32_t day  : 5;      uint32_t : 3;
    uint32_t month: 4;      uint32_t : 4;
    uint32_t year :12;      uint32_t : 4;

    uint32_t doy  :12;      uint32_t : 20;
} __attribute__((packed)) rtc_t ;

/**
 * Enumeration of the RTC.dow
 */
typedef enum {
    dow_sun = 0,
    dow_mon,
    dow_tue,
    dow_wed,
    dow_thu,
    dow_fri,
    dow_sat,
} __attribute__((packed)) day_of_week_t;

/// Initialize the RTC
void rtc_init (void);

/// @returns the latest time in RTC structure
rtc_t rtc_gettime (void);

/**
 * Sets the RTC time
 * @param [in] rtcstruct  The rtc time structure pointer
 */
void rtc_settime (const rtc_t* rtcstruct);

/**
 * Get the RTC time as string in the format: "Wed Feb 13 15:46:11 2013"
 * @returns the pointer to the time string (do not modify it)
 * @warning This method is not thread safe
 */
const char* rtc_get_date_time_str(void);



#ifdef __cplusplus
}
#endif
#endif
