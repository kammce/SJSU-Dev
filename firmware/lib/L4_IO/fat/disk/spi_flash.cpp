#include <stdint.h>
#include <string.h>

#include "spi_flash.h"
#include "ssp1.h"
#include "disk_defines.h"
#include "bio.h"            // flash cs and ds
#include "fat/ff.h"         // FR_OK and FR_DISK_ERR



/**
 * @file
 * This file supports Adesto Flash (formerly Atmel data flash)
 * It should work with the following part numbers :
 *   8mbit : AT45DB081E
 *  16mbit : AT45DB161E
 *  32mbit : AT45DB321E
 *  64mbit : AT45DB641E
 *
 *  Any other model will also work as long as the page size or the minimum size
 *  we can program is between 256 - 528 inclusively.
 */

/** @{ SPI data exchange functions */
static inline uint8_t flash_spi_io(uint8_t b)           {   return ssp1_exchange_byte(b);   }
static inline void flash_spi_multi_io(void *p, int len) {   ssp1_exchange_data(p, len);     }
/** @} */

/**
 * Macro to select and deselect SPI device during an operation.
 * When we run in high frequency (fast CPU clock), we need to make sure there is at least
 * 50ns difference between chip-selects, so we issue board_io_flash_ds() multiple times
 * on purpose
 */
#define CHIP_SELECT_OP()            for(uint8_t ___i = board_io_flash_cs();\
                                         ___i; \
                                         ___i = (board_io_flash_ds() || board_io_flash_ds() || board_io_flash_ds()))

/// This should match BYTE #1 of manufacturer and device ID information
#define FLASH_MANUFACTURER_ID       (0x1F)

/// Minimum sector size that works with FATFS (do not change this)
#define FLASH_SECTOR_SIZE           (512)

/// @{ Page size defines.  We support 256-528 byte page size.  Do not change these.
#define FLASH_PAGESIZE_256          256
#define FLASH_PAGESIZE_512          512
#define FLASH_PAGESIZE_264          264
#define FLASH_PAGESIZE_528          528
/// @}

/**
 * Bit number for specifying flash page offset when using non-standard page-size
 *
 * Example Page read/write with 264 bytes or 528 bytes:
 * 3 address bytes : | 23:16 | 15:8 | 7:0 |
 *
 * First 3 bits are don't care.
 * Next 12-bits specify page number.
 * Last 9 specify byte offset.
 */
#define FLASH_PAGENUM_BIT_OFFSET   9

/// Function pointer of I/O operation
typedef void (*flash_io_func_t) (uint8_t *data, const uint32_t addr, const uint32_t size);

/// Flash device opcodes
typedef enum {
    opcode_status_reg        = 0xD7,
    opcode_get_sig           = 0x9F,

    opcode_read_continous    = 0xE8, ///< Works up to 66Mhz but requires 4 dummy bytes
    opcode_read_cont_lowfreq = 0x03, ///< Works up to 33Mhz

    /**
     * @{ Memory write options:
     * Easiest way to write a page is using opcode_prog_thru_buffer1.
     * Efficient way to write a page (if flash is busy) is to write buffer 1 (while busy)
     * then perform page erase and buffer1 to memory without built-in-erase
     */
    opcode_page_erase        = 0x81,
    opcode_prog_thru_buffer1 = 0x82,
    opcode_write_buffer1     = 0x84,
    opcode_buffer1_to_mem_no_builtin_erase = 0x88,
    /** @} */

    opcode_read_security_reg  = 0x77,
    opcode_write_security_reg = 0x9B,
} flash_opcode_t;

/// This should match BYTE #2 of manufacturer and device ID information
typedef enum {
    flash_cap_invalid = 0,

    flash_cap_8mbit   = 0x25,
    flash_cap_16mbit  = 0x26,
    flash_cap_32mbit  = 0x27,
    flash_cap_64mbit  = 0x28,

    /**
     * @{
     * SPI Flash signature must fall in between the capacity IDs
     * for the initialization to be considered successful
     */
    flash_cap_first_valid = flash_cap_8mbit,
    flash_cap_last_valid  = flash_cap_64mbit,
    /** @} */

} flash_cap_t;

/// @{ Private variables
static flash_cap_t g_flash_capacity = flash_cap_invalid;
static uint16_t g_flash_pagesize    = 0;
static uint32_t g_sector_count = 0;
/// @}



/** @{ Private Functions used at this file */
static uint32_t flash_get_mem_size_bytes(void)
{
    switch (g_flash_capacity) {
        case flash_cap_8mbit  : return (8/8  * 1024 * 1024);
        case flash_cap_16mbit : return (16/8 * 1024 * 1024);
        case flash_cap_32mbit : return (32/8 * 1024 * 1024);
        case flash_cap_64mbit : return (64/8 * 1024 * 1024);
        default: return 0;
    }
}

static inline uint32_t flash_get_metadata_addr_from_pageaddr(const uint32_t addr)
{
    const uint32_t byte_offset =
            (g_flash_pagesize == FLASH_PAGESIZE_264) ? FLASH_PAGESIZE_256 : FLASH_PAGESIZE_512;
    return (addr | byte_offset);
}

