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

#ifndef C_TLM_COMP_H__
#define C_TLM_COMP_H__
#include "c_list.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * This file allows telemetry component registration.  Memory locations or variables
 * registered for telemetry are grouped by component.  Once registered, the telemetry
 * of these variables can be streamed to a file, network, or just stdio
 *
 * This file allows to register distinct components by name.  Components with duplicate
 * name will fail to be added to the components' list. Once a component is registered,
 * then variables underneath the component can be registered:  @see c_tlm_var.h
 *
 * Example registration of a component and a couple of variables :
 * @code
 *      tlm_component* comp = tlm_component_add("component 1");
 *      int a = 0;
 *      char b = 0;
 *      tlm_variable_register(comp, "a", &a, sizeof(a)));
 *      TLM_REG_VAR(comp, b); // Macro to register variable b
 * @endcode
 */

/**
 * Structure of a telemetry component.
 * Each component has a name, and a list of variables
 */
typedef struct {
    const char *name;    /** Name of the telemetry component */
    c_list_ptr var_list; /** List of the telemetry variables of this component */
} tlm_component;

/**
 * The callback type for each component @see tlm_component_for_each()
 */
typedef void (*tlm_comp_callback)(tlm_component *comp_ptr, void *arg1, void *arg2);

/**
 * Adds a telemetry component by name.
 * @param name The persistent data pointer to the name of the telemetry component.
 * @returns tlm_component pointer if this component added successfully, but NULL
 *          if another component already exists with this name.
 */
tlm_component* tlm_component_add(const char *name);

/**
 * Get an existing telemetry component by name
 * @returns NULL pointer if the component by name was not found
 */
tlm_component* tlm_component_get_by_name(const char *name);

/**
 * Calls your callback function for each telemetry component added to the
 * telemetry components list by tlm_component_add().
 * @param arg1 arg2  The arguments is passed to your callback like this:
 *              callback(tlm_component*, arg1, arg2)
 */
void tlm_component_for_each(tlm_comp_callback callback, void *arg1, void *arg2);



#ifdef __cplusplus
}
#endif
#endif /* C_TLM_COMP_H__ */
