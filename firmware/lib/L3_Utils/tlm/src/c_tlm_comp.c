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
#include <string.h>
#include "c_tlm_comp.h"

/** Private member of this file */
static c_list_ptr mp_tlm_component_list = NULL;

static bool tlm_component_find_by_name(void *elm_ptr, void *arg1, void *arg2, void *arg3)
{
    tlm_component *comp = elm_ptr;
    char *new_comp_name = arg1;
    return (0 != strcmp(comp->name, new_comp_name));
}

static bool tlm_component_for_each_callback(void *elm_ptr, void *arg1, void *arg2, void *arg3)
{
    tlm_comp_callback callback = arg1;
    callback(elm_ptr, arg2, arg3);
    return true;
}


tlm_component* tlm_component_add(const char *name)
{
    if (NULL == name || *name == '\0') {
        return NULL;
    }

    /* Create component list if it doesn't exist */
    if(NULL == mp_tlm_component_list) {
        mp_tlm_component_list = c_list_create();
    }

    /* Check if this component exists */
    if (NULL != tlm_component_get_by_name(name)) {
        return NULL;
    }

    /* Allocate new component */
    tlm_component *new_comp = malloc(sizeof(tlm_component));
    if(NULL == new_comp) {
        return NULL;
    }
    memset(new_comp, 0, sizeof(tlm_component));

    /* Create the component and the list of variables of this component*/
    new_comp->name = name;
    new_comp->var_list = c_list_create();
    if(NULL == new_comp->var_list) {
        free(new_comp);
        return NULL;
    }

    /* Finally, add this component to our list */
    if(!c_list_insert_elm_end(mp_tlm_component_list, new_comp)) {
        free(new_comp->var_list);
        free(new_comp);
        return NULL;
    }

    return new_comp;
}

tlm_component* tlm_component_get_by_name(const char *name)
{
    tlm_component *comp = NULL;

    if (NULL != name) {
        comp = c_list_find_elm(mp_tlm_component_list, tlm_component_find_by_name,
                               (void*)name, NULL, NULL);
    }

    return comp;
}

void tlm_component_for_each(tlm_comp_callback callback, void *arg1, void *arg2)
{
    /*
     * c_list_for_each_elm() callback only has room for 3 arguments:
     * We only take 2 customer arguments because one of the argument is
     * our own callback
     */
    if (NULL != callback) {
        c_list_for_each_elm(mp_tlm_component_list, tlm_component_for_each_callback,
                            callback, arg1, arg2);
    }
}
