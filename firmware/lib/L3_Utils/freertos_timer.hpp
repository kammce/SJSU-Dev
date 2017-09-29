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
 * @file FreeRTOSTimer.hpp
 * @brief This file provides C++ wrapper around FreeRTOS Timer class
 * @ingroup Utilities
 *
 * Version: 06192012    Initial
 */
#ifndef TIMER_HPP_
#define TIMER_HPP_

#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"



/// Typedef to clarify the time in milliseconds
typedef TickType_t timeMs_t;

/// Enumeration to indicate timer type
typedef enum { TimerOneShot, TimerPeriodic } TimerType;

/**
 * Timer class to attach C-Functions to timers, but use C++ class
 * structure to easily control the timers.
 *
 * This is C++ wrapper around FreeRTOS timers.
 * configUSE_TIMERS @ FreeRtosConfig.h must be set to 1 to use this class.
 *
 * @ingroup Utilities
 */
class FreeRTOSTimer
{
    public:
	 	/**
	 	 * Constructor to create the timer.
	 	 * @param pFunction The pointer to C-Function
	 	 * @param t			The timer expiration in milliseconds
	 	 * @param type      Timer type, either one shot or periodic, defaults to periodic
	 	 */
        FreeRTOSTimer(TimerCallbackFunction_t pFunction, timeMs_t t, TimerType type=TimerPeriodic);
        ~FreeRTOSTimer(); ///< Destructor to delete the timer

        void start();					///< Starts the timer, and calls reset() if timer is already started.
        void stop();					///< Stops the timer
        void reset();					///< Resets(restarts) the timer
        void changePeriod(timeMs_t t);	///< Changes the timer's time
        bool isRunning();				///< @returns TRUE if the timer is active

        /**
         * @{ \name Timer functions to be used from within an ISR.
         * These will automatically call FreeRTOS YIELD function if required.
         */
        void startFromISR();					///< Restarts the timer from an ISR
        void stopFromISR();						///< Stops the timer from an ISR
        void resetFromISR();					///< Resets the timer from an ISR
        void changePeriodFromISR(timeMs_t t);    ///< Changes the timer's time from an ISR
        /** @} */

        /// @returns The FreeRTOS Timer Handle for this timer.
        inline TimerHandle_t getTimerHandle() { return mTimerHandle; }

    private:
        TimerHandle_t mTimerHandle;
};

#endif /* TIMER_HPP_ */
