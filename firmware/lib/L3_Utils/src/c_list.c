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

#include "c_list.h"
#include <stdlib.h>
#include <string.h>


/**
 * Data for each linked list node
 */
typedef struct c_data_node{
    void *data_ptr;           /**< Pointer to the data */
    struct c_data_node *next; /**< Pointer to the next data node */
} c_data_node_type;

/**
 * The linked list type with head and tail pointer
 */
typedef struct {
    struct c_data_node *head;
    struct c_data_node *tail;

    uint32_t node_count;
}c_list_type;



c_list_ptr c_list_create(void)
{
    c_list_type* new_list = (c_list_type*)malloc(sizeof(c_list_type));
    if(NULL != new_list) {
        memset(new_list, 0, sizeof(c_list_type));
    }
    return new_list;
}

bool c_list_delete(c_list_ptr p, c_list_callback_t delete_callback)
{
    c_list_type *list = p;
    if(!list) {
        return false;
    }

     c_data_node_type *iterator = list->head;
     while(NULL != iterator) {
         if(delete_callback) {
             delete_callback(iterator->data_ptr, NULL, NULL, NULL);
         }

         c_data_node_type *temp = iterator;
         iterator = iterator->next;
         free(temp);
     }

     list->head = NULL;
     list->tail = NULL;
     list->node_count = 0;

     free(list);

     return true;
}

uint32_t c_list_node_count(const c_list_ptr p)
{
    const c_list_type *list = p;
    return list ? list->node_count : 0;
}

bool c_list_insert_elm_end(c_list_ptr p, const void *elm_ptr)
{
    c_list_type *list = p;
    if(!list) {
        return false;
    }

    /* Allocate memory for the new node and copy the data */
    c_data_node_type *new_node = malloc(sizeof(c_data_node_type));
    if(NULL == new_node) {
        return false;
    }
    new_node->next = NULL;
    new_node->data_ptr = (void*)elm_ptr;

    if(NULL == list->head) {
        list->head = new_node;
        list->tail = new_node;
    }
    else {
        if (list->tail) {
            (list->tail)->next = new_node;
        }
        list->tail = new_node;
    }

    ++(list->node_count);
    return true;
}

bool c_list_insert_elm_beg(c_list_ptr p, const void *elm_ptr)
{
    c_list_type *list = p;
    if(!list) {
        return false;
    }

    /* Allocate memory for the new node and copy the data */
    c_data_node_type *new_node = malloc(sizeof(c_data_node_type));
    if(NULL == new_node) {
        return false;
    }
    new_node->next = list->head;
    new_node->data_ptr = (void*)elm_ptr;

    /* Set the head (and tail if necessary) to this node */
    list->head = new_node;
    if(NULL == list->tail) {
        list->tail = new_node;
    }

    ++(list->node_count);
    return true;
}

void* c_list_get_elm_at(c_list_ptr p, uint32_t index, void **hint)
{
    c_list_type *list = p;
    if(!list) {
        return false;
    }

    c_data_node_type **hint_node = (c_data_node_type**)hint;
    if (hint_node && 0 != index) {
        c_data_node_type *node = *hint_node;
        if (NULL != node) {
            *hint_node = node->next;
        }
        return node ? node->data_ptr : NULL;
    }
    else {
        c_data_node_type *iterator = list->head;
        while (index != 0 && NULL != iterator)
        {
            iterator = iterator->next;
            --index;
        }

        if (!iterator || 0 != index) {
            return NULL;
        }
        else {
            if (hint_node) {
                *hint_node = iterator->next;
            }
            return iterator->data_ptr;
        }
    }
}

void* c_list_find_elm(c_list_ptr p, c_list_callback_t callback,
                      void *arg1, void *arg2, void *arg3)
{
    c_list_type *list = p;
    if(!list || !callback) {
        return NULL;
    }

    c_data_node_type *iterator = list->head;
    while(NULL != iterator) {
        if(!callback(iterator->data_ptr, arg1, arg2, arg3)) {
            return iterator->data_ptr;
        }
        iterator = iterator->next;
    }

    return NULL;
}

bool c_list_delete_elm(c_list_ptr p, const void *elm_ptr)
{
    c_list_type *list = p;
    if(!list) {
        return false;
    }

    c_data_node_type *iterator = list->head;
    c_data_node_type *prev_node = NULL;

    while(NULL != iterator) {
        if(elm_ptr == iterator->data_ptr) {
            if(list->head == iterator) {
                list->head = iterator->next;
            }
            if(list->tail == iterator) {
                list->tail = prev_node;
            }

            /* A->B->C ==> A->C */
            if(prev_node) {
                prev_node->next = iterator->next;
            }

            --(list->node_count);
            free(iterator);
            return true;
        }

        prev_node = iterator;
        iterator = iterator->next;
    }

    return false;
}

bool c_list_for_each_elm(const c_list_ptr p, c_list_callback_t func,
                           void *arg1, void *arg2, void *arg3)
{
    const c_list_type *list = p;

    if(list && func) {
        c_data_node_type *iterator = list->head;
        while(NULL != iterator) {
            if(!func(iterator->data_ptr, arg1, arg2, arg3)) {
                return false;
            }
            iterator = iterator->next;
        }
    }

    return true;
}

#if 0 /* Turn to 1 to enable test code */
#include <assert.h>
#include <stdio.h>

static uint32_t del_callback_count = 0;
bool del_callback(void *elm_ptr, void *arg1, void *arg2, void *arg3)
{
    del_callback_count++;
    return true;
}

