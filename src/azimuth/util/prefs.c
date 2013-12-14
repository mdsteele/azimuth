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
    .speedrun_timer = false, .fullscreen_on_startup = true,
    .keys = {
      [AZ_PREFS_UP_KEY_INDEX] = AZ_KEY_UP_ARROW,
      [AZ_PREFS_DOWN_KEY_INDEX] = AZ_KEY_DOWN_ARROW,
      [AZ_PREFS_RIGHT_KEY_INDEX] = AZ_KEY_RIGHT_ARROW,
      [AZ_PREFS_LEFT_KEY_INDEX] = AZ_KEY_LEFT_ARROW,
      [AZ_PREFS_FIRE_KEY_INDEX] = AZ_KEY_C,
      [AZ_PREFS_ORDN_KEY_INDEX] = AZ_KEY_X,
      [AZ_PREFS_UTIL_KEY_INDEX] = AZ_KEY_Z,
      [AZ_PREFS_PAUSE_KEY_INDEX] = AZ_KEY_ESCAPE
    }
  };
}

bool az_load_prefs_from_file(const char *filepath,
                             az_preferences_t *prefs_out) {
  assert(filepath != NULL);
  assert(prefs_out != NULL);
  az_reset_prefs_to_defaults(prefs_out);

  // Parse the file.
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  double music_volume, sound_volume;
  int speedrun_timer, fullscreen, keys[AZ_PREFS_NUM_KEYS];
  const bool ok = (fscanf(
      file, "@F mv=%lf sv=%lf st=%d fs=%d\n"
      "   uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d pk=%d\n",
      &music_volume, &sound_volume, &speedrun_timer, &fullscreen,
      &keys[AZ_PREFS_UP_KEY_INDEX], &keys[AZ_PREFS_DOWN_KEY_INDEX],
      &keys[AZ_PREFS_RIGHT_KEY_INDEX], &keys[AZ_PREFS_LEFT_KEY_INDEX],
      &keys[AZ_PREFS_FIRE_KEY_INDEX], &keys[AZ_PREFS_ORDN_KEY_INDEX],
      &keys[AZ_PREFS_UTIL_KEY_INDEX], &keys[AZ_PREFS_PAUSE_KEY_INDEX]) >= 12);
  fclose(file);
  if (!ok) return false;

  // Require all keys to be valid and different.
  for (int i = 0; i < AZ_ARRAY_SIZE(keys); ++i) {
    if (keys[i] <= 0 || keys[i] > (int)AZ_LAST_KEY_ID) return false;
    if (!az_is_valid_prefs_key((az_key_id_t)keys[i])) return false;
    for (int j = 0; j < i; ++j) {
      if (keys[i] == keys[j]) return false;
    }
  }

  prefs_out->music_volume = (float)fmin(fmax(0.0, music_volume), 1.0);
  prefs_out->sound_volume = (float)fmin(fmax(0.0, sound_volume), 1.0);
  prefs_out->speedrun_timer = (speedrun_timer != 0);
  prefs_out->fullscreen_on_startup = (fullscreen != 0);
  for (int i = 0; i < AZ_PREFS_NUM_KEYS; ++i) {
    prefs_out->keys[i] = (az_key_id_t)keys[i];
  }
  return true;
}

bool az_save_prefs_to_file(const az_preferences_t *prefs,
                           const char *filepath) {
  assert(prefs != NULL);
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = (fprintf(
      file, "@F mv=%.03f sv=%.03f st=%d fs=%d\n"
      "   uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d pk=%d\n",
      (double)prefs->music_volume, (double)prefs->sound_volume,
      (prefs->speedrun_timer ? 1 : 0), (prefs->fullscreen_on_startup ? 1 : 0),
      (int)prefs->keys[AZ_PREFS_UP_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_DOWN_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_RIGHT_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_LEFT_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_FIRE_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_ORDN_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_UTIL_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_PAUSE_KEY_INDEX]) >= 0);
  fclose(file);
  return ok;
}

bool az_is_valid_prefs_key(az_key_id_t key_id) {
  switch (key_id) {
    case AZ_KEY_UNKNOWN:
    case AZ_KEY_RETURN:
    case AZ_KEY_1:
    case AZ_KEY_2:
    case AZ_KEY_3:
    case AZ_KEY_4:
    case AZ_KEY_5:
    case AZ_KEY_6:
    case AZ_KEY_7:
    case AZ_KEY_8:
    case AZ_KEY_9:
    case AZ_KEY_0:
      return false;
    default:
      return true;
  }
}

/*===========================================================================*/
