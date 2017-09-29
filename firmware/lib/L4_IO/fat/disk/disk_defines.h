#ifndef _DISK_DEFINES
#define _DISK_DEFINES
#ifdef __cplusplus
extern "C" {
#endif



/* Command code for disk_ioctrl() */
/* Generic command (used by FatFs) */
#define CTRL_SYNC           0   /* Flush disk cache (for write functions) */
#define GET_SECTOR_COUNT    1   /* Get media size (for only f_mkfs()) */
#define GET_SECTOR_SIZE     2   /* Get sector size (for multiple sector size (_MAX_SS >= 1024)) */
#define GET_BLOCK_SIZE      3   /* Get erase block size (for only f_mkfs()) */
#define CTRL_ERASE_SECTOR   4   /* Force erased a block of sectors (for only _USE_ERASE) */

/* Generic command (not used by FatFs) */
#define CTRL_POWER          5   /* Get/Set power status */
#define CTRL_LOCK           6   /* Lock/Unlock media removal */
#define CTRL_EJECT          7   /* Eject media */
#define CTRL_FORMAT         8   /* Create physical format on the media */

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT      0x01    /* Drive not initialized */
#define STA_NODISK      0x02    /* No medium in the drive */
#define STA_PROTECT     0x04    /* Write protected */



#ifdef __cplusplus
}
#endif
#endif
