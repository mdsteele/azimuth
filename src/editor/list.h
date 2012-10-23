/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#pragma once
#ifndef EDITOR_LIST_H_
#define EDITOR_LIST_H_

#include <string.h> // for size_t

/*===========================================================================*/

// Declare a list with the given element type and name.
#define AZ_LIST_DECLARE(type, name) \
  struct { int num; int max; type *items; } name

// Initialize a list.  The list will be empty, and will have the given initial
// maximum size.
#define AZ_LIST_INIT(list, size) \
  (_az_list_init(&(list).num, &(list).max, (void **)&(list).items, \
                 sizeof((list).items[0]), size))

// Destroy a list, freeing associated memory.
#define AZ_LIST_DESTROY(list) \
  (_az_list_destroy(&(list).num, &(list).max, (void **)&(list).items))

// Get the current number of elements in a list.
#define AZ_LIST_SIZE(list) ((list).num)

// Add an item to the end of a list, returning a pointer to the new
// (uninitialized) item.  The list's memory will first be reallocated if it is
// currently at maximum capacity.
#define AZ_LIST_ADD(list) \
  ((__typeof__((list).items)) \
   _az_list_add(&(list).num, &(list).max, (void **)&(list).items, \
                sizeof((list).items[0])))

// Remove an item from a list, given a pointer to the item.  Other items in the
// list will be shifted down, and the list's memory will be reallocated if it
// becomes too empty.  The given pointer will no longer be valid.
#define AZ_LIST_REMOVE(list, ptr) \
  (_az_list_remove(&(list).num, &(list).max, (void **)&(list).items, \
                   sizeof((list).items[0]), ptr))

// Loop over all items in a list.  The var_name variable will be a pointer to
// successive items in the list.  It is NOT okay to add or remove items from
// the list while iterating.
#define AZ_LIST_LOOP(var_name, list) \
  for (__typeof__((list).items) var_name = (list).items; \
       var_name != (list).items + (list).num; ++var_name)

// Swap the contents of two lists.  The two lists must have the same type.
#define AZ_LIST_SWAP(list1, list2) do {                                 \
    if (0) (list1).items = (list2).items; /*warn if lists aren't same type*/ \
    _az_list_swap(&(list1).num, &(list1).max, (void **)&(list1).items,  \
                  &(list2).num, &(list2).max, (void **)&(list2).items); \
  } while (0)

/*===========================================================================*/

// Private functions.  Use the above macros instead of these directly.
void _az_list_init(int *num, int *max, void **items, size_t item_size,
                   int init_max);
void _az_list_destroy(int *num, int *max, void **items);
void *_az_list_add(int *num, int *max, void **items, size_t item_size);
void _az_list_remove(int *num, int *max, void **items, size_t item_size,
                     void *item);
void _az_list_swap(int *num1, int *max1, void **items1,
                   int *num2, int *max2, void **items2);

/*===========================================================================*/

#endif // EDITOR_LIST_H_
