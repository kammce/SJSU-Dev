#include "sd.h"
#include "disk_defines.h"
#include "lpc_sys.h"

/* Definitions for MMC/SDC command */
#define CMD0            (0x40+0)        /* GO_IDLE_STATE */
#define CMD1            (0x40+1)        /* SEND_OP_COND (MMC) */
#define ACMD41          (0xC0+41)       /* SEND_OP_COND (SDC) */
#define CMD8            (0x40+8)        /* SEND_IF_COND */
#define CMD9            (0x40+9)        /* SEND_CSD */
#define CMD10           (0x40+10)       /* SEND_CID */
#define CMD12           (0x40+12)       /* STOP_TRANSMISSION */
#define ACMD13          (0xC0+13)       /* SD_STATUS (SDC) */
#define CMD16           (0x40+16)       /* SET_BLOCKLEN */
#define CMD17           (0x40+17)       /* READ_SINGLE_BLOCK */
#define CMD18           (0x40+18)       /* READ_MULTIPLE_BLOCK */
#define CMD23           (0x40+23)       /* SET_BLOCK_COUNT (MMC) */
#define ACMD23          (0xC0+23)       /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24           (0x40+24)       /* WRITE_BLOCK */
#define CMD25           (0x40+25)       /* WRITE_MULTIPLE_BLOCK */
#define CMD55           (0x40+55)       /* APP_CMD */
#define CMD58           (0x40+58)       /* READ_OCR */

/* MMC/SDC command */
#define MMC_GET_TYPE        10
#define MMC_GET_CSD         11
#define MMC_GET_CID         12
#define MMC_GET_OCR         13
#define MMC_GET_SDSTAT      14
/* ATA/CF command */
#define ATA_GET_REV         20
#define ATA_GET_MODEL       21
#define ATA_GET_SN          22

/* Card type flags (CardType) */
#define CT_MMC              0x01
#define CT_SD1              0x02
#define CT_SD2              0x04
#define CT_SDC              (CT_SD1|CT_SD2)
#define CT_BLOCK            0x08

#define DEBUG_SD_CARD		    0	// Set to 1 to printf debug data.
#define OPTIMIZE_SSP_SPI_WRITE	1	// Set to 1 and fill in Optimized code yourself!
#define OPTIMIZE_SSP_SPI_READ   1   // Uses better SPI function for transfer

static volatile DSTATUS g_disk_status = STA_NOINIT; /**< Disk status */
static BYTE g_card_type; /**< Card type flags */

static inline char get_spi(void)
{
    return SD_SELECT();
}
static inline char release_spi(void)
{
    return SD_DESELECT();
}

BYTE wait_ready(void)
{
    BYTE res;

    /* Wait for ready in timeout of 500ms */
    UINT timeout = sys_get_uptime_ms() + 500;
    rcvr_spi();

    do
    {
        res = rcvr_spi();
    } while ((res != 0xFF) && sys_get_uptime_ms() < timeout);

    return res;
}

void power_on(void)
{
    // Power on the SD-Card Socket if hardware allows
}

void power_off(void)
{
    if (!get_spi())
        return;

    wait_ready();
    release_spi();

    // Power off the SD-Card Socket if hardware allows
    g_disk_status |= STA_NOINIT; // Set STA_NOINIT
}

int rcvr_datablock(BYTE *buff, /* Data buffer to store received data */
UINT btr /* Byte count (must be multiple of 4) */
)
{
    BYTE token;
    UINT timeout = sys_get_uptime_ms() + 100;
    do
    { /* Wait for data packet in timeout of 100ms */
        token = rcvr_spi();
    } while ((token == 0xFF) && sys_get_uptime_ms() < timeout);

    if (token != 0xFE)
        return 0; /* If not valid data token, return with error */

    /**
     * If it's worth doing DMA, then do it:
     */
    if (OPTIMIZE_SSP_SPI_READ && btr > 16)
    {
        ssp1_dma_transfer_block(buff, 512, 0);
        buff += 512;
    }
    else
    {
        do
        {
            *buff++ = rcvr_spi();
            *buff++ = rcvr_spi();
            *buff++ = rcvr_spi();
            *buff++ = rcvr_spi();
        } while (btr -= 4);
    }

    rcvr_spi(); /* Discard CRC */
    rcvr_spi();

    return 1; /* Return with success */
}

