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
 * @brief Contains FreeRTOS task examples.
 */
 
#ifndef EXAMPLES_HPP_
#define EXAMPLES_HPP_

#include "scheduler_task.hpp"
#include "shared_handles.h"
#include "uart3.hpp"
#include "rn_xv_task.hpp"
#include "FreeRTOS.h"
#include "semphr.h"



/**
 * This is the simplest example of a task.  It just computes some work
 * periodically and prints status out.
 */
class example_task : public scheduler_task
{
    public:
        example_task();
        bool run(void *p);
};

/**
 * Example task that demonstrates board io such as switch, led, and sensor data
 */
class example_io_demo : public scheduler_task
{
    public:
        example_io_demo();
        bool run(void *p);
};

/**
 * This example shows how to use FreeRTOS based alarms such that you can design
 * tasks that do work every second, every minute etc.
 */
class example_alarm : public scheduler_task
{
    public:
        example_alarm();
        bool init(void);        ///< We create our semaphores heres. No FreeRTOS Blocking API here!!!
        bool taskEntry(void);   ///< We enable the alarms here.
        bool run(void *p);      ///< We run our code here

    private:
        SemaphoreHandle_t mAlarmSec;
        SemaphoreHandle_t mAlarmMin;
};



/**
 * This example shows how to log information and use Queue Set
 */
class example_logger_qset : public scheduler_task
{
    public :
        example_logger_qset();
        bool init(void);
        bool run(void *p);

    private :
        SemaphoreHandle_t mSec, mMin;
};



/**
 * This example shows how to save variables on disk.
 * You will notice that the 'someVarWeDontWantToLose' variable will automatically
 * get saved if no command is entered in terminalTask() for 120 seconds.
 * Instead of booting up from zero value, we will actually get the previous value
 * recalled from a file saved onto the flash memory called "disk"
 */
class example_nv_vars : public scheduler_task
{
    public:
        example_nv_vars();
        bool regTlm(void);
        bool run(void *p);

    private:
        int mVarWeDontWantToLose;
};



/**
 * queue_tx and queue_rx classes show how two tasks can communicate with each other
 * using queue.
 */
class queue_tx : public scheduler_task
{
    public :
        queue_tx();
        bool init(void);
        bool run(void *p);
};

class queue_rx : public scheduler_task
{
    public :
        queue_rx();
        bool run(void *p);
};

class producer : public scheduler_task
{
    public :
        producer();
        bool run(void *p);
};

class consumer : public scheduler_task
{
    public :
        consumer();
        bool run(void *p);
};

#endif /* EXAMPLES_HPP_ */
