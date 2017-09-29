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
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "scheduler_task.hpp"
#include "FreeRTOS.h"
#include "semphr.h"

#include "c_tlm_comp.h"
#include "c_tlm_var.h"
#include "c_tlm_stream.h"



/** @{ Private global variables */
static TaskHandle_t gTaskEntryTaskHandle = 0;    ///< Task handle that will call taskEntry() for everyone
static SemaphoreHandle_t gRunTaskSemaphore = 0;  ///< Semaphore for a task to proceed to run() method
/** @} */



/// Linked list of the tasks
typedef struct task_list {
    scheduler_task *task;   ///< The pointer to the task
    task_list *next;        ///< The next link
} task_list_t;

/// The instance of scheduler task list
static task_list_t *gpTaskList = NULL;



/** @{ Shared objects' stuff */
/// Array of pointers shared by index
typedef struct {
    uint32_t count;     ///< The memory size of pointers
    void **pointers;    ///< Array of pointers
} ptr_enum_pair_t;

/// Pair of a pointer and name used by getSharedObject() and addSharedObject()
typedef struct ptr_name_pair {
    void *obj_ptr;              ///< The pointer
    const char *name;           ///< The name of the pointer
    struct ptr_name_pair *next; ///< Pointer to the next linked list element
} ptr_name_pair_t;

/// Instance of shared object by index
static ptr_enum_pair_t gEnumObjects = { 0, 0 };

/// Instance of shared objects by string name
static ptr_name_pair_t *gpNamePairList = NULL;
/** @} */



/**
 * Print two strings followed by newline characters
 *
 * @warning About stack space and the use of printf():
 * WE PURPOSELY DO NOT USE printf() because it uses a lot of stack space.
 * puts() uses very little stack space.
 */
static bool g_dbg_print = false;
static void dbg_print(const char *one, const char *two=NULL)
{
    if (g_dbg_print)
    {
        while(*one) {
            putchar(*one++);
        }
        if (two) {
            puts(two);
        }
    }
}

static void printline(const char *one, const char *two=NULL)
{
    while(*one) {
        putchar(*one++);
    }

    if (two) {
        puts(two);
    }
    else {
        putchar('\n');
    }
}

/**
 * This is the C task that is created multiple times.  Each task receives its
 * own parameter therefore differentiating between different tasks.
 */
void scheduler_c_task_private(void *task_ptr)
{
    scheduler_task& task = *(scheduler_task*)task_ptr;

    /* Have the responsible task call taskEntry() for each task */
    if (gTaskEntryTaskHandle == task.getTaskHandle()) {
        bool failure = false;
        uint32_t taskCount = 0;

        dbg_print("*  ");
        dbg_print(task.mName, " task calling taskEntry() for all tasks ... ");
        dbg_print("*  Each task will then enter the run() loop\n");

        task_list_t *e = gpTaskList;
        while (NULL != e) {
            scheduler_task *t = e->task;
            e = e->next;
            ++taskCount;

            if (!t->taskEntry()) {
                printline(t->mName, "  --> FAILED taskEntry()");
                failure = true;
            }
        }

        if (failure) {
            printline("ERROR: Killing FreeRTOS due to error(s)");
            vTaskEndScheduler();
        }
        else {
            /* Give permission for everyone to start the run() loop */
            for (uint32_t i=0; i < taskCount; i++) {
                xSemaphoreGive(gRunTaskSemaphore);
            }
        }
    }

    // Wait until we're given the go ahead by the task giving semaphore above
    xSemaphoreTake(gRunTaskSemaphore, portMAX_DELAY);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xNextStatTime = xTaskGetTickCount();

    for (;;)
    {
        #if (0 != configUSE_QUEUE_SETS)
        if (task.mQueueSet) {
            task.mQueueSetType = xQueueSelectFromSet(task.mQueueSet, task.mQueueSetBlockTime);
        }
        #endif

        // Run the task code and suspend when an error occurs
        if (!task.run((void*)task.mParam)) {
            printline(task.mName, " --> FAILURE detected; suspending this task ...");
            vTaskSuspend(0);
        }
        ++(task.mRunCount);

        // Update the task statistics once in a short while :
        if (0 != task.mStatUpdateRateMs && xTaskGetTickCount() > xNextStatTime) {
            xNextStatTime = xTaskGetTickCount() + (task.mStatUpdateRateMs / MS_PER_TICK());
            task.mFreeStack = uxTaskGetStackHighWaterMark(task.mHandle);
            // Convert to stack size in bytes instead of 4 bytes for this ARM CPU
            task.mFreeStack *= sizeof(int);
        }

        // Delay if set
        if (task.mTaskDelayMs) {
            vTaskDelayUntil( &xLastWakeTime, OS_MS(task.mTaskDelayMs));
        }
    }
}

