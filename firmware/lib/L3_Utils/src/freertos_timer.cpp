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

#include "../freertos_timer.hpp"

/**
 * Configure this macro to 1 @ FreeRTOSConfig.h to use the timer class
 */
#if (configUSE_TIMERS)
FreeRTOSTimer::FreeRTOSTimer(TimerCallbackFunction_t pFunction, timeMs_t t, TimerType type)
{
	// xTimerCreate() copies the pointer for the name, so the tName variable
	// cannot simply be on the stack, it should be global or static
	static char tName[] = "Tmr";

	mTimerHandle = xTimerCreate(tName, OS_MS(t),
								type == TimerOneShot ? pdFALSE : pdTRUE,
								0,
								pFunction);
}
FreeRTOSTimer::~FreeRTOSTimer()
{
	xTimerDelete(mTimerHandle, 0);
}

void FreeRTOSTimer::start()
{
	xTimerStart(mTimerHandle, 0);
}
void FreeRTOSTimer::stop()
{
	xTimerStop(mTimerHandle, 0);
}
void FreeRTOSTimer::reset()
{
	xTimerReset(mTimerHandle, 0);
}
void FreeRTOSTimer::changePeriod(timeMs_t t)
{
	xTimerChangePeriod(mTimerHandle, t, 0);
}
bool FreeRTOSTimer::isRunning()
{
	return (pdFALSE != xTimerIsTimerActive(mTimerHandle) ? true : false);
}

#define callTimerFunctionFromIsr(f) \
		portBASE_TYPE higherPrTaskWoken = 0;	\
		f(mTimerHandle, &higherPrTaskWoken);	\
		portEND_SWITCHING_ISR(higherPrTaskWoken)

void FreeRTOSTimer::startFromISR()
{
	callTimerFunctionFromIsr(xTimerStartFromISR);
}
void FreeRTOSTimer::stopFromISR()
{
	callTimerFunctionFromIsr(xTimerStopFromISR);
}
void FreeRTOSTimer::resetFromISR()
{
	callTimerFunctionFromIsr(xTimerResetFromISR);
}
void FreeRTOSTimer::changePeriodFromISR(timeMs_t t)
{
	portBASE_TYPE higherPrTaskWoken = 0;
	xTimerChangePeriodFromISR(mTimerHandle, t, &higherPrTaskWoken);
	portEND_SWITCHING_ISR(higherPrTaskWoken);
}
#endif
