#include "FreeRTOS.h"
#include "task.h"

#include "printf_lib.h"
#include "core_cm3.h"     // __WFI();
#include "utilities.h"
#include "lpc_sys.h"


void vApplicationIdleHook(void)
{
	// THIS FUNCTION MUST NOT BLOCK
	// Put CPU to IDLE here. RTOS will wake up CPU from OS timer interrupt.
	__WFI(); // Wait for Event: Puts the CPU in low powered mode
}

void vApplicationStackOverflowHook( TaskHandle_t *pxTask, char *pcTaskName )
{
    u0_dbg_put("HALTING SYSTEM: Stack overflow by task: ");
    u0_dbg_put((char*)pcTaskName);
    u0_dbg_put("\nTry increasing stack memory of this task.\n");

	delay_us(3000 * 1000);
	sys_reboot();
}

/* FreeRTOS tick hook*/
#if configUSE_TICK_HOOK
void vApplicationTickHook( void )
{
	// This function is called at every OS Tick
	// DO NOT PUT A LOT OF CODE HERE.  KEEP IT SHORT AND SIMPLE (if you really need it)
    #error "Are you sure you want to use this tick hook?"
}
#endif

#if( configUSE_MALLOC_FAILED_HOOK == 1 )
void vApplicationMallocFailedHook( void )
{
    u0_dbg_put("HALTING SYSTEM: Your system ran out of memory (RAM)!\n");

    delay_us(3000 * 1000);
    sys_reboot();
}
#endif