bool scheduler_init_all(bool register_internal_tlm)
{
    bool failure = false;
    uint32_t taskCount = 0;

    /* If no tasks ... */
    if (NULL == gpTaskList) {
        printline("ERROR: NO tasks added by scheduler_add_task()");
        failure = true;
        return failure;
    }

    dbg_print("*  Creating tasks ...\n");
    do {
        task_list_t *e = gpTaskList;
        while (NULL != e) {
            ++taskCount;
            scheduler_task *task = e->task;
            e = e->next;

            UBaseType_t taskPriority = task->mPriority;
#if BUILD_CFG_MPU
            taskPriority |= portPRIVILEGE_BIT;
#endif
            if (!xTaskCreate(scheduler_c_task_private,
                             task->mName,                    /* Name  */
                             STACK_BYTES(task->mStackSize),  /* Stack */
                             task,                           /* Task param    */
                             taskPriority,                   /* Task priority */
                             &(task->mHandle)))              /* Task Handle   */
            {
                printline(task->mName, "  --> FAILED xTaskCreate()");
                failure = true;
            }
        }
    } while(0);

    gRunTaskSemaphore = xSemaphoreCreateCounting(taskCount, 0);
    if (NULL == gRunTaskSemaphore) {
        printline("ERROR: Creating counting semaphore");
        failure = true;
    }
    else {
        vTraceSetSemaphoreName(gRunTaskSemaphore, "sch-cnt-sem");
    }

    /* Initialize all tasks */
    dbg_print("*  Initializing tasks ...\n");
    do {
        task_list_t *e = gpTaskList;
        while (NULL != e)
        {
            scheduler_task *task = e->task;
            e = e->next;

            if (!task->init()) {
                printline(task->getTaskName(), "  --> FAILED init()");
                failure = true;
            }
        }
    } while (0);

    /* Register telemetry for all tasks */
    #if SYS_CFG_ENABLE_TLM
    dbg_print("*  Registering tasks' telemetry ...\n");
    do {
        task_list_t *e = gpTaskList;
        while (NULL != e)
        {
            scheduler_task *task = e->task;
            e = e->next;

            if (!task->regTlm()) {
                failure = true;
            }

            /* Register statistical variables of this task */
            if (register_internal_tlm) {
                tlm_component *comp = tlm_component_add(task->mName);
                if (!tlm_variable_register(comp, "free_stack", &(task->mFreeStack),
                     sizeof(task->mFreeStack), 1, tlm_uint)) {
                    failure = true;
                }
                if (!tlm_variable_register(comp, "run_count", &(task->mRunCount),
                     sizeof(task->mRunCount), 1, tlm_uint)) {
                    failure = true;
                }
            }
            if (failure) {
                printline(task->mName, "  --> FAILED telemetry registration");
            }
        }

        dbg_print("*  Restoring disk telemetry\n");
        // Restore telemetry registered by "disk" component
        FILE *fd = fopen(SYS_CFG_DISK_TLM_NAME, "r");
        if (fd) {
            tlm_stream_decode_file(fd);
            fclose(fd);
        }
    } while (0);
    #endif

    /* Find the task with highest stack, and this task will then be used
     * to call taskEntry() for everyone once FreeRTOS starts.
     * We have one task do taskEntry() for all because otherwise tasks will perform
     * taskEntry() independent of each other, and one task might complete it and
     * start running without other tasks completing their taskEntry().
     */
    do {
        uint32_t highestStack = 0;
        task_list_t *e = gpTaskList;
        while (NULL != e) {
            scheduler_task *task = e->task;
            e = e->next;

            if (task->mStackSize > highestStack) {
                highestStack = task->mStackSize;
                gTaskEntryTaskHandle = task->mHandle;
            }
        }
    } while(0);

    return failure;
}

void scheduler_add_task(scheduler_task* task)
{
    if (NULL != task)
    {
        /* Insert new task at the beginning */
        task_list_t *newEntry = new task_list_t;
        if (NULL != newEntry) {
            newEntry->next = gpTaskList;
            newEntry->task = task;
            gpTaskList = newEntry;
        }
    }
}

