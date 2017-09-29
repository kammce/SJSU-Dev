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

#include "lpc_sys.h"
#include "wireless.h"
#include "lpc_timers.h"
#include "io.hpp"

#include "FreeRTOS.h"
#include "task.h"



/// These bitmasks should match up with the timer MCR register to trigger interrupt upon match
enum {
    mr0_mcr_for_overflow          = (UINT32_C(1) << 0),
    mr1_mcr_for_mesh_bckgnd_task  = (UINT32_C(1) << 3),
    mr2_mcr_for_ir_sensor_timeout = (UINT32_C(1) << 6),
    mr3_mcr_for_watchdog_reset    = (UINT32_C(1) << 9),
};

/// Periodic interrupt for mesh networking.  This timer match interrupt is disabled if FreeRTOS starts to run.
#define LPC_SYS_TIME_FOR_BCKGND_TASK_US     (1 * 1000)

/// Time in microseconds that will feed the watchdog, which should be roughly half of the actual watchdog reset
#define LPC_SYS_WATCHDOG_RESET_TIME_US      ((SYS_CFG_WATCHDOG_TIMEOUT_MS / 2) * 1000)

/// Timer overflow interrupt will increment this upon the last UINT32 value, 16-bit is enough for many years!
static volatile uint16_t g_timer_rollover_count = 0;

/// Pointer to the timer struct based on SYS_CFG_SYS_TIMER
LPC_TIM_TypeDef *gp_timer_ptr = NULL;



extern "C" void lpc_sys_setup_system_timer(void)
{
    // Note: Timer1 is required for IR sensor's decoding logic since its pin is tied to Timer1 Capture Pin
    const lpc_timer_t sys_timer_source = (lpc_timer_t) SYS_CFG_SYS_TIMER;

    // Get the IRQ number of the timer to enable the interrupt
    const IRQn_Type timer_irq = lpc_timer_get_irq_num(sys_timer_source);

    // Initialize the timer structure pointer
    gp_timer_ptr = lpc_timer_get_struct(sys_timer_source);

    // Setup the timer to tick with a fine-grain resolution
    const uint32_t one_micro_second = 1;
    lpc_timer_enable(sys_timer_source, one_micro_second);

    /**
     * MR0: Setup the match register to take care of the overflow.
     * Upon the roll-over, we increment the roll-over count and the timer restarts from zero.
     */
    gp_timer_ptr->MR0 = UINT32_MAX;

    // MR1: Setup the periodic interrupt to do background processing
    gp_timer_ptr->MR1 = LPC_SYS_TIME_FOR_BCKGND_TASK_US;

#if (1 == SYS_CFG_SYS_TIMER)
    // MR2: IR code timeout when timer1 is used since IR receiver is tied to timer1 capture pin
    gp_timer_ptr->MR2 = 0;
#else
    #warning "IR receiver will not work unless SYS_CFG_SYS_TIMER uses TIMER1, so set it to 1 if possible"
#endif

    /* Setup the first match interrupt to reset the watchdog */
    gp_timer_ptr->MR3 = LPC_SYS_WATCHDOG_RESET_TIME_US;

    // Enable the timer match interrupts
    gp_timer_ptr->MCR = (mr0_mcr_for_overflow | mr1_mcr_for_mesh_bckgnd_task | mr3_mcr_for_watchdog_reset);

    // Only if we have got TIMER1, we can use IR sensor timeout match interrupt
#if (1 == SYS_CFG_SYS_TIMER)
    gp_timer_ptr->MCR |= (mr2_mcr_for_ir_sensor_timeout);
#endif

    /* Enable the interrupt and use higher priority than other peripherals because we want
     * to drive the periodic ISR above other interrupts since we reset the watchdog timer.
     */
    NVIC_SetPriority(timer_irq, IP_high);
    vTraceSetISRProperties(timer_irq, "AUX Timer", IP_high);
    NVIC_EnableIRQ(timer_irq);
}

extern "C" uint64_t sys_get_uptime_us(void)
{
    uint32_t before    = 0;
    uint32_t after     = 0;
    uint32_t rollovers = 0;

    /**
     * Loop until we can safely read both the rollover value and the timer value.
     * When the timer rolls over, the TC value will start from zero, and the "after"
     * value will be less than the before value in which case, we will loop again
     * and pick up the new rollover count.  This avoid critical section and simplifies
     * the logic of reading higher 16-bit (roll-over) and lower 32-bit (timer value).
     */
    do
    {
        before    = gp_timer_ptr->TC;
        rollovers = g_timer_rollover_count;
        after     = gp_timer_ptr->TC;
    } while (after < before);

    // each rollover is 2^32 or UINT32_MAX
    return (((uint64_t)rollovers << 32) | after);
}

/**
 * Actual ISR function (@see startup.cpp)
 */
