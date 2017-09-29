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



#include <stdio.h>
#include <stdlib.h> /* malloc() */
#include <string.h> /* strcmp() */
#include <inttypes.h>

#include "c_tlm_var.h"


/** Private function of this file */
static bool tlm_variable_check_dup(void *elm_ptr, void *arg1,
                                     void *arg2, void *arg3)
{
    tlm_reg_var_type *reg_var = elm_ptr;
    tlm_reg_var_type *new_var = arg1;

    return (reg_var->data_ptr != new_var->data_ptr &&
            0 != strcmp(reg_var->name, new_var->name)
            );
}

/** Private function of this file */
static bool tlm_component_find_callback(void *elm_ptr, void *arg1, void *arg2, void *arg3)
{
    tlm_reg_var_type *reg_var = elm_ptr;
    const char *name = arg1;

    /* If we locate the name, make copy of the pointer, and return false
     * to stop iterating further through the list
     */
    return (0 != strcmp(name, reg_var->name));
}



bool tlm_variable_register(tlm_component *comp_ptr,
                             const char *name,
                             const void *data_ptr,
                             const uint16_t data_size,
                             const uint16_t arr_size,
                             tlm_type type)
{
    if(NULL == comp_ptr || NULL == name || NULL == data_ptr || 0 == data_size) {
        return false;
    }

    tlm_reg_var_type *new_var = malloc(sizeof(tlm_reg_var_type));
    if(NULL == new_var) {
        return false;
    }

    /* If not an array, a single var still has size of 1 array element
     * This is make it easier to calculate bytes of the variable
     */
    new_var->name = name;
    new_var->data_ptr = data_ptr;
    new_var->elm_size_bytes = data_size;
    new_var->elm_arr_size = 0 == arr_size ? 1 : arr_size;
    new_var->elm_type = type;

    if (!c_list_for_each_elm(comp_ptr->var_list, tlm_variable_check_dup,
                             (void*)new_var, NULL, NULL)) {
        free(new_var);
        return false;
    }

    if (!c_list_insert_elm_end(comp_ptr->var_list, new_var)) {
        free(new_var);
        return false;
    }

    return true;
}

const tlm_reg_var_type* tlm_variable_get_by_name(tlm_component *comp_ptr, const char *name)
{
    tlm_reg_var_type *reg_var = NULL;
    if (NULL != comp_ptr && NULL != name && '\0' != *name) {
        reg_var = c_list_find_elm(comp_ptr->var_list, tlm_component_find_callback,
                                  (void*)name, NULL, NULL);
    }
    return reg_var;
}

const tlm_reg_var_type* tlm_variable_get_by_comp_and_name(const char *comp_name, const char *name)
{
    tlm_reg_var_type *reg_var = NULL;
    tlm_component *comp_ptr = tlm_component_get_by_name(comp_name);

    if (NULL != comp_ptr && NULL != name && '\0' != *name) {
        reg_var = c_list_find_elm(comp_ptr->var_list, tlm_component_find_callback,
                                  (void*)name, NULL, NULL);
    }

    return reg_var;
}

bool tlm_variable_set_value(const char *comp_name, const char *name, const char *value)
{
    const tlm_reg_var_type *reg_var = tlm_variable_get_by_comp_and_name(comp_name, name);
    if (NULL == reg_var) {
        return false;
    }

    void *dst = NULL;
    void *end = (uint8_t*) (reg_var->data_ptr) + (reg_var->elm_arr_size * reg_var->elm_size_bytes);

    #define tlm_variable_set_value_find_next_token(value) \
        while (0 != *value) {       \
            if (',' == *value) {    \
                ++value;            \
                break;              \
            } else {                \
                ++value;            \
        }   }

    bool success = false;
    switch(reg_var->elm_type) {
        case tlm_int:
        {
            long int temp = 0;
            dst = (void*)reg_var->data_ptr;
            while (dst < end && 1 == sscanf(value, "%li", &temp)) {
                memcpy(dst, &temp, reg_var->elm_size_bytes);
                success = true;
                tlm_variable_set_value_find_next_token(value);
                dst += reg_var->elm_size_bytes;
            }
            break;
        }
        case tlm_uint:
        {
            long unsigned temp = 0;
            dst = (void*)reg_var->data_ptr;
            while (dst < end && 1 == sscanf(value, "%lu", &temp)) {
                memcpy(dst, &temp, reg_var->elm_size_bytes);
                success = true;
                tlm_variable_set_value_find_next_token(value);
                dst += reg_var->elm_size_bytes;
            }
            break;
        }
        case tlm_char:
        {
            char temp = 0;
            dst = (void*)reg_var->data_ptr;
            while (dst < end && 1 == sscanf(value, "%c", &temp)) {
                memcpy(dst, &temp, reg_var->elm_size_bytes);
                success = true;
                tlm_variable_set_value_find_next_token(value);
                dst += reg_var->elm_size_bytes;
            }
            break;
        }

        case tlm_string:
            memset((void*)reg_var->data_ptr, 0, reg_var->elm_size_bytes);
            memcpy((void*)reg_var->data_ptr, value,
                    strlen(value) < reg_var->elm_size_bytes ? strlen(value) : reg_var->elm_size_bytes - 1);
            success = true;
            break;

        case tlm_bit_or_bool:
            dst = (void*)reg_var->data_ptr;
            while (dst < end) {
                success = true;
                memset((void*)reg_var->data_ptr, (value == strstr("true", value)), 1);
                tlm_variable_set_value_find_next_token(value);
                dst += reg_var->elm_size_bytes;
            };
            break;

        case tlm_binary:
            success = false;
            break;

        case tlm_float:
        {
            float temp = 0;
            dst = (void*)reg_var->data_ptr;
            while (dst < end && 1 == sscanf(value, "%f", &temp)) {
                memcpy(dst, &temp, reg_var->elm_size_bytes);
                success = true;
                tlm_variable_set_value_find_next_token(value);
                dst += reg_var->elm_size_bytes;
            }
            break;
        }

        case tlm_double:
        default:
            success = false;
            break;
    }

    return success;
}

