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
 * This contains the period callback functions for the periodic scheduler
 *
 * @warning
 * These callbacks should be used for hard real-time system, and the priority of these
 * tasks are above everything else in the system (above the PRIORITY_CRITICAL).
 * The period functions SHOULD NEVER block and SHOULD NEVER run over their time slot.
 * For example, the 1000Hz take slot runs periodically every 1ms, and whatever you
 * do must be completed within 1ms.  Running over the time slot will reset the system.
 */

#include <stdint.h>
#include <stdio.h>
#include "io.hpp"
#include "periodic_callback.h"
#include "gpio.hpp"
#include "MAX7456.hpp"

/// This is the stack size used for each of the period tasks (1Hz, 10Hz, 100Hz, and 1000Hz)
const uint32_t PERIOD_TASKS_STACK_SIZE_BYTES = (512 * 4);

/**
 * This is the stack size of the dispatcher task that triggers the period tasks to run.
 * Minimum 1500 bytes are needed in order to write a debug file if the period tasks overrun.
 * This stack size is also used while calling the period_init() and period_reg_tlm(), and if you use
 * printf inside these functions, you need about 1500 bytes minimum
 */
const uint32_t PERIOD_DISPATCHER_TASK_STACK_SIZE_BYTES = (512 * 3);

void spi0_Init()
{
    LPC_SC->PCONP          |=  (1 << 21); // SPI1 Power Enable
    LPC_SC->PCLKSEL1       |=  (3 << 10); // CLK / 8

    // Select MISO, MOSI, and SCK pin-select functionality
    LPC_PINCON->PINSEL0    &= ~( (3 << 30) );
    LPC_PINCON->PINSEL1    &= ~( (3 <<  2) | (3 << 4) );
    LPC_PINCON->PINSEL0    |=  ( (2 << 30) );
    LPC_PINCON->PINSEL1    |=  ( (2 <<  2) | (2 << 4) );

    LPC_SSP0->CR0           =  7;         // 8-bit mode
    LPC_SSP0->CR1           =  (1 << 1);  // Enable SSP as Master
    LPC_SSP0->CPSR          =  60;         // SCK speed = CPU / 8
}

char spi0_ExchangeByte(char out)
{
    LPC_SSP0->DR = out;
    while(LPC_SSP0->SR & (1 << 4)); // Wait until SSP is busy
    return LPC_SSP0->DR;
}

/// Called once before the RTOS is started, this is a good place to initialize things once
bool period_init(void)
{
    return true; // Must return true upon success
}

/// Register any telemetry variables
bool period_reg_tlm(void)
{
    // Make sure "SYS_CFG_ENABLE_TLM" is enabled at sys_config.h to use Telemetry
    return true; // Must return true upon success
}

/**
 * Below are your periodic functions.
 * The argument 'count' is the number of times each periodic task is called.
 */
void period_1Hz(uint32_t count)
{
	LE.toggle(1);
}


void ChipSelect(bool Select)
{
	static GPIO ChipSelectPin(P0_0);
	static bool Initialized = false;
	if(!Initialized)
	{
		ChipSelectPin.setAsOutput();
		Initialized = true;
	}
	if(Select)
	{
		ChipSelectPin.setLow();
	}
	else
	{
		ChipSelectPin.setHigh();
	}
}

uint8_t SPITransfer(uint8_t Data)
{
	return spi0_ExchangeByte(Data);
}

void period_10Hz(uint32_t count)
{
	static MAX7456 Max(ChipSelect, SPITransfer);
	static bool Initialized = false;
	static char Buffer[64];
	static uint32_t i = 0;
	static uint32_t j = 0;
	static GPIO ResetPin(P0_1);
	if(!Initialized)
	{
		ResetPin.setAsOutput();
		ResetPin.setHigh();
		spi0_Init();
		Max.Initialize();
		Initialized = true;
		Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_CENTER, MAX7456::DISPLAY_MIDDLE), 0x4A);
		Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_CENTER+1, MAX7456::DISPLAY_MIDDLE), 0x4B);
	}
	else
	{
		sprintf(Buffer, "SCORE:%d", i++);
		Max.WriteString(Max.CoordsToPosition(0,0), Buffer);

		if(j < 5*3)
		{
			Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(5+3), 0), 0xED);
			if(j < 5)
			{
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(4+3), 0),   0xF2-j);
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(3+3), 0),   0xF2);
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(2+3), 0),   0xF2);
			}
			else if(j < 10)
			{
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(4+3), 0),   0xEE);
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(3+3), 0),   0xF2-(j % 5));
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(2+3), 0),   0xF2);
			}
			else if(j < 15)
			{
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(4+3), 0),   0xEE);
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(3+3), 0),   0xEE);
				Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(2+3), 0),   0xF2-(j % 5));
			}
			Max.WriteCharacterToScreen(Max.CoordsToPosition(MAX7456::DISPLAY_WIDTH-(1+3), 0), 0xF3);
			j++;
		}
		else
		{
			j = 0;
		}
	}
    LE.toggle(2);
}

void period_100Hz(uint32_t count)
{
    LE.toggle(3);
}

// 1Khz (1ms) is only run if Periodic Dispatcher was configured to run it at main():
// scheduler_add_task(new periodicSchedulerTask(run_1Khz = true));
#define BUTTONS 4
void period_1000Hz(uint32_t count)
{
	static bool Pressed[BUTTONS+1] = { 0 };

	for (int i = 0; i < BUTTONS+1; ++i)
	{
		if (SW.getSwitch(i))
		{
			Pressed[i] = true;
		}
		if(!SW.getSwitch(i) && Pressed[i])
		{
			switch(i)
			{
				case 1:
					printf("Button 1 released\n");
					break;
				case 2:
					printf("Button 2 released\n");
					break;
				case 3:
					printf("Button 3 released\n");
					break;
				case 4:
					printf("Button 4 released\n");
					break;
			}
			Pressed[i] = false;
		}
	}

    LE.toggle(4);

}
