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
 * @brief Provides bit-manipulation macros
 */
#ifndef BIT_MANIP_H__
#define BIT_MANIP_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>



/**
 * Bit manipulation macro.
 * Usage:
 * @code
 *      BIT(LPC_GPIO1->FIODIR).b5 = 1;
 *      BIT(my_var).b0 = 1;
 * @endcode
 */
#define BIT(reg)  (*((volatile bit_struct_t*)&(reg)))




/**
 * 1-bit structure
 * Use BIT() macro for bit manipulation of your memory/variable.
 */
typedef union
{
    // 32-bit structure below overlaps with this 32-bit integer
    // because each var of a union uses same base memory location.
    uint32_t full32bit;

    // : 1 (colon 1) means use only 1 bit size for the variable.
    struct {
        uint32_t b0  :1; uint32_t b1  :1; uint32_t b2  :1; uint32_t b3  :1;
        uint32_t b4  :1; uint32_t b5  :1; uint32_t b6  :1; uint32_t b7  :1;
        uint32_t b8  :1; uint32_t b9  :1; uint32_t b10 :1; uint32_t b11 :1;
        uint32_t b12 :1; uint32_t b13 :1; uint32_t b14 :1; uint32_t b15 :1;
        uint32_t b16 :1; uint32_t b17 :1; uint32_t b18 :1; uint32_t b19 :1;
        uint32_t b20 :1; uint32_t b21 :1; uint32_t b22 :1; uint32_t b23 :1;
        uint32_t b24 :1; uint32_t b25 :1; uint32_t b26 :1; uint32_t b27 :1;
        uint32_t b28 :1; uint32_t b29 :1; uint32_t b30 :1; uint32_t b31 :1;
    } __attribute__((packed));
    // packed means pack all 1 bit members tightly

    struct {
        uint32_t b1_0 : 2;   uint32_t b3_2 : 2;   uint32_t b5_4 : 2;   uint32_t b7_6 : 2;
        uint32_t b9_8 : 2;   uint32_t b11_10 : 2; uint32_t b13_12 : 2; uint32_t b15_14 : 2;
        uint32_t b17_16 : 2; uint32_t b19_18 : 2; uint32_t b21_20 : 2; uint32_t b23_22 : 2;
        uint32_t b25_24 : 2; uint32_t b27_26 : 2; uint32_t b29_28 : 2; uint32_t b31_30 : 2;
    } __attribute__((packed));
} bit_struct_t;



#ifdef __cplusplus
}
#endif
#endif /* BIT_MANIP_H__ */
