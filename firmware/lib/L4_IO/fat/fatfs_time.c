#include "integer.h"    // DWORD
#include "rtc.h"        // RTC functions

/**
 * This function is called by FAT FS System to get system time
 * @return DWORD containing the time structure
 */
DWORD get_fattime()
{
    rtc_t sysTime = rtc_gettime();

    return ((DWORD) (sysTime.year - 1980) << 25)
            | ((DWORD) sysTime.month << 21)
            | ((DWORD) sysTime.day << 16)
            | ((DWORD) sysTime.hour << 11)
            | ((DWORD) sysTime.min << 5)
            | ((DWORD) sysTime.sec >> 1);
}