void scheduler_start(bool dp, bool register_internal_tlm)
{
    g_dbg_print = dp;

    /* If no failure, start the FreeRTOS scheduler */
    if (!scheduler_init_all(register_internal_tlm))
    {
        dbg_print("*  Starting scheduler ...\n");

        // vTaskStartScheduler() should not return
        vTaskStartScheduler();
        printline("ERROR: Someone killed the scheduler");
    }
    else {
        printline("ERROR: Refusing to start OS scheduler due to error(s)");
    }
}



scheduler_task::scheduler_task(const char *name, uint32_t stack, uint8_t priority, void *param) :
#if (0 != configUSE_QUEUE_SETS)
   mQueueSet(0),
   mQueueSetType(0),
   mQueueSetBlockTime(1000),
#endif
   mHandle(0),
   mFreeStack(0),
   mRunCount(0),
   mTaskDelayMs(0),
   mStatUpdateRateMs(60 * 1000),
   mName(name),
   mParam(param),
   mStackSize(stack),
   mPriority(priority)
{

}

uint8_t scheduler_task::getTaskCpuPercent(void) const
{
    return uxTaskGetCpuUsage(getTaskHandle());
}

// static function:
uint8_t scheduler_task::getSysCpuPercent(void)
{
    return (100 - getSysIdlePercent());
}

// static function:
uint8_t scheduler_task::getSysIdlePercent(void)
{
    return uxTaskGetCpuUsage(xTaskGetIdleTaskHandle());
}

#if (0 != configUSE_QUEUE_SETS)
void scheduler_task::initQueueSet(uint32_t queueSetSize, uint32_t count, ...)
{
    void *handle = 0;
    va_list vl;
    va_start(vl, count);

    mQueueSet = xQueueCreateSet(queueSetSize);
    while(count--) {
        handle = va_arg(vl, void*);
        xQueueAddToSet( handle, mQueueSet);
    }

    va_end(vl);
}
#endif



/**
 * Static functions:
 */
scheduler_task* scheduler_task::getTaskPtrByName(const char *name)
{
    task_list_t *e = gpTaskList;
    while (NULL != e) {
        scheduler_task *task = e->task;
        e = e->next;

        if (0 == strcmp(name, task->getTaskName())) {
            return task;
        }
    }
    return NULL;
}

bool scheduler_task::addSharedObject(const char *name, void *obj_ptr)
{
    bool ok = false;

    if (NULL != name && NULL != obj_ptr)
    {
        /* Disallow adding duplicate items by name */
        ptr_name_pair_t *e = gpNamePairList;
        while (NULL != e) {
            if (0 == strcmp(e->name, name)) {
                break;
            }
            e = e->next;
        }

        /* e must be NULL if not a duplicate */
        if (NULL == e) {
            ptr_name_pair_t *newEntry = new ptr_name_pair_t;
            if (NULL != newEntry) {
                ok = true;

                newEntry->name    = name;
                newEntry->obj_ptr = obj_ptr;
                newEntry->next    = gpNamePairList;
                gpNamePairList = newEntry;
            }
        }
    }

    return ok;
}

void* scheduler_task::getSharedObject(const char *name)
{
    void *ret = NULL;

    ptr_name_pair_t *e = gpNamePairList;
    while (NULL != e) {
        if (0 == strcmp(e->name, name)) {
            ret = e->obj_ptr;
            break;
        }
        e = e->next;
    }

    return ret;
}

bool scheduler_task::addSharedObject(uint8_t index, void *obj)
{
    // Reallocate memory if the current index is not less than the count
    if (index >= gEnumObjects.count)
    {
        // Reallocate in increments of the number of objects defined by memGrowth
        const uint32_t memGrowth = 4;
        const uint32_t newCount  = memGrowth + (index/memGrowth)*memGrowth;

        /* Allocate space for new objects */
        gEnumObjects.pointers = (void**) realloc(gEnumObjects.pointers, newCount * sizeof(void*));
        if (NULL == gEnumObjects.pointers) {
            return false;
        }

        /* NULL out the new pointers we allocated above */
        for (uint8_t i = gEnumObjects.count; i < newCount; i++) {
            gEnumObjects.pointers[i] = NULL;
        }
        gEnumObjects.count = newCount;
    }

    gEnumObjects.pointers[index] = obj;
    return true;
}

void* scheduler_task::getSharedObject(uint8_t index)
{
    return (index < gEnumObjects.count) ? gEnumObjects.pointers[index] : NULL;
}
