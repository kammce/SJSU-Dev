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
 *
 * @file
 * @brief C++ Task class based on FreeRTOS
 * @ingroup Utilities
 *
 * This file provides the structure to create and manage a FreeRTOS task.
 * @see scheduler_task for further documentation
 *
 * 20140215     : Added add/get sharedHandles() by index (faster).
 *                Made some functions static with documentation on how to use them.
 * 20140211     : Fixed bug with statistical update (ticks to ms conversion)
 * 20140209     : Added debug print option.  @see scheduler_start();
 * 20140205     : Added resume(), provided simplified cpu usage
 * 20131201     : Remove vector dependency, use internal linked list without a library dependency
 * 20131007     : Enhanced CPU usage API
 * 20130415     : Added queue set functionality
 */
#ifndef CPP_TASK_HPP_
#define CPP_TASK_HPP_
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"



/// Forward declaration of the scheduler task class
class scheduler_task;


/**
 * Adds your task to the scheduler
 * @param task  The pointer of your allocated class.
 * @code
 *      scheduler_add_task(new my_task());
 * @endcode
 */
void scheduler_add_task(scheduler_task *task);

/**
 * Starts FreeRTOS scheduler with the tasks added by scheduler_add_task()
 *
 * @param register_internal_tlm  If true, then this method will register the
 *        statistical telemetry for each task.  @note This is different from
 *        the regTlm() method that is called regardless.
 *
 * @param dbg_print  If true, process of initing and starting the tasks is
 *                    printed through stdio output
 */
void scheduler_start(bool dbg_print=false, bool register_internal_tlm=false);


/**
 * Scheduler task.
 * This class should be inherited and at minimum, you would need to override the
 * run() function to execute as part of your task.  Note that the run() function
 * should return as the scheduler is responsible for the FreeRTOS task loop.
 *
 * All tasks perform init() one after another, followed by taskEntry().
 * Only after all tasks perform their init() and taskEntry(), the run() method
 * begins to loop, so this solves the initialization dependency problem.
 *
 * The order of operations is the following :
 *      - All tasks call their: init()
 *      - All tasks call their: regTlm()
 *      - FreeRTOS is started
 *      - All tasks call their: taskEntry()
 *      - All tasks enter loop to call run() method
 *
 * Note that until you get to the run() function, you are essentially in a
 * "thread safe" mode since context or task switch will not occur.
 *
 * Example task that prints a message every 1000ms :
 * @code
 *      class my_task : public scheduler_task
 *      {
 *          public :
 *          // Call scheduler_task() constructor with your task name, stack size, and priority
 *          my_task() : scheduler_task("task name", 2048, 1)
 *          {
 *              setRunDuration(1000); // Optional frequency control for run()
 *          }
 *
 *          bool run(void *param)
 *          {
 *              puts("Hello World");
 *              return true;
 *          }
 *      };
 *
 *      // Add the task to the scheduler and start it :
 *      scheduler_add_task(new my_task());
 *      scheduler_start();
 * @endcode
 */
class scheduler_task
{
    public :
        /**
         * Get a task pointer by name
         * This is declared static such that any code in any context can retrieve a
         * task pointer by name.  Here is the usage (use the scope operator directly):
         * @code
         *      scheduler_task* pTask = scheduler_task::getTaskPtrByName("taskname");
         *      pTask->suspend();
         * @endcode
         */
        static scheduler_task* getTaskPtrByName(const char *);

        /**
         * @{ Get methods
         * Tasks can get each others' pointers by using their task names.
         * After obtaining the pointer, they can get each other's CPU usage, and task handle.
         */
        inline const char*  getTaskName(void)     const { return mName;      }
        inline uint32_t     getFreeStack(void)    const { return mFreeStack; }
        inline uint32_t     getRunCount(void)     const { return mRunCount;  }
        inline TaskHandle_t getTaskHandle(void)   const { return mHandle;    }
        inline uint8_t      getTaskPriority(void) const { return mPriority;  }
        /** @} */

        /** @{ CPU usage API
         *  @note static functions can be called from any function, even not belonging to this class.
         */
        uint8_t getTaskCpuPercent(void) const;   ///< Get CPU usage of this task
        static uint8_t getSysCpuPercent (void);  ///< Get total system CPU percent
        static uint8_t getSysIdlePercent(void);  ///< Get total system IDLE percent
        /** @} */