bool del_callback_free(void *elm_ptr, void *arg1, void *arg2, void *arg3)
{
    del_callback_count++;
    free(elm_ptr);
    return true;
}

bool test_list(void)
{
    int i = 0;

    puts("Test: C-List");
    c_list_type *p_list = c_list_create();
    assert(p_list);
    do {
        assert(!p_list->head);
        assert(!p_list->tail);
    } while(0);

    c_list_insert_elm_end(p_list, (void*)1);
    do {
        c_data_node_type *p_it = p_list->head;
        assert(p_list->head == p_list->tail);
        assert((void*)1 == p_it->data_ptr);
        assert(!(p_it->next));
    } while(0);

    c_list_insert_elm_end(p_list, (void*) 2);
    do {
        c_data_node_type *p_it = p_list->head;
        assert((void*)1 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)2 == p_it->next->data_ptr);
        assert(!(p_it->next->next));
    } while(0);

    c_list_insert_elm_end(p_list, (void*) 3);
    do {
        c_data_node_type *p_it = p_list->head;
        assert((void*)1 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)2 == p_it->next->data_ptr);
        assert((p_it->next->next));
        assert((void*)3 == p_it->next->next->data_ptr);
        assert(!(p_it->next->next->next ));
    } while(0);

    assert(!c_list_delete_elm(p_list, (void*)4));
    assert(c_list_delete_elm(p_list, (void*)1));
    do {
        c_data_node_type *p_it = p_list->head;
        assert((void*)2 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)3 == p_it->next->data_ptr);
        assert(!(p_it->next->next));
    } while(0);

    assert(c_list_delete_elm(p_list, (void*)2));
    do {
        c_data_node_type *p_it = p_list->head;
        assert((void*)3 == p_it->data_ptr);
        assert(!p_it->next);
        assert(p_list->head == p_list->tail);
    } while(0);

    assert(c_list_delete_elm(p_list, (void*)3));
    do {
        assert(p_list->head == 0);
        assert(p_list->tail == 0);
    } while(0);

    c_list_insert_elm_end(p_list, (void*) 1);
    c_list_insert_elm_end(p_list, (void*) 2);
    c_list_insert_elm_end(p_list, (void*) 3);
    assert(c_list_delete_elm(p_list, (void*)2));
    do {
        c_data_node_type *p_it = p_list->head;
        assert((void*)1 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)3 == p_it->next->data_ptr);
        assert(!(p_it->next->next));
    } while(0);

    assert(c_list_delete_elm(p_list, (void*)1));
    assert(c_list_delete_elm(p_list, (void*)3));
    do {
        assert(p_list->head == 0);
        assert(p_list->tail == 0);
    } while(0);

    c_list_type *list2 = c_list_create();
    assert(list2);
    do {
        assert(!list2->head);
        assert(!list2->tail);
    } while(0);

    assert(c_list_insert_elm_beg(list2, (void*)2));
    do {
        c_data_node_type *p_it = list2->head;
        assert(list2->head == list2->tail);
        assert((void*)2 == p_it->data_ptr);
        assert(!(p_it->next));
    } while(0);

    c_list_insert_elm_beg(list2, (void*) 1);
    do {
        c_data_node_type *p_it = list2->head;
        assert((void*)1 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)2 == p_it->next->data_ptr);
        assert(!(p_it->next->next));;
        assert(list2->head != list2->tail);
        assert(list2->tail == list2->head->next);
    } while(0);

    c_list_insert_elm_end(list2, (void*) 3);
    do {
        c_data_node_type *p_it = list2->head;
        assert((void*)1 == p_it->data_ptr);
        assert(p_it->next);
        assert((void*)2 == p_it->next->data_ptr);
        assert((p_it->next->next));
        assert((void*)3 == p_it->next->next->data_ptr);
        assert(!(p_it->next->next->next ));
    } while(0);

    assert((void*)1 == c_list_get_elm_at(list2, 0, NULL));
    assert((void*)2 == c_list_get_elm_at(list2, 1, NULL));
    assert((void*)3 == c_list_get_elm_at(list2, 2, NULL));
    assert((void*)0 == c_list_get_elm_at(list2, 3, NULL));
    assert((void*)0 == c_list_get_elm_at(list2, 4, NULL));

    assert((void*)1 == c_list_get_elm_at(list2, 0, NULL));
    assert((void*)2 == c_list_get_elm_at(list2, 1, NULL));
    assert((void*)3 == c_list_get_elm_at(list2, 2, NULL));

    void *hint = 0;
    assert((void*)1 == c_list_get_elm_at(list2, 0, &hint));
    assert((void*)2 == c_list_get_elm_at(list2, 1, &hint));
    assert((void*)3 == c_list_get_elm_at(list2, 2, &hint));
    assert((void*)0 == c_list_get_elm_at(list2, 3, &hint));
    assert((void*)0 == c_list_get_elm_at(list2, 4, &hint));

    assert((void*)1 == c_list_get_elm_at(list2, 0, &hint));
    assert((void*)2 == c_list_get_elm_at(list2, 1, &hint));
    assert((void*)3 == c_list_get_elm_at(list2, 2, &hint));

    del_callback_count = 0;
    c_list_delete(list2, del_callback);
    assert(3 == del_callback_count);

    list2 = c_list_create();
    for (i=0; i<10; i++) {
        c_list_insert_elm_beg(list2, malloc(1));
    }
    del_callback_count = 0;
    c_list_delete(list2, del_callback_free);
    assert(10 == del_callback_count);

    return true;
}
#endif
