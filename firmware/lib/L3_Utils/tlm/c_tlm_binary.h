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

#ifndef C_TLM_BINARY_H__
#define C_TLM_BINARY_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "c_tlm_comp.h"



/**
 * @file
 * This file allows to get telemetry data in binary format.  This only provides
 * the raw, contiguous data, not the data type, size or name etc.
 *
 * Applications of this API :
 *  - Provide raw contiguous data
 *  - Provide means to compare if telemetry data has changed.
 *
 * @warning  This API should only be used if no additional telemetry registration
 *           will take place.
 * @code
 *      int size = tlm_binary_get_size_one(my_comp);
 *      char *binary = (char*)malloc(size);
 *      tlm_binary_get_one(my_comp, binary));
 *
 *      if(!tlm_binary_compare_one(my_comp, binary)) {
 *          // Data changed, do something
 *      }
 * @endcode
 */


/**
 * @{
 * Get the size of the binary data which should be used to allocate
 * memory before calling tlm_binary_get_xxx() functions.
 * @param comp_ptr  The component pointer
 */
uint32_t tlm_binary_get_size_one(tlm_component *comp_ptr);
uint32_t tlm_binary_get_size_all(void);
/** @} */


/**
 * @{
 * Copy the telemetry data to the provided memory pointer
 * @param comp_ptr   The component pointer
 * @param binary     The data pointer to which data will be copied
 * @returns The number of bytes copied into the data pointer.
 */
uint32_t tlm_binary_get_one(tlm_component *comp_ptr, char *binary);
uint32_t tlm_binary_get_all(char *binary);
/** @} */


/**
 * @{
 * Compare the binary data with the latest telemetry's binary data
 * @param comp_ptr   The component pointer
 * @param binary     The binary data at which to compare
 * @returns true if the binary data is the same as the latest telemetry data
 */
bool tlm_binary_compare_one(tlm_component *comp_ptr, char *binary);
bool tlm_binary_compare_all(char *binary);
/** @} */



#ifdef __cplusplus
}
#endif
#endif /* C_TLM_BINARY_H__ */
