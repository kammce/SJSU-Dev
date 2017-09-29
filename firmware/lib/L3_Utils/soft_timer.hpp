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
 * @brief Provides "Software Timer" or a polling timer
 *
 * 20140401 : Added more useful methods
 * 20131201 : First version history tag (this one)
 */
#ifndef SOFT_TIMER_H__
#define SOFT_TIMER_H__

#include <stdint.h>
#include "lpc_sys.h"



/**
 * Soft timer class to provide thin layer of a timer.  There is no hard-timer running in
 * the background and this class imply relies on system timer to provide timer capability.
 *
 * @warning If you run FreeRTOS in "configUSE_TICKLESS_IDLE", then this timer will not
 *          work well since the OS ticks will not happen to drive the timer.  In that
 *          case, you are better off using the true FreeRTOS timer which will not
 *          suppress the timer ticks if a timer expires.
 */
class SoftTimer
{
    public:
        /// Default constructor
        SoftTimer() : mTargetMs(0), mIntervalMs(0) {}

        /// Constructor to set timer while instantiating this object.
        SoftTimer(uint32_t ms) : mTargetMs(0), mIntervalMs(0) { reset(ms); }

        /// @returns true if the timer has expired
        inline bool expired(void) const
        { return (0 == mIntervalMs) ? false : ( getCurrentTimeMs() >= mTargetMs); }

        /**
         * Restarts the timer to provide consistent frequency of expired().  restart() is better suited
         * to provide same frequency because in case we check for expired() at a later time that runs
         * past our timer 3 times, then expired() in combination with restart() will return true 3 times.
         * See example below :
         *
         * @code
         *      timer.reset(10); // Init once
         *
         *      // After reset(), use this as recurring timer :
         *      if (timer.expired()) {
         *          timer.restart();
         *      }
         * @endcode
         */
        inline void restart(void) { mTargetMs += mIntervalMs; };

        /**
         * Resets the timer from this point of time using the new timer value given.
         * @param ms  The milliseconds at which timer should expire next.
         */
        inline void reset(uint64_t ms) { mIntervalMs = ms; mTargetMs = getCurrentTimeMs() + ms; }

        /// Resets the timer from this point of time using the previous timeout interval
        inline void reset(void) { mTargetMs = getCurrentTimeMs() + mIntervalMs; }

        /// Stops the timer.
        inline void stop(void) { mIntervalMs = mTargetMs = 0; }

        /// @returns true if the timer is set and running
        inline bool isRunning(void) const { return (mIntervalMs > 0); }

        /// @returns the timer value set by the constructor or reset(uint64_t)
        inline uint64_t getTimerValueMs(void) const { return mIntervalMs; }

        /// @returns the absolute time value when timer will expire with respect to system timer (in milliseconds)
        inline uint64_t getTargetTimerValueMs(void) const { return mTargetMs; }

        /**
         * @returns the time value until expiration (in milliseconds)
         * @note If timer hasn't expired, zero value is returned.
         */
        inline uint64_t getTimeToExpirationMs(void) const
        {
            uint64_t now = getCurrentTimeMs();
            const bool isExpired = (now >= mTargetMs);
            return isExpired ? 0 : (mTargetMs - now);
        }

        /**
         * @returns the time value since expiration (in milliseconds)
         * @note If timer hasn't expired, zero value is returned.
         */
        inline uint64_t getTimeSinceExpirationMs(void) const
        {
            uint64_t now = getCurrentTimeMs();
            const bool isExpired = (now >= mTargetMs);
            return isExpired ? (now - mTargetMs) : 0;
        }

        /**
         * Static function to get the timer value.
         * This is the only method that is needed by the system.
         *
         * @note You can use this method without instantiating an object of the class:
         * @code
         *      uint64_t timer = SoftTimer::getCurrentTimeMs();
         * @endcode
         *
         * @returns the current timer value of the system
         */
        static inline uint64_t getCurrentTimeMs(void) { return sys_get_uptime_ms(); }

    protected:
        /** @{ uint64_t is large enough such that timer will never overflow */
        uint64_t mTargetMs;     ///< Expire time with respect to OS tick
        uint64_t mIntervalMs;   ///< Timer interval
        /** @} */
};




#ifdef TESTING
static inline void test_soft_timer_file(void)
{
    SoftTimer t;
    assert(!t.expired());
    assert(!t.isRunning());

    t.reset(); assert(!t.expired());
    t.reset(10); assert(!t.expired());
    delay_ms(8); assert(!t.expired());
    delay_ms(3); assert(t.expired());
    t.reset(); assert(!t.expired());
    delay_ms(8); assert(!t.expired());
    delay_ms(3); assert(t.expired());
    assert(t.isRunning());
    assert(10 == t.getTimerValueMs());

    t.reset(5); assert(5 == t.getTimeToExpirationMs());
    delay_ms(10); assert(5 == t.getTimeSinceExpirationMs());
}
#endif /* #ifdef TESTING */



#endif /* SOFT_TIMER_H__ */
