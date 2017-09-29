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

#include "sys_config.h"
#include "LPC17xx.h"


#if 0
void sys_clock_use_fastest_clock (void)
{
    // 4Mhz * 2 * (499+1) / (7+1) / (4+1) = 100Mhz
    LPC_SC->CCLKCFG = 4;
    LPC_SC->CLKSRCSEL = 0;

    // M=499, N=7
    LPC_SC->PLL0CFG   = (499) | (7 << 16);
    LPC_SC->PLL0FEED  = 0xAA;
    LPC_SC->PLL0FEED  = 0x55;

    LPC_SC->PLL0CON   = 0x01;
    LPC_SC->PLL0FEED  = 0xAA;
    LPC_SC->PLL0FEED  = 0x55;
    while (!(LPC_SC->PLL0STAT & (1<<26)));

    LPC_SC->PLL0CON   = 0x03;
    LPC_SC->PLL0FEED  = 0xAA;
    LPC_SC->PLL0FEED  = 0x55;
    while (!(LPC_SC->PLL0STAT & ((1<<25) | (1<<24))));
}
#endif

/* If clock source is external crystal or internal IRC oscillator */
#if (CLOCK_SOURCE_INTERNAL == SYS_CFG_CLOCK_SOURCE || CLOCK_SOURCE_EXTERNAL == SYS_CFG_CLOCK_SOURCE)
/**
 * Calculates the PLL Parameters needed to set the desired CPU Speed
 * @note The parameters are in KiloHertz to avoid overflow of unsigned int (32-bit)
 * @param desiredCpuSpeedKhz  The desired CPU speed in KiloHertz
 * @param inputFreqKhz        The input clock to the PLL in KiloHertz
 * @param pPLL_M    The PLL "M" parameter that multiplies PLL input
 * @param pPLL_N    The PLL "N" Parameter that divides PLL output
 * @param pCPU_D    The CPU divider post the PLL output to feed to the CPU and peripheral clock
 * @post pPLL_M, pPLL_N, and pCPU_D will be set to the values that match or closely match desiredCpuSpeedKhz
 */
static bool sys_clock_configure_pll(const unsigned int desiredCpuSpeedKhz,
                                    const unsigned int inputFreqKhz,
                                    int* pPLL_M, int* pPLL_N, int* pCPU_D)
{
    *pPLL_M = 0;
    *pPLL_N = 0;
    *pCPU_D = 0;

    /**
     * This simple algorithm (not really optimized in any way), will scroll through
     * possible values of M, N, and D (See PLL Chapter), to find these values based
     * on user desired CPU Speed.
     *
     * Tests show values match faster with m decrementing than m incrementing
     */
    for (int m = 511; m >= 6; m--)
    {
        for (int n = 0; n < 32; n++)
        {
            const unsigned int FccoMinKhz = 275 * 1000;
            const unsigned int FccoMaxKhz = 550 * 1000;
            const unsigned int FccoKhz = (2 * (m + 1) * inputFreqKhz) / (n + 1);

            // If Fcco is in range, then check if we can generate desired CPU clock out of this
            if (FccoKhz >= FccoMinKhz && FccoKhz <= FccoMaxKhz)
            {
                // User Manual, Page 55/840 : Value of 0 and 1 is not allowed
                for (int cpudiv = 3; cpudiv < 256; cpudiv++)
                {
                    const unsigned int cpuClockKhz = FccoKhz / (cpudiv + 1);

                    /**
                     * Save the parameters if at least we got a speed that is acceptable by CPU
                     * Since cpudiv counts up, we will always get the slowest speed possible
                     */
                    const unsigned int maxCpuSpeedKhz = 100 * 1000;
                    if (cpuClockKhz <= maxCpuSpeedKhz)
                    {
                        *pPLL_M = m;
                        *pPLL_N = n;
                        *pCPU_D = cpudiv;

                        // If we absolutely got the speed we wanted, return from this function
                        if (cpuClockKhz == desiredCpuSpeedKhz)
                        {
                            // Break all the loops and return
                            return true;
                        }
                    }
                } // cpudiv for loop
            }
        } // N finder for loop
    } // M finder for loop

    return false;
}
#else /* If RTC clock is chosen */

