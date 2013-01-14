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

#include "editor/list.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h> // for memmove

#include "azimuth/util/misc.h" // for AZ_FATAL

/*===========================================================================*/

void _az_list_init(int *num, int *max, void **items, size_t item_size,
                   int init_max) {
  *items = calloc(init_max, item_size);
  if (*items == NULL) AZ_FATAL("Out of memory.\n");
  *max = init_max;
  *num = 0;
}

void _az_list_destroy(int *num, int *max, void **items) {
  free(*items);
  // Clobber list fields to be safe:
  *num = 0;
  *max = 0;
  *items = NULL;
}

void *_az_list_get(int num, void *items, size_t item_size, int index) {
  assert(index >= 0);
  assert(index < num);
  return (char *)items + index * item_size;
}

void *_az_list_add(int *num, int *max, void **items, size_t item_size) {
  // Sanity check that the list isn't bigger than its maximum size.
  const int old_num = *num, old_max = *max;
  assert(old_num <= old_max);
  // If we're at capacity, grow the list's underlying array.
  if (old_num == old_max) {
    const int new_max = 1 + old_max * 2 - old_max / 2;
    void *new_items = realloc(*items, new_max * item_size);
    if (new_items == NULL) AZ_FATAL("Out of memory.\n");
    *items = new_items;
    *max = new_max;
  }
  // Okay, we should now have capcity to spare.
  assert(old_num < *max);
  // Increment the size of the list and return a pointer to the new item.
  ++(*num);
  void *item = (char *)(*items) + old_num * item_size;
  memset(item, 0, item_size);
  return item;
}

void _az_list_remove(int *num, int *max, void **items, size_t item_size,
                     void *item) {
  // Calculate offsets.
  const int old_num = *num;
  const size_t total_size = old_num * item_size;
  const size_t offset_to_item = (char *)item - (char *)(*items);
  // Validate that the item pointer points to an item in the list.
  assert(offset_to_item >= 0);
  assert(offset_to_item < total_size);
  assert(offset_to_item % item_size == 0);
  // Shift down items that come after the removed item.
  memmove(item, (char *)item + item_size,
          total_size - offset_to_item - item_size);
  --(*num);
  // Shrink list if it's now way too big.
  const int old_max = *max;
  if (*num < old_max / 3) {
    const int new_max = old_max - old_max / 3;
    assert(*num <= new_max);
    void *new_items = realloc(*items, new_max * item_size);
    if (new_items == NULL) AZ_FATAL("Out of memory.\n");
    *max = new_max;
    *items = new_items;
  }
}

void _az_list_swap(int *num1, int *max1, void **items1,
                   int *num2, int *max2, void **items2) {
  int temp_num = *num1;
  int temp_max = *max1;
  void *temp_items = *items1;
  *num1 = *num2;
  *max1 = *max2;
  *items1 = *items2;
  *num2 = temp_num;
  *max2 = temp_max;
  *items2 = temp_items;
}

/*===========================================================================*/
