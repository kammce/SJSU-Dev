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
 * @ingroup Utilities
 *
 * Linked list implementation in C.
 * This is a SINGLY linked list with the head and the tail pointers, therefore
 * insertion at either at the head or the tail will be quick.
 * @note This linked list doesn't copy data internally; it only keeps the link
 *       aka pointer to your data that you need to maintain yourself.
 *       In other words: Make sure the linked data doesn't go out of scope
 *       otherwise the list will basically contain 'dangling' pointer(s).
 *
 * Example code of list of integer pointers :
 * @code
 *      bool print_callback(void *elm_ptr, void *arg1, void *arg2, void *arg3)
 *      {
 *          printf("Value = %i\n", *(int*)elm_ptr);
 *          return true;
 *      }
 *      bool delete_callback(void *elm_ptr, void *arg1, void *arg2, void *arg3)
 *      {
 *          free(elm_ptr);
 *          return true;
 *      }
 *
 *      c_list_ptr list = c_list_create();
 *      int *a = malloc(sizeof(int));
 *      int *b = malloc(sizeof(int));
 *      *a = 1;
 *      *b = 2;
 *      c_list_insert_elm_beg(list, a);
 *      c_list_insert_elm_end(list, b);
 *
 *      // This will print 1 and 2 by our print_callback()
 *      c_list_for_each_elm(list, print_callback, NULL, NULL, NULL);
 *
 *      // Delete our list and free up "a" and "b"
 *      c_list_delete(list, delete_callback);
 * @endcode
 */
#ifndef C_LIST_H_
#define C_LIST_H_


/**************/
/** INCLUDES **/
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif


/************/
/** COMMON **/
typedef bool (*c_list_callback_t)(void *elm_ptr, void *arg1, void *arg2, void *arg3);

/**
 * Typedef of the c-list pointer type.
 * User shouldn't need to know about the internal structure of the list.
 */
typedef void* c_list_ptr;

/**
 * Creates a linked list structure
 * @returns Heap allocated list pointer.
 */
c_list_ptr c_list_create(void);

/**
 * Deletes the linked list and calls your del() function for each element.
 * @param list  The linked list pointer.
 * @param delete_callback  This can be NULL if you just want to free up the
 *        list and its nodes and you don't want to use the delete callback.
 *        If you provide this callback, then you will receive a callback with
 *        the elm_ptr and you decide what to do with the pointers you added to
 *        the list.
 */
bool c_list_delete(c_list_ptr list, c_list_callback_t delete_callback);

/**
 * @returns the number of items in the list.
 */
uint32_t c_list_node_count(const c_list_ptr list);

/**
 * @{ List insertion functions
 * Inserts your data pointer to the list
 * @param list  The list to insert a new node
 * @param elm_ptr   Pointer to your data.
 * @note The data at elm is not copied internally, only the pointer is copied.
 * This pointer thus, should not go out of scope after you add to the list.
 */
bool c_list_insert_elm_beg(c_list_ptr list, const void *elm_ptr);
bool c_list_insert_elm_end(c_list_ptr list, const void *elm_ptr);
/** @} */

/**
 * Gets the linked list element at the given index.
 *
 * @param list   The list pointer
 * @param index  The index location with bounds of 0 to c_list_node_count()
 * @param hint   Can be NULL if you don't want to use it.  You can use the hint
 *               to iterate through the elements faster if you are using a for
 *               loop.  See example below :
 *  @code
 *      void *hint = 0;
 *      for(int i=0; i < c_list_node_count(my_list); i++) {
 *          void *my_elm = c_list_get_elm_at(my_list, i, &hint);
 *      }
 *  @endcode
 *
 * @returns The element pointer or NULL if out of bound element is accessed
 */
void* c_list_get_elm_at(c_list_ptr list, uint32_t index, void **hint);

/**
 * Finds an element in the list.
 * When your callback returns false, this function will return that element back.
 * If your callback returns true and list iteration finishes, NULL pointer is returned.
 *
 * @param list     The list to iterate
 * @param callback The callback function.
 * @param arg1 arg2 arg3 The arguments to pass to your call-back function.
 */
void* c_list_find_elm(c_list_ptr list, c_list_callback_t callback,
                      void *arg1, void *arg2, void *arg3);

/**
 * Deletes an element by the pointer
 * @param list    The list to delete the node from
 * @param elm_ptr Pointer to the element that should be deleted.  Note that this
 *                will delete the first element it finds and will not remove all
 *                instances if duplicate elements were added.
 * @returns true if element was found and was deleted.
 */
bool c_list_delete_elm(c_list_ptr list, const void *elm_ptr);

/**
 * Iterates your list's element(s)
 * @param list    The list to iterate
 * @param func    The callback function.  The list will iterate as long as this callback
 *                function returns true.  When the callback function returns false,
 *                the iteration stops and immediately returns false.  If iteration
 *                finished and your callback always returned true, then this function will
 *                also return true.
 *
 * @param arg1 arg2 arg3 The arguments to pass to your call-back function.
 * @returns true if entire list was iterate without your callback returning false
 *
 * If your list contains integers, you can use the following to check for duplicates :
 * @code
 *      static bool check_dup(void *elm, void *new_int, void *arg2_unused, void *arg3_unused)
 *      {
 *          // Return true when we want c_list_for_each_elm() to continue
 *          return( *(int*)elm != *(int*)new_int);
 *      }
 *
 *      if (!c_list_for_each_elm(list, check_dup, (void*)new_var_ptr, NULL, NULL))
 *      {
 *          // Duplicate insertion when c_list_for_each_elm() returns false
 *      }
 * @endcode
 */
bool c_list_for_each_elm(const c_list_ptr list, c_list_callback_t func,
                           void *arg1, void *arg2, void *arg3);



#ifdef __cplusplus
}
#endif
#endif /* C_LIST_H_ */