static inline void flash_send_op_addr(const flash_opcode_t opcode, const uint32_t addr)
{
    uint8_t data[] = { (uint8_t)opcode, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)(addr >> 0)};
    flash_spi_multi_io(&data[0], sizeof(data));
}

static uint8_t flash_wait_for_ready()
{
    const uint8_t busybit = (1 << 7); ///< "1" means device is ready
    uint8_t status = 0;

    CHIP_SELECT_OP()
    {
        flash_spi_io(opcode_status_reg);
        do {
            status = flash_spi_io(0xFF);
        } while (! (status & busybit));
    }

    return status;
}

static void flash_write_page(uint8_t *data, const uint32_t addr, const uint32_t size)
{
    uint32_t writeCounter = 0xFFFFFFFF;

    /* wait for any previous write operation to finish */
    flash_wait_for_ready();

    /* If page-size is not 256 or 512, we can read-back metadata of the page
     * and use it as a "write" counter
     * The address will be in terms of the page number, we just need to offset
     * the address to the meta data address by adding the byte offset
     */
    const bool meta_data_exists = flash_supports_metadata();
    if (meta_data_exists)
    {
        CHIP_SELECT_OP()
        {
            flash_send_op_addr(opcode_read_cont_lowfreq, flash_get_metadata_addr_from_pageaddr(addr));
            flash_spi_multi_io(&writeCounter, sizeof(writeCounter));
        }
    }

    CHIP_SELECT_OP()
    {
        flash_send_op_addr(opcode_prog_thru_buffer1, addr);
        ssp1_dma_transfer_block(data, size, 1);

        if (meta_data_exists) {
            ++writeCounter;
            flash_spi_multi_io(&writeCounter, sizeof(writeCounter));
        }
    }
}

static void flash_read_page(uint8_t *data, const uint32_t addr, const uint32_t size)
{
    CHIP_SELECT_OP()
    {
        uint8_t op[] = {opcode_read_cont_lowfreq,
                        (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)(addr >> 0),
        };

        flash_spi_multi_io(&op[0], sizeof(op));
        ssp1_dma_transfer_block(data, size, 0);
    }
}

static void flash_perform_page_io_of_fatfs_sector(flash_io_func_t func, uint8_t* pData, uint32_t addr)
{
    const uint32_t halfsector = FLASH_SECTOR_SIZE / 2;

    /* simplest case when page size matches our sector size */
    if (FLASH_SECTOR_SIZE == g_flash_pagesize) {
        func(pData, addr, FLASH_SECTOR_SIZE);
    }
    /* next simplest case when page size is half of the sector size */
    else if (halfsector == g_flash_pagesize) {
        func(pData, addr, halfsector);
        func((pData + halfsector), (addr + halfsector), halfsector);
    }
    /* If pages are 528 bytes, we need to calculate the real address here */
    else if (FLASH_PAGESIZE_528 == g_flash_pagesize) {
        const uint32_t pagenum = (addr / FLASH_SECTOR_SIZE);
        /* 528 byte page requires 10 address bits, then 12 page number bits, and 2 dummy bits */
        addr = (pagenum << (FLASH_PAGENUM_BIT_OFFSET + 1));
        func(pData, addr, FLASH_SECTOR_SIZE);
    }
    /* If pages are 264 bytes, we need to read two of them with different addresses */
    else if (FLASH_PAGESIZE_264 == g_flash_pagesize) {
        const uint32_t pagenum = (addr / halfsector);

        addr = (pagenum + 0) << FLASH_PAGENUM_BIT_OFFSET;
        func(pData, addr, halfsector);

        addr = (pagenum + 1) << FLASH_PAGENUM_BIT_OFFSET;
        func((pData + halfsector), addr, halfsector);
    }
}
/** @} */



DSTATUS flash_initialize()
{
    uint8_t sig1 = 0;
    uint8_t sig2 = 0;
    const uint8_t status = flash_wait_for_ready();
    const uint8_t std_page_size_bit = (1 << 0);
    g_flash_pagesize = 0;

    CHIP_SELECT_OP()
    {
        uint8_t data[] = { opcode_get_sig, 0xFF, 0xFF };
        flash_spi_multi_io(&data[0], sizeof(data));
        sig1 = data[1];
        sig2 = data[2];
    }

    if (FLASH_MANUFACTURER_ID == sig1 &&
        (sig2 >= flash_cap_first_valid && sig2 <= flash_cap_last_valid)
        )
    {
        g_flash_capacity = (flash_cap_t) sig2;

        // 8-mbit version has 256/264 byte page size, 16-mbit has 512/528 byte page size
        if (flash_cap_8mbit == g_flash_capacity) {
            g_flash_pagesize = (status & std_page_size_bit) ? FLASH_PAGESIZE_256 : FLASH_PAGESIZE_264;
        }
        else {
            g_flash_pagesize = (status & std_page_size_bit) ? FLASH_PAGESIZE_512 : FLASH_PAGESIZE_528;
        }

        g_sector_count = flash_get_mem_size_bytes() / FLASH_SECTOR_SIZE;
    }

    return (0 == g_flash_pagesize) ? FR_DISK_ERR : FR_OK;
}

