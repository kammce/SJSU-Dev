#ifndef _SDIO
#ifdef __cplusplus
extern "C" {
#endif



#define _READONLY	0	    /* 1: Read-only mode */
#define _USE_IOCTL	1

#include "diskioStructs.h"
#include "sd_defines.h"	// Platform dependent calls should be in this file!



DSTATUS sd_initialize();             ///< Initializes the SD Card for SPI Mode, called automatically by disk_initialize()
DSTATUS sd_status();                 ///< Returns status of card (if its been initialized or not)

DRESULT sd_read (BYTE *buff, DWORD sector, BYTE count);	        ///< Reads a sector from the SD Card
DRESULT sd_write(const BYTE *buff, DWORD sector, BYTE count);	///< Writes a sector to the SD-Card
DRESULT sd_ioctl(BYTE ctrl,void *buff);							///< Low level function used by FAT File System Layer
void sd_update_card_status(void); 										///< Timeout function MUST BE CALLED AT 100Hz (every 10ms)



#ifdef __cplusplus
}
#endif
#define _SDIO
#endif
