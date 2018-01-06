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
 * @brief This is the application entry point.
 */

#include <stdio.h>
#include "utilities.h"
#include "io.hpp"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

void ctoba(char * arr, char num) {
    arr[4] = 0;
    for(int i = 3; i >= 0; i--) {
        if(num & (8 >> i)) {
            arr[i] = '1';
        } else {
            arr[i] = '0';
        }
    }
}

int main (void)
{
    char buff[5];
    while(1)
    {
        for(int i = 0; i < 16; i++)
        {
            ctoba(buff, (char)i);
            for(int j = 1; j < 5; j++)
            {
                LE.set((5-j), CHECK_BIT(i,j-1));
            }
            LD.setNumber(i);
            printf("Hello World 0x%X\n", i);
            delay_ms(1000);
        }
    }
    return 0;
}
