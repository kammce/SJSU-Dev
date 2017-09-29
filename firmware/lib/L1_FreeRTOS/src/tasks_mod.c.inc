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

#if (1 == configGENERATE_RUN_TIME_STATS)
BaseType_t  uxTaskGetCpuUsage(TaskHandle_t xTask)
{
    tskTCB *pxTCB = prvGetTCBFromHandle( xTask );
    uint32_t totalRunTime;
    uint64_t taskRunTime;
    
    vPortEnterCritical();
    taskRunTime = pxTCB->ulRunTimeCounter;
#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
    portALT_GET_RUN_TIME_COUNTER_VALUE( totalRunTime );
#else
    totalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
#endif
    vPortExitCritical();

    return (taskRunTime * 100) / totalRunTime;
}

static void prvResetRunTimeStatsInList(List_t *pxList)
{
    volatile tskTCB *pxNextTCB, *pxFirstTCB;

    listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
    do
    {
        listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
        pxNextTCB->ulRunTimeCounter = 0;
    } while( pxNextTCB != pxFirstTCB );
}

void vTaskResetRunTimeStats(void)
{
    BaseType_t uxQueue = configMAX_PRIORITIES;
    vTaskSuspendAll();
    {
        do
        {
            uxQueue--;
            if (listLIST_IS_EMPTY(&(pxReadyTasksLists[uxQueue])) == pdFALSE)
            {
                prvResetRunTimeStatsInList(
                        (List_t *) &(pxReadyTasksLists[uxQueue]));
            }
        } while (uxQueue > ( unsigned portBASE_TYPE ) tskIDLE_PRIORITY);

        if (listLIST_IS_EMPTY(pxDelayedTaskList) == pdFALSE)
        {
            prvResetRunTimeStatsInList((List_t *) pxDelayedTaskList);
        }

        if (listLIST_IS_EMPTY(pxOverflowDelayedTaskList) == pdFALSE)
        {
            prvResetRunTimeStatsInList((List_t *) pxOverflowDelayedTaskList);
        }

#if ( INCLUDE_vTaskDelete == 1 )
        {
            if( listLIST_IS_EMPTY( &xTasksWaitingTermination ) == pdFALSE )
            {
                prvResetRunTimeStatsInList(( List_t * ) &xTasksWaitingTermination );
            }
        }
#endif

#if ( INCLUDE_vTaskSuspend == 1 )
        {
            if( listLIST_IS_EMPTY( &xSuspendedTaskList ) == pdFALSE )
            {
                prvResetRunTimeStatsInList(( List_t * ) &xSuspendedTaskList );
            }
        }
#endif
        ulTaskSwitchedInTime = 0;
        portRESET_TIMER_FOR_RUN_TIME_STATS();
    }
    xTaskResumeAll();
}

    void vRunTimeStatIsrEntry()
    {
        unsigned long ulTempCounter = portGET_RUN_TIME_COUNTER_VALUE();
        
        // Add the time spent in the previously running task's counter.
        if (NULL != pxCurrentTCB) {
            pxCurrentTCB->ulRunTimeCounter += ( ulTempCounter - ulTaskSwitchedInTime );
        }
        
        // Save the current time to log how long the ISR is going to take.
        ulTaskSwitchedInTime = ulTempCounter;
    }
    void vRunTimeStatIsrExit()
    {
        unsigned long ulTempCounter = portGET_RUN_TIME_COUNTER_VALUE();
        
        // ISR spent current time - entry of ISR, which was saved in ulTaskSwitchedInTime
        // We don't need this because whatever is the "leftover" for the counters, that is 
        // is the overhead itself!
        // ulISROverhead += ( ulTempCounter - ulTaskSwitchedInTime );
        
        // Save the current time so next time when task is switched out, it will use this time-stamp.
        ulTaskSwitchedInTime = ulTempCounter;
    }
#else /* configGENERATE_RUN_TIME_STATS */
    void vRunTimeStatIsrEntry()
    {
    }
    void vRunTimeStatIsrExit()
    {
    }
#endif
