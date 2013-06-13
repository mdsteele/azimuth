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

#include "azimuth/util/prefs.h"

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

/*===========================================================================*/

// Serialize the paragraph to a file and return true, or return false on error
// (e.g. if file I/O fails).
bool az_fprint_paragraph(const char *paragraph, FILE *file);

// Parse and allocate the paragraph.
char *az_sscan_paragraph(const char *string, int length);

// Return the number of lines in the paragraph.  This will be at least 1,
// even for an empty string.
int az_paragraph_num_lines(const char *paragraph);

// Return the length, in printed characters, of the line in the paragraph
// starting at paragraph[start].
int az_paragraph_line_length(
    const az_preferences_t *prefs, const char *paragraph, int start);

// Return the total length, in printed characters, of the paragraph.
int az_paragraph_total_length(
    const az_preferences_t *prefs, const char *paragraph);

/*===========================================================================*/

#endif // AZIMUTH_STATE_DIALOG_H_
