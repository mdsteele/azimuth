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
#include <stdio.h>

#include "azimuth/util/key.h"

/*===========================================================================*/

#define AZ_PREFS_UP_KEY_INDEX 0
#define AZ_PREFS_DOWN_KEY_INDEX 1
#define AZ_PREFS_RIGHT_KEY_INDEX 2
#define AZ_PREFS_LEFT_KEY_INDEX 3
#define AZ_PREFS_FIRE_KEY_INDEX 4
#define AZ_PREFS_ORDN_KEY_INDEX 5
#define AZ_PREFS_UTIL_KEY_INDEX 6
#define AZ_PREFS_PAUSE_KEY_INDEX 7
#define AZ_PREFS_NUM_KEYS 8

typedef struct {
  float music_volume, sound_volume;
  bool speedrun_timer, fullscreen_on_startup, enable_hints;
  az_key_id_t keys[AZ_PREFS_NUM_KEYS];
} az_preferences_t;

void az_reset_prefs_to_defaults(az_preferences_t *prefs);

// Attempts to open the file located at the given path and load preferences
// data from it.  Returns true on success, or false on failure.
bool az_load_prefs_from_path(const char *filepath,
                             az_preferences_t *prefs_out);

// Attempts to load preferences data from the given file.  Returns true on
// success, or false on failure.
bool az_load_prefs_from_file(FILE *file, az_preferences_t *prefs_out);

// Attempts to save the given preferences to the file located at the given
// path.  Return true on success, or false on failure.
bool az_save_prefs_to_path(const az_preferences_t *prefs,
                           const char *filepath);

// Attempts to save the given preferences to the given file.  Return true on
// success, or false on failure.
bool az_save_prefs_to_file(const az_preferences_t *prefs, FILE *file);

// Returns true if this key may be used for one of the game controls.
bool az_is_valid_prefs_key(az_key_id_t key_id);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_PREFS_H_