#include <stdlib.h> // abs()
/**
 * This structure defines a table of MSEL, and NSEL of PLLCFG register that produce
 * the given Fcco value. These MSEL values are special values only used when RTC is
 * used as an input to the PLL.
 */
typedef struct {
    const uint16_t msel;    ///< Value of msel
    const uint16_t nsel;    ///< Value of nsel
    const uint32_t fcco_hz; ///< Based on msel and nsel, the Fcco value
} rtc_pll_table_t;

static const rtc_pll_table_t rtc_pll_vals[] = {
        {4272,  1, 279969800},
        {4395,  1, 288030700},
        {4578,  1, 300023800},
        {4725,  1, 309657600},
        {4807,  1, 315031600},
        {5127,  1, 336003100},
        {5188,  1, 340000800},
        {5400,  1, 353894400},
        {5493,  1, 359989200},
        {5859,  1, 383975400},
        {6042,  1, 395968500},
        {6075,  1, 398131200},
        {6104,  1, 400031700},
        {6409,  1, 420020200},
        {6592,  1, 432013300},
        {6750,  1, 442368000},
        {6836,  1, 448004100},
        {6866,  1, 449970200},
        {6958,  1, 455999500},
        {7050,  1, 462028800},
        {7324,  1, 479985700},
        {7425,  1, 486604800},
        {7690,  1, 503971800},
        {7813,  1, 512032800},
        {7935,  1, 520028200},
        {8057,  1, 528023600},
        {8100,  1, 530841600},
        {8545,  2, 280002600},
        {8789,  2, 287998000},
        {9155,  2, 299991000},
        {9613,  2, 314998800},
        {10254, 2, 336003100},
        {10376, 2, 340000800},
        {10986, 2, 359989200},
        {11719, 2, 384008200},
        {12085, 2, 396001300},
        {12207, 2, 399999000},
        {12817, 2, 419987500},
        {12817, 3, 279991600},
        {13184, 2, 432013300},
        {13184, 3, 288008900},
        {13672, 2, 448004100},
        {13733, 2, 450002900},
        {13733, 3, 300002000},
        {13916, 2, 455999500},
        {14099, 2, 461996000},
        {14420, 3, 315009700},
        {14648, 2, 479985700},
        {15381, 2, 504004600},
        {15381, 3, 336003100},
        {15564, 3, 340000800},
        {15625, 2, 512000000},
        {15869, 2, 519995400},
        {16113, 2, 527990800},
        {16479, 3, 359989200},
        {17578, 3, 383997300},
        {18127, 3, 395990400},
        {18311, 3, 400009900},
        {19226, 3, 419998400},
        {19775, 3, 431991500},
        {20508, 3, 448004100},
        {20599, 3, 449992000},
        {20874, 3, 455999500},
        {21149, 3, 462007000},
        {21973, 3, 480007500},
        {23071, 3, 503993700},
        {23438, 3, 512010900},
        {23804, 3, 520006300},
        {24170, 3, 528001700}
};

static void sys_clock_get_pll_params_for_rtc(const uint32_t desired_hz,
                                             int *m, int *n, int *d)
{
    const uint16_t table_size = sizeof(rtc_pll_vals) / sizeof(rtc_pll_vals[0]);
    // If CPU clock diff is below this difference, we'll pick it
    const uint32_t lowest_diff_hz = 1000;
    // If our diff reaches a value lower than this, we will skip nested for loop
    const int32_t skip_after_diff = -1 * 1000 * 1000;

    uint32_t closest = 0xffffffff;
    uint32_t diff_abs = 0;
    int32_t diff = 0;

    /* Outer loop to try every value of the special M/N table */
    for (uint16_t i = 0; i < table_size; i++)
    {
        /* Inner loop to try different values of the cclkdiv values */
        /* Minimum value is 3 otherwise CPU clock will be out of range */
        for (uint16_t cclkdiv = 3; cclkdiv <= 256; cclkdiv++) {
            diff = ((int32_t)(rtc_pll_vals[i].fcco_hz / cclkdiv) - desired_hz);

            /* The difference is only going to rise as we increase our clock divider, so break out
             * of here and try the next table entry rather than continuing on with more cclkdiv
             * values.  This small tweak can cut down on thousands of floating-point calculations.
             */
            if (diff < skip_after_diff) {
                break;
            }

            diff_abs = abs(diff);
            /* Get as close as possible */
            if (diff_abs < closest) {
                closest = diff_abs;

                /// Actual register values are written with (VAL - 1)
                *m = rtc_pll_vals[i].msel - 1;
                *n = rtc_pll_vals[i].nsel - 1;
                *d = cclkdiv - 1;

                /* If we are close enough, then return from function */
                if (diff_abs < lowest_diff_hz) {
                    return;
                }
            }
        }
    }
}
#endif

