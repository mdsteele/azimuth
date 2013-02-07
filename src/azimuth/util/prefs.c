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

/*===========================================================================*/

void az_reset_prefs_to_defaults(az_preferences_t *prefs) {
  *prefs = (az_preferences_t){
    .music_volume = 0.8,
    .sound_volume = 0.8
  };
}

/*===========================================================================*/

bool az_load_prefs_from_file(const char *filepath,
                             az_preferences_t *prefs_out) {
  assert(filepath != NULL);
  assert(prefs_out != NULL);
  memset(prefs_out, 0, sizeof(*prefs_out));
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  double music_volume, sound_volume;
  const bool ok = (fscanf(
      file, "@F mv=%lf sv=%lf\n",
      &music_volume, &sound_volume) >= 2);
  fclose(file);
  if (!ok) return false;
  prefs_out->music_volume = (float)fmin(fmax(0.0, music_volume), 1.0);
  prefs_out->sound_volume = (float)fmin(fmax(0.0, sound_volume), 1.0);
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
      file, "@F mv=%.03f sv=%.03f\n",
      (double)prefs->music_volume, (double)prefs->sound_volume) >= 0);
  fclose(file);
  return ok;
}

/*===========================================================================*/