DRESULT flash_read_sectors(unsigned char *pData, int sectorNum, int sectorCount)
{
    uint32_t addr = (sectorNum * FLASH_SECTOR_SIZE);

    if ((uint32_t) (sectorNum + sectorCount - 1) > g_sector_count)
    {
        return RES_ERROR;
    }

    /* Wait for any pending write operation to finish.  Once flash is ready, then
     * we no longer need to perform this operation to read more sectors
     */
    flash_wait_for_ready();

    for(int i = 0; i < sectorCount; i++)
    {
        flash_perform_page_io_of_fatfs_sector(flash_read_page, pData, addr);
        addr  += FLASH_SECTOR_SIZE;
        pData += FLASH_SECTOR_SIZE;
    }

    return RES_OK;
}

DRESULT flash_write_sectors(unsigned char *pData, int sectorNum, int sectorCount)
{
    uint32_t addr = (sectorNum * FLASH_SECTOR_SIZE);

    if ((uint32_t) (sectorNum + sectorCount - 1) > g_sector_count)
    {
        return RES_ERROR;
    }

    for(int i = 0; i < sectorCount; i++)
    {
        flash_perform_page_io_of_fatfs_sector(flash_write_page, pData, addr);
        addr  += FLASH_SECTOR_SIZE;
        pData += FLASH_SECTOR_SIZE;
    }

    return RES_OK;
}

DRESULT flash_ioctl(BYTE ctrl,void *buff)
{
    DRESULT status = RES_PARERR;

    switch(ctrl)
    {
        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
            status = RES_OK;
            break;

        // Flush any pending write operation
        case CTRL_SYNC:
            flash_wait_for_ready();
            status = RES_OK;
            break;

        // Used by mkfs() while formatting the memory
        case GET_SECTOR_COUNT:
            *(DWORD*) buff = (DWORD) ((flash_get_mem_size_bytes() / FLASH_SECTOR_SIZE));
            status = RES_OK;
            break;

        case GET_SECTOR_SIZE:
            *(WORD*) buff = FLASH_SECTOR_SIZE;
            status = RES_OK;
            break;

        // Used by mkfs() while aligning memory
        case GET_BLOCK_SIZE:
            *(DWORD*) buff = 1; /* Block size is unknown */
            status = RES_OK;
            break;

        case CTRL_ERASE_SECTOR:
            status = RES_OK;
            break;

        default:
            status = RES_PARERR;
            break;
    }

    return status;
}

void flash_write_permanent_id(char *id_64bytes)
{
    char id_bytes[64] = { 0 };
    memcpy(id_bytes, id_64bytes, sizeof(id_bytes));

    CHIP_SELECT_OP()
    {
        flash_send_op_addr(opcode_write_security_reg, 0);
        flash_spi_multi_io(&id_bytes[0], sizeof(id_bytes));
    }
}

void flash_read_permanent_id(char *id_64bytes)
{
    CHIP_SELECT_OP()
    {
        flash_send_op_addr(opcode_read_security_reg, 0);
        flash_spi_multi_io(id_64bytes, 64);
    }
}

uint32_t flash_get_page_count(void)
{
    /* We want to divide by 256 or 512 but not 264 or 528 because the size
     * reported by flash_get_mem_size_bytes() assumes 256/512 byte page.
     */
    const uint32_t rounded_page_size = (g_flash_pagesize & ~0x0000001F);
    return (0 == rounded_page_size) ? 0 : (flash_get_mem_size_bytes() / rounded_page_size);
}

uint32_t flash_get_page_size(void)
{
    return g_flash_pagesize;
}

bool flash_supports_metadata(void)
{
    return (0 != (g_flash_pagesize % FLASH_PAGESIZE_256));
}

uint32_t flash_get_page_write_count(uint32_t page_number)
{
    /* Metadata is at the end of the page */
    const uint32_t page_addr = (page_number << FLASH_PAGENUM_BIT_OFFSET);
    const uint32_t meta_data_addr = flash_get_metadata_addr_from_pageaddr(page_addr);
    uint32_t write_counter = UINT32_MAX;

    if (flash_supports_metadata())
    {
        CHIP_SELECT_OP()
        {
            flash_send_op_addr(opcode_read_cont_lowfreq, meta_data_addr);
            flash_spi_multi_io(&write_counter, sizeof(write_counter));
        }
    }

    return (UINT32_MAX == write_counter) ? 0 : write_counter;
}

void flash_chip_erase(void)
{
    unsigned char chip_erase[] = { 0xC7, 0x94, 0x80, 0x9A };

    CHIP_SELECT_OP()
    {
        flash_spi_multi_io(&chip_erase, sizeof(chip_erase));
    }
}