        /** @{
         * Suspend and resume functions.
         * Obviously you can suspend yourself but someone else will have to resume you ;)
         */
        inline void suspend(void) const { vTaskSuspend(mHandle); }
        inline void resume (void) const { vTaskResume(mHandle);  }
        /** @} */

        /**
         * @{
         * Add/Get a shared object pointer by name such that multiple classes of this type
         * can communicate between themselves.
         * @note It is recommended to use the later version of addSharedObject() and
         *       getSharedObject() because it is faster.  Consider this version of
         *       overloaded API deprecated -- :)
         *
         * @code
         *     QueueHandle_t sensor_queue;
         *
         *     // Task A can share its handle by a name:
         *     addSharedObject("senqueue", sensor_queue);
         *
         *     // Task B can retrieve the handle in its context:
         *     QueueHandle_t sensor_queue = getSharedObject("senqueue");
         * @endcode
         *
         * @note
         * These functions are declared static because that way, any code in any context
         * can get a shared object.  So a C function (ie terminal cmd) can get a shared object
         * and queue data for a task. Here is the usage (use the scope operator directly):
         * @code
         *     void my_func(void)
         *     {
         *         xSemaphoreGive(scheduler_task::getSharedObject("foo"));
         *     }
         * @endcode
         */
        static bool  addSharedObject(const char *name, void *obj_ptr);
        static void* getSharedObject(const char *name);
        /** @} */

        /**
         * @{
         * Add or get a shared object by an index number.  This provides a better
         * alternative to sharing handles because strcmp() doesn't take place, instead
         * the getSharedObject() simply returns the handle at the index.
         *
         * This index is best utilized by using an enumeration, such as this:
         * @code
         *     enum {
         *         shr_Sem,
         *         shr_Queue,
         *     };
         *
         *     SemaphoreHandle_t semaphore;
         *     QueueHandle_t senqueue;
         *
         *     // Task A can share its handles:
         *     addSharedObject(shr_Sem,   semaphore);
         *     addSharedObject(shr_Queue, senqueue);
         *
         *     // Task B can retrieve the handles in its class:
         *     QueueHandle_t qh = getSharedObject(shr_Queue);
         * @endcode
         *
         * @note
         * These functions are declared static because that way, any code in any context
         * can get a shared object.  So a C function (ie terminal cmd) can get a shared object
         * and queue data for a task. Here is the usage (use the scope operator directly):
         * @code
         *     void my_func(void)
         *     {
         *         xSemaphoreGive(scheduler_task::getSharedObject(shr_Sem));
         *     }
         * @endcode
         */
        static bool  addSharedObject(uint8_t index, void *obj_ptr);
        static void* getSharedObject(uint8_t index);
        /** @} */

    protected:
        /**
         * Constructor
         * @param name      The task's name
         * @param stack     The task's stack size in bytes
         * @param priority  FreeRTOS task priority
         * @param param     OPTIONAL param to pass to your run() when it's called.
         */
        scheduler_task(const char *name, uint32_t stack, uint8_t priority, void *param=0);
        virtual ~scheduler_task() {}

        /**
         * Optional: Override this function to initialize anything before FreeRTOS starts.
         * @return   true upon success.
         * @warning  DO NOT USE FreeRTOS blocking API in this function
         * @note You should use init() instead of the constructor to initialize and allocate memory
         *       because the status of init() is checked and reported, but a failure at constructor
         *       will not be easily caught (or reported).
         */
        virtual bool init(void)   { return true; }

        /**
         * Optional: Override this function to register your telemetry before FreeRTOS starts.
         * @return   true upon success.
         * @warning  DO NOT USE FreeRTOS blocking API in this function
         */
        virtual bool regTlm(void) { return true; }

        /**
         * Optional : Override this function which is called ONCE after FreeRTOS starts
         * If your task depends on someone else's init(), then you can put your
         * relevant code here that is called after everyone's init().  You CAN
         * use FreeRTOS API but note that all other tasks will wait for everyone's
         * taskEntry() to take place before the run() method is called.
         *
         * @warning  Do not suspend your task otherwise all tasks will get suspended.
         * @return   true upon success.
         */
        virtual bool taskEntry(void) { return true; }

        /**
         * MUST override this function to run your tasks' code
         * @param param  The same pointer as the constructor's param
         * @return   false if something unexpected occurs.  In this case, the
         *           task will be suspended from doing further work.
         */
        virtual bool run(void *param)=0;

