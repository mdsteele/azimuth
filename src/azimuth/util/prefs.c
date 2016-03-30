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

#ifdef __APPLE__
#define DEFAULT_FULLSCREEN true
#else
#define DEFAULT_FULLSCREEN false
#endif

void az_reset_prefs_to_defaults(az_preferences_t *prefs) {
  *prefs = (az_preferences_t){
    .music_volume = 0.8, .sound_volume = 0.8,
    .speedrun_timer = false, .fullscreen_on_startup = DEFAULT_FULLSCREEN,
    .enable_hints = false,
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


bool az_load_prefs_from_path(const char *filepath,
                             az_preferences_t *prefs_out) {
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  const bool ok = az_load_prefs_from_file(file, prefs_out);
  fclose(file);
  return ok;
}

static bool read_bool(FILE *file, bool *out) {
  int value;
  if (fscanf(file, "=%d ", &value) < 1) return false;
  *out = (bool)value;
  return true;
}

static bool read_key(FILE *file, az_key_id_t *out) {
  int value;
  if (fscanf(file, "=%d ", &value) < 1) return false;
  if (value <= 0 || value > (int)AZ_LAST_KEY_ID) return false;
  const az_key_id_t key = (az_key_id_t)value;
  if (!az_is_valid_prefs_key(key)) return false;
  *out = key;
  return true;
}

static bool read_volume(FILE *file, float *out) {
  double value;
  if (fscanf(file, "=%lf ", &value) < 1) return false;
  *out = fmin(fmax(0.0, value), 1.0);
  return true;
}

bool az_load_prefs_from_file(FILE *file, az_preferences_t *prefs_out) {
  assert(file != NULL);
  assert(prefs_out != NULL);

  az_preferences_t prefs;
  az_reset_prefs_to_defaults(&prefs);

  // Parse the file.
  if (fscanf(file, "@F ") < 0) return false;
  while (!feof(file)) {
    char name[3] = {0};
    name[0] = fgetc(file);
    name[1] = fgetc(file);
    if (strcmp(name, "mv") == 0) {
      if (!read_volume(file, &prefs.music_volume)) return false;
    }
    if (strcmp(name, "sv") == 0) {
      if (!read_volume(file, &prefs.sound_volume)) return false;
    }
    if (strcmp(name, "st") == 0) {
      if (!read_bool(file, &prefs.speedrun_timer)) return false;
    }
    if (strcmp(name, "fs") == 0) {
      if (!read_bool(file, &prefs.fullscreen_on_startup)) return false;
    }
    if (strcmp(name, "eh") == 0) {
      if (!read_bool(file, &prefs.enable_hints)) return false;
    }
    if (strcmp(name, "uk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_UP_KEY_INDEX])) return false;
    }
    if (strcmp(name, "dk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_DOWN_KEY_INDEX])) return false;
    }
    if (strcmp(name, "rk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_RIGHT_KEY_INDEX])) return false;
    }
    if (strcmp(name, "lk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_LEFT_KEY_INDEX])) return false;
    }
    if (strcmp(name, "fk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_FIRE_KEY_INDEX])) return false;
    }
    if (strcmp(name, "ok") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_ORDN_KEY_INDEX])) return false;
    }
    if (strcmp(name, "tk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_UTIL_KEY_INDEX])) return false;
    }
    if (strcmp(name, "pk") == 0) {
      if (!read_key(file, &prefs.keys[AZ_PREFS_PAUSE_KEY_INDEX])) return false;
    }
  }

  // Require all keys to be different.
  for (int i = 0; i < AZ_ARRAY_SIZE(prefs.keys); ++i) {
    for (int j = 0; j < i; ++j) {
      if (prefs.keys[i] == prefs.keys[j]) return false;
    }
  }

  *prefs_out = prefs;
  return true;
}

bool az_save_prefs_to_path(const az_preferences_t *prefs,
                           const char *filepath) {
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = az_save_prefs_to_file(prefs, file);
  fclose(file);
  return ok;
}

bool az_save_prefs_to_file(const az_preferences_t *prefs, FILE *file) {
  assert(prefs != NULL);
  assert(file != NULL);
  return (fprintf(
      file, "@F mv=%.03f sv=%.03f st=%d fs=%d eh=%d\n"
      "   uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d pk=%d\n",
      (double)prefs->music_volume, (double)prefs->sound_volume,
      (prefs->speedrun_timer ? 1 : 0), (prefs->fullscreen_on_startup ? 1 : 0),
      (prefs->enable_hints ? 1 : 0),
      (int)prefs->keys[AZ_PREFS_UP_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_DOWN_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_RIGHT_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_LEFT_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_FIRE_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_ORDN_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_UTIL_KEY_INDEX],
      (int)prefs->keys[AZ_PREFS_PAUSE_KEY_INDEX]) >= 0);
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
