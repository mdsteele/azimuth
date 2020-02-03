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

// The current control that a key press corresponds to.
typedef enum {
  AZ_CONTROL_NONE = 0,
  AZ_CONTROL_UP,
  AZ_CONTROL_DOWN,
  AZ_CONTROL_RIGHT,
  AZ_CONTROL_LEFT,
  AZ_CONTROL_FIRE,
  AZ_CONTROL_ORDN,
  AZ_CONTROL_UTIL,
  AZ_CONTROL_PAUSE,
  AZ_CONTROL_BOMBS,
  AZ_CONTROL_CHARGE,
  AZ_CONTROL_FREEZE,
  AZ_CONTROL_TRIPLE,
  AZ_CONTROL_HOMING,
  AZ_CONTROL_PHASE,
  AZ_CONTROL_BURST,
  AZ_CONTROL_PIERCE,
  AZ_CONTROL_BEAM,
  AZ_CONTROL_ROCKETS,
} az_control_id_t;

#define AZ_FIRST_CONTROL AZ_CONTROL_UP
#define AZ_NUM_CONTROLS (AZ_CONTROL_ROCKETS + 1)

typedef struct {
  float music_volume, sound_volume;
  bool speedrun_timer, fullscreen_on_startup, enable_hints;
  az_key_id_t key_for_control[AZ_NUM_CONTROLS];
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
bool az_is_valid_prefs_key(az_key_id_t key_id, az_control_id_t control_index);

// Returns true if user has configured a non-default key for weapon
// slots 0-9, i.e., AZ_CONTROL_BOMBS to AZ_CONTROL_ROCKETS.
bool az_show_extra_weapon_key(const az_preferences_t *prefs,
                              unsigned int slot);

// Returns the control for a given key.
az_control_id_t az_control_for_key(const az_preferences_t *prefs,
                                   az_key_id_t key_id);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_PREFS_H_
