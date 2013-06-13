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

#include "azimuth/util/prefs.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "azimuth/util/key.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_reset_prefs_to_defaults(az_preferences_t *prefs) {
  *prefs = (az_preferences_t){
    .music_volume = 0.8, .sound_volume = 0.8,
    .up_key = AZ_KEY_UP_ARROW, .down_key = AZ_KEY_DOWN_ARROW,
    .right_key = AZ_KEY_RIGHT_ARROW, .left_key = AZ_KEY_LEFT_ARROW,
    .fire_key = AZ_KEY_C, .ordn_key = AZ_KEY_X, .util_key = AZ_KEY_Z
  };
}

/*===========================================================================*/

bool az_load_prefs_from_file(const char *filepath,
                             az_preferences_t *prefs_out) {
  assert(filepath != NULL);
  assert(prefs_out != NULL);
  az_reset_prefs_to_defaults(prefs_out);

  // Parse the file.
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  double music_volume, sound_volume;
  int keys[7];
  const bool ok = (fscanf(
      file, "@F mv=%lf sv=%lf uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d\n",
      &music_volume, &sound_volume, &keys[0], &keys[1], &keys[2], &keys[3],
      &keys[4], &keys[5], &keys[6]) >= 9);
  fclose(file);
  if (!ok) return false;

  // Require all keys to be valid and different.
  for (int i = 0; i < AZ_ARRAY_SIZE(keys); ++i) {
    if (keys[i] <= 0 || keys[i] > (int)AZ_LAST_KEY_ID) return false;
    for (int j = 0; j < i; ++j) {
      if (keys[i] == keys[j]) return false;
    }
  }

  prefs_out->music_volume = (float)fmin(fmax(0.0, music_volume), 1.0);
  prefs_out->sound_volume = (float)fmin(fmax(0.0, sound_volume), 1.0);
  prefs_out->up_key = (az_key_id_t)keys[0];
  prefs_out->down_key = (az_key_id_t)keys[1];
  prefs_out->right_key = (az_key_id_t)keys[2];
  prefs_out->left_key = (az_key_id_t)keys[3];
  prefs_out->fire_key = (az_key_id_t)keys[4];
  prefs_out->ordn_key = (az_key_id_t)keys[5];
  prefs_out->util_key = (az_key_id_t)keys[6];
  return true;
}

/*===========================================================================*/

bool az_save_prefs_to_file(const az_preferences_t *prefs,
                           const char *filepath) {
  assert(prefs != NULL);
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = (fprintf(
      file, "@F mv=%.03f sv=%.03f uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d\n",
      (double)prefs->music_volume, (double)prefs->sound_volume,
      (int)prefs->up_key, (int)prefs->down_key, (int)prefs->right_key,
      (int)prefs->left_key, (int)prefs->fire_key, (int)prefs->ordn_key,
      (int)prefs->util_key) >= 0);
  fclose(file);
  return ok;
}

/*===========================================================================*/