/**
 * Feed is used to make PLL register contents take effect
 */
static inline void sys_clock_pll0_feed()
{
    LPC_SC->PLL0FEED = 0xAA;
    LPC_SC->PLL0FEED = 0x55;
}

/**
 * Disables PLL0 (in case bootloader has it enabled)
 * and defaults to using internal oscillator
 */
static void sys_clock_disable_pll_use_internal_4mhz()
{
    LPC_SC->PLL0CON &= ~(1 << 1);
    sys_clock_pll0_feed();

    // Disable PLL0
    LPC_SC->PLL0CON &= ~(1 << 0);
    sys_clock_pll0_feed();

    LPC_SC->CLKSRCSEL = 0;
    LPC_SC->PLL0CFG = 0;    // M=1, N=1 (disregarded anyway after PLL is disabled)
    sys_clock_pll0_feed();
    LPC_SC->CCLKCFG = 0;    // Divider = 1 when PLL is not used
}

void sys_clock_configure()
{
    union {
        uint32_t raw;
        struct {
            uint32_t msel :16;
            uint32_t nsel :8;
            uint32_t : 8; ///< Not used (reserved)
        }__attribute__ ((packed));
    } PLL0ConfigValue = { 0 };

	int m = 0;
	int n = 0;
	int d = 0;

    /**
     * Disconnect PLL0 (in case bootloader uses PLL0)
     * Choose internal oscillator going forward (for now)
     */
	sys_clock_disable_pll_use_internal_4mhz();

#if (CLOCK_SOURCE_INTERNAL == SYS_CFG_CLOCK_SOURCE)
	const unsigned int PLLInputClockKhz = INTERNAL_CLOCK / 1000;
    unsigned int cpuClockKhz = SYS_CFG_DESIRED_CPU_CLK / 1000;
#elif (CLOCK_SOURCE_EXTERNAL == SYS_CFG_CLOCK_SOURCE)
	const unsigned int PLLInputClockKhz = EXTERNAL_CLOCK / 1000;
    unsigned int cpuClockKhz = SYS_CFG_DESIRED_CPU_CLK / 1000;
#elif (CLOCK_SOURCE_RTC == SYS_CFG_CLOCK_SOURCE)
    /* Nothing needed here since we pass in SYS_CFG_DESIRED_CPU_CLK in Hz to sys_clock_get_pll_params_for_rtc() */
#else
#error "Clock source must be CLOCK_SOURCE_INTERNAL, CLOCK_SOURCE_EXTERNAL or CLOCK_SOURCE_RTC"
#endif

#if (CLOCK_SOURCE_INTERNAL == SYS_CFG_CLOCK_SOURCE || CLOCK_SOURCE_EXTERNAL == SYS_CFG_CLOCK_SOURCE)
	// If we cannot calculate desired CPU clock, then default to a safe value
	if(!sys_clock_configure_pll(cpuClockKhz, PLLInputClockKhz, &m, &n, &d)) {
	    cpuClockKhz = SYS_CFG_DEFAULT_CPU_CLK / 1000;
	    sys_clock_configure_pll(cpuClockKhz, PLLInputClockKhz, &m, &n, &d);
	}
#else
	sys_clock_get_pll_params_for_rtc(SYS_CFG_DESIRED_CPU_CLK, &m, &n, &d);
#endif

	PLL0ConfigValue.msel = m;
	PLL0ConfigValue.nsel = n;

	// Enable main oscillator if needed :
#if (CLOCK_SOURCE_EXTERNAL == SYS_CFG_CLOCK_SOURCE)
	// Bit4 must be set if oscillator is between 15-25Mhz
    #if (EXTERNAL_CLOCK >= 15 * 1000 * 1000)
	    LPC_SC->SCS = (1 << 5) | (1 << 4);
    #else
        LPC_SC->SCS = (1 << 5); // Main Oscillator is enabled
    #endif

	while ((LPC_SC->SCS & (1 << 6)) == 0)
		; // Wait for main oscillator to be ready
#endif

	// Select the clock source and if the clock source is the desired clock, then
	// do not use PLL at all and simply return!
	LPC_SC->CLKSRCSEL = SYS_CFG_CLOCK_SOURCE;

	/* We can only do this if CLOCK input is not RTC since the user should
	 * always use PLL with RTC clock input
	 */
#if (CLOCK_SOURCE_INTERNAL == SYS_CFG_CLOCK_SOURCE || CLOCK_SOURCE_EXTERNAL == SYS_CFG_CLOCK_SOURCE)
	if(SYS_CFG_DESIRED_CPU_CLK == PLLInputClockKhz*1000) {
	    return;
	}
	else
#endif
	{
	    LPC_SC->PLL0CFG = PLL0ConfigValue.raw; // Set values of PLL Multiplier and divider
	    sys_clock_pll0_feed();

	    // Enable PLL0
	    LPC_SC->PLL0CON = 0x01;
	    sys_clock_pll0_feed();
	    while (!(LPC_SC->PLL0STAT & (1 << 26))) {
	        ; // Wait for PLL0 to lock
	    }

	    /**
	     * Connect PLL0 as our clock source.
	     * Right before we make the PLL clock as CPU clock, set our divider so our
	     * CPU clock doesn't go out of range once the faster PLL clock is established.
	     */
        LPC_SC->CCLKCFG  = d;
	    LPC_SC->PLL0CON = 0x03;
	    sys_clock_pll0_feed();

	    // Finally, wait for PLLC0_STAT & PLLE0_STAT
	    while (!(LPC_SC->PLL0STAT & ((1 << 25) | (1 << 24)))) {
	        ;
	    }
	}
}

