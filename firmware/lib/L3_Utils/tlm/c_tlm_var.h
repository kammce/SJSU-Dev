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

#ifndef C_TLM_VAR_H__
#define C_TLM_VAR_H__
#include "c_list.h"
#include "c_tlm_comp.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * This file allows variables to be registered under a component.
 * @see Sample code of c_tlm_comp.h
 */

/**
 * The type of the variable.  This can be used by a telemetry
 * decoder to print-out the data in human readable format.
 */
typedef enum {
    tlm_undefined   = 0,
    tlm_int         = 1,
    tlm_uint        = 2,
    tlm_char        = 3,
    tlm_float       = 4,
    tlm_double      = 5,
    tlm_string      = 6,
    tlm_binary      = 7,
    tlm_bit_or_bool = 8,
} tlm_type ;

/**
 * Structure of a single variable registration
 */
typedef struct {
    const char *name;      /**< Name of the variable */
    const void *data_ptr;  /**< Data pointer of the variable */

    uint32_t elm_size_bytes; /**< Size of the variable in bytes */
    uint32_t elm_arr_size;   /**< If an array, the size of the array */
    tlm_type elm_type;       /**< The type of the element */
} tlm_reg_var_type;


/**
 * Adds a variable to a component.
 * @param comp_ptr  The component pointer
 * @param name      The name of the variable
 * @param data_ptr  The data pointer of the variable
 * @param data_size The size of the variable
 * @param arr_size  If not an array, use zero.  If registering an array, use
 *                  the number of elements in the array.
 * @param type      The type of the variable.
 *
 * @returns true upon success.  If memory allocation fails, or if another variable
 *          is registered by the same name or same memory pointer, false is returned.
 */
bool tlm_variable_register(tlm_component *comp_ptr,
                             const char *name,
                             const void *data_ptr,
                             const uint16_t data_size,
                             const uint16_t arr_size,
                             tlm_type type);

/**
 * Macro to register a variable.
 * If a variable is called "var", then this macro will yield :
 *  tlm_variable_register(comp, "var", &var, sizeof(var), 1);
 */
#define TLM_REG_VAR(comp, var, type) \
    tlm_variable_register(comp, #var, &var, sizeof(var), 1, type)

/**
 * Macro to register an array.
 */
#define TLM_REG_ARR(comp, var, type) \
    tlm_variable_register(comp, #var, &var[0], sizeof(var[0]), sizeof(var)/sizeof(var[0]), type)

/**
 * Get the data pointer and the size of a previously registered variable.
 * The tlm_reg_var_type structure contains the pointer and the size.
 * @param comp_ptr   The component pointer that contains the variable
 * @param name       The registered name of the variable
 */
const tlm_reg_var_type* tlm_variable_get_by_name(tlm_component *comp_ptr,
                                                 const char *name);

/**
 * Get the data pointer and the size of a previously registered variable.
 * The tlm_reg_var_type structure contains the pointer and the size.
 * @param comp_name   The component name that contains the variable
 * @param name        The registered name of the variable
 */
const tlm_reg_var_type* tlm_variable_get_by_comp_and_name(const char *comp_name,
                                                          const char *name);

/**
 * Sets a value to one of the telemetry variables.  This is sort of a back-door way to force
 * a value to the telemetry variable.
 * @param comp_name     The name of the component (this will be located by name)
 * @param name          The name of the registered variable
 * @param value         The string value.  For example, to set an integer, just pass string value
 *                      such as "123".  To set boolean value, use "true" or "false"
 */
bool tlm_variable_set_value(const char *comp_name, const char *name, const char *value);

/**
 * Gets a value to one of the telemetry variables
 * @param comp_name     The name of the component (this will be located by name)
 * @param name          The name of the registered variable
 * @param buffer        The buffer at which to print the data
 * @param len           The length of the buffer
 */
bool tlm_variable_get_value(const char *comp_name, const char *name, char *buffer, int len);

/**
 * Prints the value of the given variable
 * @param reg_var       A registered variable, possibly obtained from tlm_variable_get_by_name()
 * @param buffer        The buffer at which to print the data
 * @param len           The length of the buffer
 * @returns             true upon success
 */
bool tlm_variable_print_value(const tlm_reg_var_type *reg_var, char *buffer, int len);



#ifdef __cplusplus
}
#endif
#endif /* C_TLM_VAR_H__ */