#if _READONLY == 0
int xmit_datablock(const BYTE *buff, /* 512 byte data block to be transmitted */
BYTE token /* Data/Stop token */
)
{
    BYTE resp;

    if (wait_ready() != 0xFF)
        return 0;

    xmit_spi(token);
    /* Xmit data token */
    if (token != 0xFD)
    { /* Is data token */
#if OPTIMIZE_SSP_SPI_WRITE
        ssp1_dma_transfer_block((unsigned char*) buff, 512, 0xff);
#else
        unsigned char wc = 0;
        do
        { /* Xmit the 512 byte data block to MMC */
            xmit_spi(*buff++);
            xmit_spi(*buff++);
        }while (--wc);
#endif
        xmit_spi(0xFF);
        /* CRC (Dummy) */
        xmit_spi(0xFF);
        resp = rcvr_spi(); /* Reveive data response */
        if ((resp & 0x1F) != 0x05) /* If not accepted, return with error */
            return 0;
    }

    return 1;
}
#endif /* _READONLY */

BYTE send_cmd(BYTE cmd, /* Command byte */
DWORD arg /* Argument */
)
{
    BYTE n, res;

    if (cmd & 0x80)
    { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1)
            return res;
    }

    /* Select the card and wait for ready */
    SD_DESELECT();
    SD_SELECT(); // Wait for card ready

    if (wait_ready() != 0xFF)
    {
#if(DEBUG_SD_CARD)
        rprintf("send_cmd: Error, wait_ready() did not return 0xFF\n");
#endif
        return 0xFF;
    }

    /* Send command packet */xmit_spi(cmd);
    /* Start + Command index */
    xmit_spi((BYTE)(arg >> 24));
    /* Argument[31..24] */
    xmit_spi((BYTE)(arg >> 16));
    /* Argument[23..16] */
    xmit_spi((BYTE)(arg >> 8));
    /* Argument[15..8] */
    xmit_spi((BYTE)arg);
    /* Argument[7..0] */
    n = 0x01; /* Dummy CRC + Stop */
    if (cmd == CMD0)
        n = 0x95; /* Valid CRC for CMD0(0) */
    if (cmd == CMD8)
        n = 0x87; /* Valid CRC for CMD8(0x1AA) */
    xmit_spi(n);

    /* Receive command response */
    if (cmd == CMD12)
        rcvr_spi(); /* Skip a stuff byte when stop reading */
    n = 10; /* Wait for a valid response in timeout of 10 attempts */

    do
    {
        res = rcvr_spi();
    } while ((res & 0x80) && --n);

#if(DEBUG_SD_CARD)
    if (n == 0) rprintf("send_cmd: Timeout during card read\n");
#endif

    return res; /* Return with the response value */
}

DSTATUS sd_initialize()
{
    BYTE n, cmd, ty, ocr[4];

    sd_update_card_status();

    if (g_disk_status & STA_NODISK)
    {
        return g_disk_status; /* No card in the socket */
    }

    power_on(); /* Force socket power on */
    FCLK_SLOW();
    for (n = 10; n; n--)
        rcvr_spi(); /* 80 dummy clocks */

    if (!get_spi())
        return RES_ERROR;
    ty = 0;
    if (send_cmd(CMD0, 0) == 1)
    { /* Enter Idle state */

        UINT timeout = sys_get_uptime_ms() + 1000;
        if (send_cmd(CMD8, 0x1AA) == 1)
        { /* SDHC */
#if(DEBUG_SD_CARD)
            rprintf("sd_initialize: CMD8 succeeded...\n");
#endif

            for (n = 0; n < 4; n++)
                ocr[n] = rcvr_spi(); /* Get trailing return value of R7 resp */
            if (ocr[2] == 0x01 && ocr[3] == 0xAA)
            { /* The card can work at vdd range of 2.7-3.6V */
#if(DEBUG_SD_CARD)
                rprintf("sd_initialize: SD-HC Card detected!\n");
#endif

                while (sys_get_uptime_ms() < timeout
                        && send_cmd(ACMD41, 1UL << 30))
                    ; /* Wait for leaving idle state (ACMD41 with HCS bit) */

                if (sys_get_uptime_ms() < timeout && send_cmd(CMD58, 0) == 0)
                { /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++)
                        ocr[n] = rcvr_spi();
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
                else
                {
#if(DEBUG_SD_CARD)
                    rprintf("sd_initialize: CMD58 FAILED!\n");
#endif
                }
            }
        }
        else
        { /* SDSC or MMC */
#if(DEBUG_SD_CARD)
            rprintf("sd_initialize: Card is not SD-HC!\n");
#endif
            if (send_cmd(ACMD41, 0) <= 1)
            {
                ty = CT_SD1;
                cmd = ACMD41; /* SDSC */
            }
            else
            {
                ty = CT_MMC;
                cmd = CMD1; /* MMC */
            }
            while (sys_get_uptime_ms() < timeout && send_cmd(cmd, 0))
                ; /* Wait for leaving idle state */

            if ((sys_get_uptime_ms() < timeout) || send_cmd(CMD16, 512) != 0) /* Set R/W block length to 512 */
            {
#if(DEBUG_SD_CARD)
                rprintf("sd_initialize: Could not set block length to 512\n");
                ty = 0;
#endif
            }
        }
    }
    else
    {
#if(DEBUG_SD_CARD)
        rprintf("sd_initialize: CMD0 did not respond...\n");
#endif
    }
    g_card_type = ty;
    release_spi();

    if (ty)
    { /* Initialization succeded */
        g_disk_status &= ~STA_NOINIT; /* Clear STA_NOINIT */
        FCLK_FAST();
    }
    else
    { /* Initialization failed */
        power_off();
    }

    return g_disk_status;
}

