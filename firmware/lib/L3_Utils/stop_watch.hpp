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
#ifndef STOP_WATCH_H_
#define STOP_WATCH_H_

#include <stdint.h>
#include "lpc_sys.h"



/**
 * Stopwatch to measure an elapsed time.
 */
class MicroSecondStopWatch
{
    public:
        /// Default constructor that starts the stopwatch
        MicroSecondStopWatch() : mStartValue(0), mStopValue(0) { start(); }

        /// Starts the stopwatch operation (can be used to restart the stopwatch too)
        inline void start(void)    { mStartValue = mStopValue = getTimerValue(); }

        /// Stops the stopwatch operation enabling the captured value to be obtained
        inline void stop(void)     { mStopValue = getTimerValue(); }

        /**
         * Get the captured value between start() and stop()
         * If the stopwatch was never stopped, zero value will be returned.
         */
        inline uint64_t getCapturedTime(void) const { return (mStopValue - mStartValue); }

        /// Get the elapsed time since the stopwatch was started
        inline uint64_t getElapsedTime (void) const { return (getTimerValue() - mStartValue); }

    private:
        /// Single method to obtain the system time in us
        inline uint64_t getTimerValue(void) const { return sys_get_uptime_us(); }

        uint64_t mStartValue;   ///< Start time of the stopwatch
        uint64_t mStopValue;    ///< Stop time of the stopwatch
};



#ifdef TESTING
static inline void test_stop_watch_file(void)
{
    MicroSecondStopWatch w;
    assert(0 == w.getCapturedTime());
    assert(0 == w.getElapsedTime());
    w.start();
    delay_ms(5);
    assert(0 == w.getCapturedTime());
    w.stop();
    assert(w.getCapturedTime() >= 5000 && w.getCapturedTime() <= 6000);
}
#endif /* #ifdef TESTING */



#endif /* STOP_WATCH_H_ */