bool tlm_variable_get_value(const char *comp_name, const char *name, char *buffer, int len)
{
    const tlm_reg_var_type *reg_var = tlm_variable_get_by_comp_and_name(comp_name, name);
    bool success = false;

    if (NULL != reg_var) {
        success = tlm_variable_print_value(reg_var, buffer, len);
    }

    return success;
}

bool tlm_variable_print_value(const tlm_reg_var_type *reg_var, char *buffer, int len)
{
    uint16_t i = 0;

    #define tlm_variable_print_array(format, var, buffer)           \
        for ( i=1; i < reg_var->elm_arr_size; i++) {                \
            ++var;                                                  \
            size_t curr_len = strlen(buffer);                       \
            snprintf(buffer+curr_len, len-curr_len, format, *data); \
        } do{ } while(0)

    bool success = false;
    switch(reg_var->elm_type) {
        case tlm_int:
        {
            success = true;
            if (1 == reg_var->elm_size_bytes) {
                int8_t *data = (int8_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "int8:%i", *data);
                tlm_variable_print_array(",%i", data, buffer);
            }
            else if (2 == reg_var->elm_size_bytes) {
                int16_t *data = (int16_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "int16:%i", *data);
                tlm_variable_print_array(",%i", data, buffer);
            }
            else if (4 == reg_var->elm_size_bytes) {
                int32_t *data = (int32_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "int32:%" PRIi32, *data);
                tlm_variable_print_array(",%"PRIi32, data, buffer);
            }
            else if (8 == reg_var->elm_size_bytes) {
                int64_t *data = (int64_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "int64:%"PRIi64, *data);
                tlm_variable_print_array(",%"PRIi64, data, buffer);
            }
            else {
                success = false;
            }
            break;
        }

        case tlm_uint:
        {
            success = true;
            if (1 == reg_var->elm_size_bytes) {
                uint8_t *data = (uint8_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "uint8:%u", *data);
                tlm_variable_print_array(",%u", data, buffer);
            }
            else if (2 == reg_var->elm_size_bytes) {
                uint16_t *data = (uint16_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "uint16:%u", *data);
                tlm_variable_print_array(",%u", data, buffer);
            }
            else if (4 == reg_var->elm_size_bytes) {
                uint32_t *data = (uint32_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "uint32:%"PRIu32, *data);
                tlm_variable_print_array(",%"PRIu32, data, buffer);
            }
            else if (8 == reg_var->elm_size_bytes) {
                uint64_t *data = (uint64_t*)(reg_var->data_ptr);
                snprintf(buffer, len, "uint64:%"PRIu64, *data);
                tlm_variable_print_array(",%"PRIu64, data, buffer);
            }
            else {
                success = false;
            }
            break;
        }

        case tlm_char:
        {
            char *data = (char*) (reg_var->data_ptr);
            snprintf(buffer, len, "char:%c", *data);
            tlm_variable_print_array(",%c", data, buffer);
            success = true;
            break;
        }

        case tlm_binary:
        {
            char *data = (char*) (reg_var->data_ptr);
            snprintf(buffer, len, "binary:%c", *data);
            tlm_variable_print_array("%c", data, buffer);
            success = true;
            break;
        }

        case tlm_string:
        {
            snprintf(buffer, len, "string:%s", (char*)(reg_var->data_ptr));
            success = true;
            break;
        }

        case tlm_bit_or_bool:
        {
            char *data = (char*) (reg_var->data_ptr);
            snprintf(buffer, len, "bool:%s", *data ? "true" : "false");
            for (i=1; i < reg_var->elm_arr_size; i++) {
                ++data;
                size_t curr_len = strlen(buffer);
                snprintf(buffer+curr_len, len-curr_len, ",%s", *data ? "true" : "false");
            }
            success = true;
            break;
        }

        case tlm_float:
        {
            float *data = (float*) (reg_var->data_ptr);
            snprintf(buffer, len, "float:%f", *data);
            tlm_variable_print_array(",%f", data, buffer);
            success = true;
            break;
        }

        case tlm_double:
        {
            double *data = (double*) (reg_var->data_ptr);
            snprintf(buffer, len, "double:%f", *data);
            tlm_variable_print_array(",%f", data, buffer);
            success = true;
            break;
        }

        default:
            success = false;
            break;
    }

    return success;
}
