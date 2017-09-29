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
#include "LPC17xx.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"       /* xTaskGetSchedulerState() */



/**
 * Once a conversion is complete, we put the result into a queue.
 * This avoids polling as the conversion routine can just wait
 * until the ISR inputs the value into the queue.
 */
QueueHandle_t g_adc_result_queue = 0;

/// This is the mutex such that only one ADC conversion is performed at a time
SemaphoreHandle_t g_adc_mutex = 0;



/**
 * This is the ADC interrupt mapped to startup.cpp IRQ name.
 * This is called by the CPU core when ADC interrupt occurs.
 */
void ADC_IRQHandler(void)
{
    const uint16_t twelve_bits = 0x0FFF;
    BaseType_t switch_required = 0;

    uint16_t result = (LPC_ADC->ADGDR >> 4) & twelve_bits;
    xQueueSendFromISR(g_adc_result_queue, &result, &switch_required);

    portEND_SWITCHING_ISR(switch_required);
}

void adc0_init()
{
    uint32_t i = 0;
    const uint32_t max_adc_clock = (13 * 1000UL * 1000UL); // 13Mhz is max ADC clock in datasheet
    const uint32_t adc_clock = (sys_get_cpu_clock() / 8);  // We configure ADC clock = CPU/8
    const uint32_t enable_adc_bitmask = (1 << 21);

    lpc_pconp(pconp_adc, true);
    lpc_pclk(pclk_adc, clkdiv_8);
    LPC_ADC->ADCR = enable_adc_bitmask;

    // Calculate and set prescalar for ADC based on maximum ADC clock
    for(i=2; i < 255; i+=2) {
        if( (adc_clock / i) < max_adc_clock) {
            LPC_ADC->ADCR |= (i << 8);
            break;
        }
    }

    g_adc_mutex = xSemaphoreCreateMutex();
    g_adc_result_queue = xQueueCreate(1, sizeof(uint16_t));

    // Optional: Provide names of the FreeRTOS objects for the Trace Facility
    vTraceSetMutexName(g_adc_mutex,        "ADC Mutex");
    vTraceSetQueueName(g_adc_result_queue, "ADC RX-Q");
    vTraceSetISRProperties(ADC_IRQn,       "ADC", IP_adc);

    NVIC_EnableIRQ(ADC_IRQn);
}

static inline void adc0_start_conversion(uint8_t channel_num)
{
    const uint32_t start_conversion = (1 << 24);
    const uint32_t channel_masks = 0xFF;
    const uint32_t start_conversion_masks = (7 << 24); // Clear B26:B25:B24

    // Clear previously selected channel
    LPC_ADC->ADCR &= ~(channel_masks | start_conversion_masks);

    // Set the channel number and start the conversion now
    LPC_ADC->ADCR |= (1 << channel_num) | start_conversion;
}

uint16_t adc0_get_reading(uint8_t channel_num)
{
    uint16_t result = 0;
    const uint8_t max_channels = 8;

    if (channel_num >= max_channels) {
        result = 0;
    }
    else if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState())
    {
        xSemaphoreTake(g_adc_mutex, portMAX_DELAY);
        {
            adc0_start_conversion(channel_num);
            xQueueReceive(g_adc_result_queue, &result, portMAX_DELAY);
        }
        xSemaphoreGive(g_adc_mutex);
    }
    else
    {
        adc0_start_conversion(channel_num);
        while(! xQueueReceive(g_adc_result_queue, &result, 0))
        {
            ;
        }
    }

    return result;
}
