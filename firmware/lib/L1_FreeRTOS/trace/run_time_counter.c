#include <FreeRTOS.h>
#include <stdint.h>



extern void vTracePortGetTimeStamp(uint32_t *pTimestamp);

unsigned trace_get_run_time_counter(void)
{
    uint32_t value = 0;
#if (configUSE_TRACE_FACILITY)
    vTracePortGetTimeStamp(&value);
#endif
    return value;
}
