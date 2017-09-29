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

#include <string.h>
#include <stdint.h>
#include "c_tlm_binary.h"
#include "c_tlm_var.h"


/**
 * Gets either the size of binary telemetry or the binary telemetry itself or both.
 * @param arg_size  Must be a valid uint32_t pointer.  This pointer is updated with
 *                  the size of the telemetry in raw bytes
 * @param binary    If null, only the size of telemetry will be obtained.
 *                  If non-null, the telemetry will be saved into this data pointer.
 */
static void get_tlm_one_comp(tlm_component *comp_ptr, void *arg_size, void *binary)
{
    void *hint = 0;
    tlm_reg_var_type *var = NULL;
    uint32_t *size = arg_size;
    uint32_t i = 0, sizeOfVar = 0;

    if (NULL != size && NULL != comp_ptr) {
        for(i=0; i < c_list_node_count(comp_ptr->var_list); i++) {
            var = c_list_get_elm_at(comp_ptr->var_list, i, &hint);
            if (NULL != var) {
                sizeOfVar = (var->elm_arr_size) * (var->elm_size_bytes);
                if (binary) {
                    memcpy(((char*)binary + (*size)), var->data_ptr, sizeOfVar);
                }
                (*size) += sizeOfVar;
            }
        }
    }
}

uint32_t tlm_binary_get_size_one(tlm_component *comp_ptr)
{
    uint32_t size = 0;
    get_tlm_one_comp(comp_ptr, &size, NULL);
    return size;
}
uint32_t tlm_binary_get_size_all(void)
{
    uint32_t size = 0;
    tlm_component_for_each((tlm_comp_callback)get_tlm_one_comp, &size, NULL);
    return size;
}

uint32_t tlm_binary_get_one(tlm_component *comp_ptr, char *binary)
{
    uint32_t offset = 0;
    get_tlm_one_comp(comp_ptr, &offset, binary);
    return offset;
}
uint32_t tlm_binary_get_all(char *binary)
{
    uint32_t offset = 0;
    tlm_component_for_each((tlm_comp_callback)get_tlm_one_comp, &offset, binary);
    return offset;
}

/**
 * Compare telemetry
 * @param binary      The binary telemetry to compare
 * @param offset_arg  When non-zero, then telemetry is the same as binary
 */
static void cmp_tlm_one_comp(tlm_component *comp_ptr, void *binary, void *offset_arg)
{
    void *hint = 0;
    tlm_reg_var_type *var = NULL;
    uint32_t size = 0, i = 0;
    uint32_t *offset = offset_arg;

    if (NULL != comp_ptr) {
        for(i=0; i < c_list_node_count(comp_ptr->var_list); i++) {
            var = c_list_get_elm_at(comp_ptr->var_list, i, &hint);
            if (NULL != var) {
                size = (var->elm_arr_size) * (var->elm_size_bytes);
                if (0 != memcmp(((char*)binary + (*offset)), var->data_ptr, size)) {
                    *offset = 0;
                    break;
                }
                else {
                    *offset += size;
                }
            }
        }
    }
}

bool tlm_binary_compare_one(tlm_component *comp_ptr, char *binary)
{
    uint32_t offset = 0;
    cmp_tlm_one_comp(comp_ptr, binary, &offset);
    return (0 != offset);
}

bool tlm_binary_compare_all(char *binary)
{
    uint32_t offset = 0;
    tlm_component_for_each((tlm_comp_callback)cmp_tlm_one_comp, binary, &offset);
    return (0 != offset);
}
