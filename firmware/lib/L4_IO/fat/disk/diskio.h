#ifndef _DISKIO
#define _DISKIO
#ifdef __cplusplus
extern "C" {
#endif


#include "disk_defines.h"
#include "diskioStructs.h"  // DSTATUS

/// Enumeration of the Drive numbers :
typedef enum {
    driveNumFlashMem = 0,
    driveNumSdCard = 1
} DriveNumberType;

/**
 * Initializes the disk given by @param drv
 */
DSTATUS disk_initialize(BYTE drv);

/**
 * @returns Disk status of the @param drv
 */
DSTATUS disk_status(BYTE drv);

/**
 * Performs low level disk read
 * @param drv   The disk drive to read from
 * @param buff  The pointer to the buffer to read data to
 * @param sector The sector number to read
 * @param count  The number of sectors to read
 * @returns      The status of the read operation
 */
DRESULT disk_read (BYTE drv, BYTE *buff, DWORD sector, BYTE count);

/**
 * Performs low level disk write
 * @param drv   The disk drive to write data to
 * @param buff  The pointer to the buffer to write data from
 * @param sector The sector number to write
 * @param count  The number of sectors to write
 * * @returns     The status of the write operation
 */
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count);

/**
 * Gets control data of the disk
 * @param drv   The drive number to get data from
 * @param ctrl  The control data type to get
 * @param buff  The buffer to write data to
 * @returns     The status of the request
 */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff);



#ifdef __cplusplus
}
#endif
#endif
