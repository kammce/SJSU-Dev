/**
 * @file  spi_flash.h
 * @brief SPI Flash Driver complaint with FAT-FS System
 * @ingroup Board IO
 */
#ifndef SPI_FLASH_H_
#define SPI_FLASH_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "diskioStructs.h" // Used in flash_ioctl()



/**
 * Initializes the Flash Memory
 */
 DSTATUS flash_initialize();

/**
 * Reads a sector from the Flash Memory
 * @param pData The pointer to the data to save the read
 * @param sectorNum     The sector number to read with each sector being 512 bytes
 * @param sectorCount   The number of sectors to read
 */
DRESULT flash_read_sectors(unsigned char* pData, int sectorNum, int sectorCount);

/**
 * Writes a sector to the Flash Memory
 * @param pData The pointer to the data to write
 * @param sectorNum     The sector number to write with each sector being 512 bytes
 * @param sectorCount   The number of sectors to write
 */
DRESULT flash_write_sectors(unsigned char* pData, int sectorNum, int sectorCount);

/**
 * Gets control information from this flash memory drive, such as sector count, sector size
 * @param ctrl  The type of information to get
 * @param buff  The pointer to save the requested information to
 */
DRESULT flash_ioctl(BYTE ctrl, void *buff);

/**
 * Writes a permanent ID on the SPI Flash
 * @warning THIS CAN ONLY BE DONE ONCE IN A LIFETIME OF THE SPI FLASH
 * @warning DO NOT USE THIS FUNCTION WITHOUT THE SPI SEMAPHORE!!!
 */
void flash_write_permanent_id(char *id_64bytes);

/**
 * Reads the permanent ID programmed into the SPI flash
 * @warning DO NOT USE THIS FUNCTION WITHOUT THE SPI SEMAPHORE!!!
 */
void flash_read_permanent_id(char *id_64bytes);

/**
 * @{ Flash memory metadata functions
 * If the flash memory contains additional space for each page, then each time a page
 * is written, we also write its write-counter value.  You can then retrieve the
 * page write count values by the page number.
 */
uint32_t flash_get_page_count(void);
uint32_t flash_get_page_size(void);
bool flash_supports_metadata(void);

/// @warning DO NOT USE THIS FUNCTION WITHOUT THE SPI SEMAPHORE!!!
uint32_t flash_get_page_write_count(uint32_t page_number);
/** @} */

/**
 * This will ERASE the entire chip, including the meta-data!!
 * This can take several seconds to perform the chip erase...
 */
void flash_chip_erase(void);



#ifdef __cplusplus
 }
#endif
#endif /* SPI_FLASH_H_ */