unsigned int sys_get_cpu_clock()
{
	unsigned clock = 0;

	/* Determine clock frequency according to clock register values             */
	if (((LPC_SC->PLL0STAT >> 24) & 3) == 3)
	{ /* If PLL0 enabled and connected */
	    switch (LPC_SC->CLKSRCSEL & 0x03)
	    {
	        case 0: /* Int. RC oscillator => PLL0    */
	        case 3: /* Reserved, default to Int. RC  */
	            clock = (INTERNAL_CLOCK
	                    * ((2 * ((LPC_SC->PLL0STAT & 0x7FFF) + 1)))
	                    / (((LPC_SC->PLL0STAT >> 16) & 0xFF) + 1)
	                    / ((LPC_SC->CCLKCFG & 0xFF) + 1));
	            break;

	        case 1: /* Main oscillator => PLL0       */
	            clock = (EXTERNAL_CLOCK
	                    * ((2 * ((LPC_SC->PLL0STAT & 0x7FFF) + 1)))
	                    / (((LPC_SC->PLL0STAT >> 16) & 0xFF) + 1)
	                    / ((LPC_SC->CCLKCFG & 0xFF) + 1));
	            break;

	        case 2: /* RTC oscillator => PLL0        */
	            clock = (RTC_CLOCK
	                    * ((2 * ((LPC_SC->PLL0STAT & 0x7FFF) + 1)))
	                    / (((LPC_SC->PLL0STAT >> 16) & 0xFF) + 1)
	                    / ((LPC_SC->CCLKCFG & 0xFF) + 1));
	            break;
	    }
	}
	else
	{
	    switch (LPC_SC->CLKSRCSEL & 0x03)
	    {
	        case 0: /* Int. RC oscillator => PLL0    */
	        case 3: /* Reserved, default to Int. RC  */
	            clock = INTERNAL_CLOCK / ((LPC_SC->CCLKCFG & 0xFF) + 1);
	            break;
	        case 1: /* Main oscillator => PLL0       */
	            clock = EXTERNAL_CLOCK / ((LPC_SC->CCLKCFG & 0xFF) + 1);
	            break;
	        case 2: /* RTC oscillator => PLL0        */
	            clock = RTC_CLOCK / ((LPC_SC->CCLKCFG & 0xFF) + 1);
	            break;
	    }
	}

	return clock;
}
