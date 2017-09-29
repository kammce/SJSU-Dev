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

#include <stdlib.h>
#include <malloc.h>
#include <stddef.h>

#include "lpc_sys.h"



/**
 * malloc() and other memory allocations functions go here to get memory from heap.
 * This is used by printf and other C/C++ functions.
 * _pvHeapStart is defined by the compiler after used RAM space.
 *
 * @warning There is no check for heap memory going beyond addressable RAM region
 */
static void *g_last_sbrk_ptr = 0;           ///< The last pointer given by _sbrk()
static char *g_next_heap_ptr = 0;           ///< The next pointer _sbrk() may provide
static unsigned int g_sbrk_calls = 0;       ///< Number of calls to _sbrk()
static unsigned int g_last_sbrk_size = 0;   ///< The last size requested from _sbrk()

/**
 * @{ RAM constants
 * ram_region_1_end and ram_region_2_end is not inclusive of the end.
 */
static const unsigned int one_sram_block_size = 32 * 1024;
static const char * const ram_region_1_base = (char*)0x10000000;
static const char * const ram_region_1_end  = ram_region_1_base + one_sram_block_size;
static const char * const ram_region_2_base = (char*)0x2007C000;
static const char * const ram_region_2_end  = ram_region_2_base + one_sram_block_size;
/** @} */



extern "C" void * _sbrk(size_t req_bytes)
{
    char  *ret_mem = 0;

    /* Initialize Heap pointer to bottom of RAM region 1 */
    if (!g_next_heap_ptr) {
        g_next_heap_ptr = (char*) ram_region_1_base;
    }

    ret_mem = g_next_heap_ptr;    /* Save the pointer we will return */
    g_next_heap_ptr += req_bytes; /* Increase our pointer */

    /**
     * Check if we gave out too much memory beyond the ram_region_1.
     * We have more RAM in ram region #2 but some of it is used by global variable space.
     *
     * @note
     * If next heap pointer is exactly at ram_region_1_end, then we've got perfectly aligned
     * ret_mem to the end of SRAM1. If it is greater, then we've been asked for too much so try
     * to return from SRAM2.
     */
    if ((unsigned)g_next_heap_ptr > ((unsigned)ram_region_1_end) &&
        (unsigned)g_next_heap_ptr < (unsigned)ram_region_2_base
    ) {
        /* Defined by linker.  This is location after global memory space */
        extern char _pvHeapStart[];
        g_next_heap_ptr = _pvHeapStart;

        ret_mem = g_next_heap_ptr;
        g_next_heap_ptr += req_bytes;
    }

    /* Did we exhaust the 2nd ram region too ? */
    if (g_next_heap_ptr >= ram_region_2_end) {
        g_next_heap_ptr = (char*) ram_region_2_end;
        ret_mem = 0;
    }

    /* Seems like newlib is calling us twice, one for real allocation and once more
     * with req_bytes set to zero, so do not increment counters for this case
     */
    if (req_bytes > 0) {
        ++g_sbrk_calls;
        g_last_sbrk_ptr = ret_mem;
        g_last_sbrk_size = req_bytes;
    }

    return ret_mem;        /*  Return pointer to start of new heap area.   */
}

/** @{ Redirect C++ memory functions to C */
void *operator new(size_t size)     {   return malloc(size);    }
void *operator new[](size_t size)   {   return malloc(size);    }
void operator delete(void *p)       {   free(p);                }
void operator delete[](void *p)     {   free(p);                }
/** @} */

extern "C" sys_mem_t sys_get_mem_info()
{
    sys_mem_t meminfo;

    // This is defined by linker script (loader.ld)
    extern unsigned int _pvHeapStart;

    // Heap pointer starts after global memory in SRAM2
    const unsigned int globalMem = (unsigned int) &_pvHeapStart - (unsigned int)ram_region_2_base;

    // Only print malloc() info if it has been used (arena is > 0)
    struct mallinfo info = mallinfo();

    meminfo.used_global = globalMem;
    meminfo.avail_heap = info.fordblks;
    meminfo.used_heap = info.uordblks;

    /* HACK: 20131214 (GCC 4.7.4)
     *  Current version newlib nano returns zero for all mallinfo struct members.
     *  So this is a hack to deduce the used heap.
     */
    if (0 == meminfo.used_heap) {
        if ((unsigned) g_next_heap_ptr <= (unsigned)ram_region_1_end) {
            meminfo.used_heap = (g_next_heap_ptr - ram_region_1_base);
        }
        else {
            meminfo.used_heap = one_sram_block_size + (g_next_heap_ptr - ram_region_2_base);
        }
    }

    /* HACK : Fix to malloc's memory reporting once we switch to RAM 2
     * It looks like mallinfo() doesn't like when we return memory with a gap from sbrk()
     */
    if (meminfo.used_heap > (2 * one_sram_block_size)) {
        // meminfo.used_heap -= (0x1007C000 - one_sram_block_size);
		meminfo.used_heap -= ( (ram_region_2_base - ram_region_1_base) - one_sram_block_size);
    }

    /* If next heap is still in SRAM1 then we have got all of SRAM2 and the remaining of the SRAM1
     * If next heap pointer is in SRAM2, then we've got whatever is left in SRAM 2
     */
    meminfo.avail_sys = 0;
    if ((unsigned) g_next_heap_ptr <= (unsigned)ram_region_1_end) {
        meminfo.avail_sys = one_sram_block_size + (unsigned) (ram_region_1_end - g_next_heap_ptr) - globalMem;
    }
    else if ((unsigned)g_next_heap_ptr < (unsigned)ram_region_2_end){
        /* If RAM runs out, subtracting globalMem will make it negative, so cover for that */
        int avail = (int) (ram_region_2_end - g_next_heap_ptr) - globalMem;
        if (avail > 0) {
            meminfo.avail_sys = avail;
        }
    }

    meminfo.next_malloc_ptr = g_next_heap_ptr;
    meminfo.last_sbrk_ptr   = g_last_sbrk_ptr;
    meminfo.last_sbrk_size  = g_last_sbrk_size;
    meminfo.num_sbrk_calls  = g_sbrk_calls;

    return meminfo;
}

extern "C" int __aeabi_atexit(void *object,
		void (*destructor)(void *),
		void *dso_handle)
{
	return 0;
}


#ifndef CPP_USE_CPPLIBRARY_TERMINATE_HANDLER
/******************************************************************
 * __verbose_terminate_handler()
 *
 * This is the function that is called when an uncaught C++
 * exception is encountered. The default version within the C++
 * library prints the name of the uncaught exception, but to do so
 * it must demangle its name - which causes a large amount of code
 * to be pulled in. The below minimal implementation can reduce
 * code size noticeably. Note that this function should not return.
 ******************************************************************/
namespace __gnu_cxx {
void __verbose_terminate_handler()
{
  while(1);
}
}
#endif
