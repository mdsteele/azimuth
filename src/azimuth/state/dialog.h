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
#ifndef AZIMUTH_STATE_DIALOG_H_
#define AZIMUTH_STATE_DIALOG_H_

#include <stdbool.h>
#include <stdio.h> // for FILE

#include "azimuth/util/color.h"

/*===========================================================================*/

// The number of different portraits there are, not counting AZ_POR_NOTHING:
#define AZ_NUM_PORTRAITS 6

typedef enum {
  AZ_POR_NOTHING = 0,
  AZ_POR_HOPPER,
  AZ_POR_HQ,
  AZ_POR_CPU_A,
  AZ_POR_CPU_B,
  AZ_POR_CPU_C,
  AZ_POR_TRICHORD
} az_portrait_t;

typedef struct {
  az_color_t color;
  int length;
  char *chars; // not NUL-terminated; owned by text-fragment object
} az_text_fragment_t;

typedef struct {
  int total_length; // sum of lengths of fragments
  int num_fragments;
  az_text_fragment_t *fragments;
} az_text_line_t;

typedef struct {
  int num_lines;
  az_text_line_t *lines;
} az_text_t;

// Serialize the text to a file and return true, or return false on error
// (e.g. if file I/O fails).
bool az_fprint_text(const az_text_t *text, FILE *file);

// Parse the text and return true, or return false on error.
bool az_sscan_text(const char *string, int length, az_text_t *text_out);

// Make a deep copy of a text object (so that destroying one will not affect
// the other).  The argument pointers must not be equal.
void az_clone_text(const az_text_t *text, az_text_t *copy_out);

// Delete the data arrays owned by a text (but not the text object itself).
void az_destroy_text(az_text_t *text);

/*===========================================================================*/

#endif // AZIMUTH_STATE_DIALOG_H_