DSTATUS sd_status()
{
    sd_update_card_status();
    return g_disk_status;
}

DRESULT sd_read(BYTE *buff, /* Pointer to the data buffer to store read data */
DWORD sector, /* Start sector number (LBA) */
BYTE count /* Sector count (1..255) */
)
{
    sd_update_card_status();

    if (!count)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;
    if (!get_spi())
        return RES_ERROR;

    if (!(g_card_type & CT_BLOCK))
        sector *= 512; /* Convert to byte address if needed */

    if (count == 1)
    { /* Single block read */
        if ((send_cmd(CMD17, sector) == 0) /* READ_SINGLE_BLOCK */
        && rcvr_datablock(buff, 512))
            count = 0;
    }
    else
    { /* Multiple block read */
        if (send_cmd(CMD18, sector) == 0)
        { /* READ_MULTIPLE_BLOCK */
            do
            {
                if (!rcvr_datablock(buff, 512))
                    break;
                buff += 512;
            } while (--count);
            send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
        }
    }
    release_spi();

    return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _READONLY == 0
DRESULT sd_write(const BYTE *buff, /* Pointer to the data to be written */
DWORD sector, /* Start sector number (LBA) */
BYTE count /* Sector count (1..255) */
)
{
    sd_update_card_status();

    if (!count)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;
    if (g_disk_status & STA_PROTECT)
        return RES_WRPRT;
    if (!get_spi())
        return RES_ERROR;

    if (!(g_card_type & CT_BLOCK))
        sector *= 512; /* Convert to byte address if needed */

    if (count == 1)
    { /* Single block write */
        if ((send_cmd(CMD24, sector) == 0) && xmit_datablock(buff, 0xFE))
            count = 0;
    }
    else
    {
        if (g_card_type & CT_SDC)
            send_cmd(ACMD23, count);
        if (send_cmd(CMD25, sector) == 0)
        { /* WRITE_MULTIPLE_BLOCK */
            do
            {
                if (!xmit_datablock(buff, 0xFC))
                    break;
                buff += 512;
            } while (--count);
            if (!xmit_datablock(0, 0xFD)) /* STOP_TRAN token */
                count = 1;
        }
    }
    release_spi();

    return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY == 0 */

#if _USE_IOCTL != 0
DRESULT sd_ioctl(BYTE ctrl, /* Control code */
void *buff /* Buffer to send/receive control data */
)
{
    DRESULT res;
    BYTE n, csd[16], *ptr = (BYTE*) buff;
    WORD csize;

    sd_update_card_status();
    res = RES_ERROR;

    if (ctrl == CTRL_POWER)
    {
        switch (*ptr)
        {
            case 0: /* Sub control code == 0 (POWER_OFF) */
                if (SD_PRESENT())
                    power_off(); /* Power off */
                res = RES_OK;
                break;
            case 1: /* Sub control code == 1 (POWER_ON) */
                power_on(); /* Power on */
                res = RES_OK;
                break;
            case 2: /* Sub control code == 2 (POWER_GET) */
                *(ptr + 1) = (BYTE) SD_PRESENT();
                res = RES_OK;
                break;
            default:
                res = RES_PARERR;
                break;
        }
    }
    else
    {
        if (g_disk_status & STA_NOINIT)
            return RES_NOTRDY;

        switch (ctrl)
        {
            case CTRL_SYNC: /* Make sure that no pending write process */
                //SELECT();			// Wait for card ready
                if (!get_spi())
                    return RES_ERROR;
                if (wait_ready() == 0xFF)
                    res = RES_OK;
                break;

            case GET_SECTOR_COUNT: /* Get number of sectors on the disk (DWORD) */
                if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16))
                {
                    if ((csd[0] >> 6) == 1)
                    { /* SDC ver 2.00 */
                        csize = csd[9] + ((WORD) csd[8] << 8) + 1;
                        *(DWORD*) buff = (DWORD) csize << 10;
                    }
                    else
                    { /* SDC ver 1.XX or MMC*/
                        n = (csd[5] & 15) + ((csd[10] & 128) >> 7)
                                + ((csd[9] & 3) << 1) + 2;
                        csize = (csd[8] >> 6) + ((WORD) csd[7] << 2)
                                + ((WORD) (csd[6] & 3) << 10) + 1;
                        *(DWORD*) buff = (DWORD) csize << (n - 9);
                    }
                    res = RES_OK;
                }
                break;

            case GET_SECTOR_SIZE: /* Get R/W sector size (WORD) */
                *(WORD*) buff = 512;
                res = RES_OK;
                break;

            case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
                if (g_card_type & CT_SD2)
                { /* SDC ver 2.00 */
                    if (send_cmd(ACMD13, 0) == 0)
                    { /* Read SD status */
                        rcvr_spi();
                        if (rcvr_datablock(csd, 16))
                        { /* Read partial block */
                            for (n = 64 - 16; n; n--)
                                rcvr_spi(); /* Purge trailing data */
                            *(DWORD*) buff = 16UL << (csd[10] >> 4);
                            res = RES_OK;
                        }
                    }
                }
                else
                { /* SDC ver 1.XX or MMC */
                    if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16))
                    { /* Read CSD */
                        if (g_card_type & CT_SD1)
                        { /* SDC ver 1.XX */
                            *(DWORD*) buff = (((csd[10] & 63) << 1)
                                    + ((WORD) (csd[11] & 128) >> 7) + 1)
                                    << ((csd[13] >> 6) - 1);
                        }
                        else
                        { /* MMC */
                            *(DWORD*) buff = ((WORD) ((csd[10] & 124) >> 2) + 1)
                                    * (((csd[11] & 3) << 3)
                                            + ((csd[11] & 224) >> 5) + 1);
                        }
                        res = RES_OK;
                    }
                }
                break;

            case MMC_GET_TYPE: /* Get card type flags (1 byte) */
                *ptr = g_card_type;
                res = RES_OK;
                break;

            case MMC_GET_CSD: /* Receive CSD as a data block (16 bytes) */
                if (send_cmd(CMD9, 0) == 0 /* READ_CSD */
                && rcvr_datablock(ptr, 16))
                    res = RES_OK;
                break;

            case MMC_GET_CID: /* Receive CID as a data block (16 bytes) */
                if (send_cmd(CMD10, 0) == 0 /* READ_CID */
                && rcvr_datablock(ptr, 16))
                    res = RES_OK;
                break;

            case MMC_GET_OCR: /* Receive OCR as an R3 resp (4 bytes) */
                if (send_cmd(CMD58, 0) == 0)
                { /* READ_OCR */
                    for (n = 4; n; n--)
                        *ptr++ = rcvr_spi();
                    res = RES_OK;
                }
                break;

            case MMC_GET_SDSTAT: /* Receive SD status as a data block (64 bytes) */
                if (send_cmd(ACMD13, 0) == 0)
                { /* SD_STATUS */
                    rcvr_spi();
                    if (rcvr_datablock(ptr, 64))
                        res = RES_OK;
                }
                break;

            default:
                res = RES_PARERR;
                break;
        }

        release_spi();
    }

    return res;
}
#endif /* _USE_IOCTL != 0 */

void sd_update_card_status(void)
{
    BYTE s = g_disk_status;

    if (SD_PRESENT()) {
        s &= ~STA_NODISK;
    }
    else {
        s |= (STA_NODISK | STA_NOINIT);
    }

    g_disk_status = s;
}