        /**
         * This can be used to set a desired time that the run() method will be called.
         * For example, if frequency is set to 1000, then the run() will be called
         * every 1 second regardless the duration that your run() method.  If your run()
         * takes longer than the milliseconds you set, then it will be called without a
         * delay.
         * @param milliseconds The frequency to call your run() function.
         *                     Set this to zero to disable this functionality.
         * @note By default, run() method is called continuously without a delay.
         */
        inline void setRunDuration(uint32_t milliseconds) { mTaskDelayMs = milliseconds; }

        /// @returns the run duration set by setRunDuration()
        inline uint32_t getRunDuration(void) const { return mTaskDelayMs; }

    #if (0 != configUSE_QUEUE_SETS)
        /**
         * Initialize the queue set to block on multiple queues and/or semaphores.
         * @param queueSetSize  The size of COMBINED queue set, which is the total size
         *          of all the queues.  Semaphore size is always 1, unless it is a
         *          counting semaphore. @see http://www.freertos.org/xQueueCreateSet.html
         * @param count The number of queue or semaphore handles.
         * @warning Your Queues should be EMPTY and your semaphores should be TAKEN (empty)
         *          otherwise queue set will not work.
         *
         * @code
         *      // For two binary semaphores :
         *      initQueueSet(2, 2, mySem1, mySem2);
         *
         *      // For Counting semaphore of 2, and queue size of 3 :
         *      initQueueSet(5, 2, myCntSemWithSize2, myQueueWithSize3);
         * @endcode
         */
        void initQueueSet(uint32_t queueSetSize, uint32_t count, ...);

        /** @{ Queue set API.
         * Once a queue set has been created, we can specify the block time after
         * which your run() method will be called.  By default it is 1 second.
         * @code
         *      // Do this in your init() or similar method :
         *      setQueueSetBlockTime(portMAX_DELAY);
         *
         *      // In your run, you can now select on queue set items
         *      void run()
         *      {
         *          if (getQueueSetSelection() == mySem) {
         *              xSemaphoreTake(mySem, 0);
         *              // Do something
         *          }
         *          else if (getQueueSetSelection() == myQueue) {
         *              xQueueReceive(myQueue, &myItem, 0);
         *              // Do something
         *          }
         *
         *          return true;
         *      }
         * @endcode
         */
        inline void setQueueSetBlockTime(TickType_t t) { mQueueSetBlockTime = t; }
        inline QueueSetHandle_t getQueueSetSelection(void) { return mQueueSetType; }
        /** @} */
    #endif

        /**
         * Set the update rate in milliseconds that the free stack size is calculated at.
         * Default rate is 60 seconds; zero is to disable it.
         */
        inline void setStatUpdateRate(uint32_t rateMs) { mStatUpdateRateMs = rateMs; }

    private:
        /* Hide the constructor that user is not supposed to use.
         * Need this for const member initialization
         */
        scheduler_task() :
    #if (0 != configUSE_QUEUE_SETS)
            mQueueSet(0), mQueueSetType(0), mQueueSetBlockTime(0),
    #endif
            mHandle(0), mFreeStack(0), mRunCount(0), mTaskDelayMs(0), mStatUpdateRateMs(0),
            mName(0), mParam(0), mStackSize(0), mPriority(0) {}

    #if (0 != configUSE_QUEUE_SETS)
        /** @{ Queue set members */
        QueueSetHandle_t mQueueSet;           ///< Queue set handle
        QueueSetMemberHandle_t mQueueSetType; ///< Queue set that was selected
        TickType_t mQueueSetBlockTime;        ///< Block time of queue set
        /** @} */
    #endif

        /** @{ Other member variables */
        TaskHandle_t mHandle;       ///< Task handle of this task
        uint32_t mFreeStack;        ///< Free stack of this task
        uint32_t mRunCount;         ///< run-count of run() method
        uint32_t mTaskDelayMs;      ///< To set task frequency
        uint32_t mStatUpdateRateMs; ///< Statistics update rate
        /** @} */

        /** @{ FreeRTOS task creation parameters */
        const char *mName;          ///< Task name
        const void *mParam;         ///< Parameter that will be passed to run()
        const uint32_t mStackSize;  ///< Stack size in bytes
        const uint8_t mPriority;    ///< Task priority
        /** @} */

        /** @{ Give access to our private members to these functions */
        friend bool scheduler_init_all(bool register_task_tlm);
        friend void scheduler_c_task_private(void *param);
        /** @} */
};



#endif /* CPP_TASK_HPP_ */
