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
#ifndef AZIMUTH_UTIL_PREFS_H_
#define AZIMUTH_UTIL_PREFS_H_

#include <stdbool.h>

#include "azimuth/util/key.h"

/*===========================================================================*/

typedef struct {
  float music_volume;
  float sound_volume;
  az_key_id_t up_key, down_key, right_key, left_key;
  az_key_id_t fire_key, ordn_key, util_key;
} az_preferences_t;

void az_reset_prefs_to_defaults(az_preferences_t *prefs);

// Attempt to open the file located at the given path and load preferences data
// from it.  Returns true on success, false on failure.
bool az_load_prefs_from_file(const char *filepath,
                             az_preferences_t *prefs_out);

// Attempt to save the given preferences to the file located at the given path.
// Return true on success, or false on failure.
bool az_save_prefs_to_file(const az_preferences_t *prefs,
                           const char *filepath);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_PREFS_H_
