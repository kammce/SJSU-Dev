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
 * @brief  Provides the chip programming info.
 *
 * This file is meant to have "magic" code so users cannot easily
 * manipulate the number of times the chip has been programmed.
 *
 * Though on the other hand, anyone with bootloader experience can
 * break-in, but probably not young students muahaha :D
 */
#ifndef CHIP_INFO_H__
#define CHIP_INFO_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <string.h> // memcpy()



/// @returns the magic location of the magical data - Do not use this function directly.
static inline const uint32_t* chip_get_magic_location(void) {   return (const uint32_t*) 0xF000;        }

/// @returns the number of times the chip has been programmed.
static inline uint32_t chip_get_prog_count(void)        {   return *(chip_get_magic_location() + 0);    }

/// @returns the number of times the chip was programmed a different program than the preceding program
static inline uint32_t chip_get_prog_modify_count(void) {   return *(chip_get_magic_location() + 1);    }

/// @returns the highest program size that was programmed
static inline uint32_t chip_get_prog_max(void)          {   return *(chip_get_magic_location() + 2);    }

/// @returns the lowest program size that was programmed
static inline uint32_t chip_get_prog_min(void)          {   return *(chip_get_magic_location() + 3);    }

/// @returns the number of milliseconds it took to program the last program.
static inline uint32_t chip_get_prog_time_ms(void)      {   return *(chip_get_magic_location() + 4);    }



/**
 * Program the flash memory from the given filename.
 * @param filename  The filename (must be less than 12 chars)
 */
static inline void chip_program_from_filename(const char* filename)
{
    // These magic constants and memory locations must match up with hyperload bootloader.
    const uint32_t maxChars = 12;               ///< maximum chars we can write (limited by magic memory space)
    const uint32_t signature = 0xDEADD00D;      ///< Signature to write to command register
    volatile uint32_t *pCommandReg = &(LPC_RTC->GPREG0); ///< Image program "command" is stored here
    volatile uint32_t *pFilename   = &(LPC_RTC->GPREG1); ///< Null terminated image name (up to 12 chars)

    // Do not use strcpy(), it will not work!
    memcpy((void*) pFilename, filename, maxChars);

    // Setting the signature is the "command" to program from the filename upon next boot.
    *pCommandReg = signature;
}



#ifdef __cplusplus
}
#endif
#endif /* CHIP_INFO_H__ */