#if (0 == SYS_CFG_SYS_TIMER)
extern "C" void TIMER0_IRQHandler()
#elif (1 == SYS_CFG_SYS_TIMER)
extern "C" void TIMER1_IRQHandler()
#elif (2 == SYS_CFG_SYS_TIMER)
extern "C" void TIMER2_IRQHandler()
#elif (3 == SYS_CFG_SYS_TIMER)
extern "C" void TIMER3_IRQHandler()
#else
#error "SYS_CFG_SYS_TIMER must be between 0-3 inclusively"
void TIMERX_BAD_IRQHandler()
#endif
{
    enum {
        timer_mr0_intr_timer_rollover     = (1 << 0),
        timer_mr1_intr_mesh_servicing     = (1 << 1),
        timer_mr2_intr_ir_sensor_timeout  = (1 << 2),
        timer_mr3_intr_for_watchdog_rst   = (1 << 3),

        timer_capt0_intr_ir_sensor_edge_time_captured = (1 << 4),
        timer_capt1_intr = (1 << 5),
    };

    const uint32_t intr_reason = gp_timer_ptr->IR;

#if (1 == SYS_CFG_SYS_TIMER)
    /* ISR for captured time of the capture input pin */
    if (intr_reason & timer_capt0_intr_ir_sensor_edge_time_captured)
    {
        gp_timer_ptr->IR = timer_capt0_intr_ir_sensor_edge_time_captured;

        // Store the IR capture time and setup timeout of the IR signal (unless we reset it again)
        IS.storeIrCode(gp_timer_ptr->CR0);
        gp_timer_ptr->MR2 = 10000 + gp_timer_ptr->TC;
    }
    /* MR2: End of IR capture (no IR capture after initial IR signal) */
    else if (intr_reason & timer_mr2_intr_ir_sensor_timeout)
    {
        gp_timer_ptr->IR = timer_mr2_intr_ir_sensor_timeout;
        IS.decodeIrCode();
    }
    /* MR0 is used for the timer rollover count */
    else
#endif
    if(intr_reason & timer_mr0_intr_timer_rollover)
    {
        gp_timer_ptr->IR = timer_mr0_intr_timer_rollover;
        ++g_timer_rollover_count;
    }
    else if(intr_reason & timer_mr1_intr_mesh_servicing)
    {
        gp_timer_ptr->IR = timer_mr1_intr_mesh_servicing;

        /* FreeRTOS task is used to service the wireless_service() function, otherwise if FreeRTOS
         * is not running, timer ISR will call this function to carry out mesh networking logic.
         */
        if (taskSCHEDULER_RUNNING != xTaskGetSchedulerState()) {
            wireless_service();
        }
        else {
            /* Disable this timer interrupt if FreeRTOS starts to run */
            gp_timer_ptr->MCR &= ~(mr1_mcr_for_mesh_bckgnd_task);
        }

        /* Setup the next periodic interrupt */
        gp_timer_ptr->MR1 += gp_timer_ptr->TC + LPC_SYS_TIME_FOR_BCKGND_TASK_US;
    }
    else if (intr_reason & timer_mr3_intr_for_watchdog_rst) {
        gp_timer_ptr->IR = timer_mr3_intr_for_watchdog_rst;

        /* If no one feeds the watchdog, we will watchdog reset.  We are using a periodic ISR
         * to feed watchdog because if a critical exception hits, it will enter while(1) loop inside
         * the interrupt, and since watchdog won't reset, it will trigger system reset.
         */
        sys_watchdog_feed();

        /* Setup the next watchdog reset timer */
        gp_timer_ptr->MR3 = gp_timer_ptr->TC + LPC_SYS_WATCHDOG_RESET_TIME_US;
    }
    else
    {
        // Unexpected interrupt, so stay here to trigger watchdog interrupt
        puts("Unexpected ISR call at lpc_sys.c");
        while (1) {
            ;
        }
    }
}

extern "C" void sys_get_mem_info_str(char buffer[280])
{
    sys_mem_t info = sys_get_mem_info();
    sprintf(buffer,
            "Memory Information:\n"
            "Global Used   : %5d\n"
            "malloc Used   : %5d\n"
            "malloc Avail. : %5d\n"
            "System Avail. : %5d\n"

            "Next Heap ptr    : 0x%08X\n"
            "Last sbrk() ptr  : 0x%08X\n"
            "Last sbrk() size : %u\n"
            "Num  sbrk() calls: %u\n",

            (int)info.used_global, (int)info.used_heap, (int)info.avail_heap, (int)info.avail_sys,
            (unsigned int)info.next_malloc_ptr,
            (unsigned int)info.last_sbrk_ptr,
            (unsigned int)info.last_sbrk_size,
            (unsigned int)info.num_sbrk_calls);
}
