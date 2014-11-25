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

#include "azimuth/control/util.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "azimuth/gui/audio.h"
#include "azimuth/state/save.h"
#include "azimuth/system/resource.h"
#include "azimuth/util/prefs.h"
#include "azimuth/util/string.h"
#include "azimuth/view/prefs.h"

/*===========================================================================*/

void az_load_preferences(az_preferences_t *prefs) {
  assert(prefs != NULL);
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char *prefs_path = az_strprintf("%s/prefs.txt", data_dir);
  if (!az_load_prefs_from_path(prefs_path, prefs)) {
    az_reset_prefs_to_defaults(prefs);
  }
  free(prefs_path);
}

bool az_save_preferences(const az_preferences_t *prefs) {
  assert(prefs != NULL);
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char *prefs_path = az_strprintf("%s/prefs.txt", data_dir);
  const bool success = az_save_prefs_to_path(prefs, prefs_path);
  free(prefs_path);
  return success;
}

void az_update_prefefences(const az_prefs_pane_t *pane,
                           az_preferences_t *prefs, bool *prefs_changed) {
  if (prefs->music_volume != pane->music_slider.value) {
    prefs->music_volume = pane->music_slider.value;
    az_set_global_music_volume(prefs->music_volume);
    *prefs_changed = true;
  }
  if (prefs->sound_volume != pane->sound_slider.value) {
    prefs->sound_volume = pane->sound_slider.value;
    az_set_global_sound_volume(prefs->sound_volume);
    *prefs_changed = true;
  }
  if (prefs->speedrun_timer != pane->speedrun_timer_checkbox.checked ||
      prefs->fullscreen_on_startup != pane->fullscreen_checkbox.checked) {
    prefs->speedrun_timer = pane->speedrun_timer_checkbox.checked;
    prefs->fullscreen_on_startup = pane->fullscreen_checkbox.checked;
    *prefs_changed = true;
  }
  for (int i = 0; i < AZ_PREFS_NUM_KEYS; ++i) {
    if (prefs->keys[i] != pane->pickers[i].key) {
      prefs->keys[i] = pane->pickers[i].key;
      *prefs_changed = true;
    }
  }
  if (*prefs_changed && !pane->music_slider.grabbed &&
      !pane->sound_slider.grabbed) {
    az_save_preferences(prefs);
    *prefs_changed = false;
  }
}

void az_load_saved_games(const az_planet_t *planet,
                         az_saved_games_t *saved_games) {
  assert(saved_games != NULL);
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char *save_path = az_strprintf("%s/save.txt", data_dir);
  if (!az_load_games_from_path(planet, save_path, saved_games)) {
    az_reset_saved_games(saved_games);
  }
  free(save_path);
}

bool az_save_saved_games(const az_saved_games_t *saved_games) {
  assert(saved_games != NULL);
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char *save_path = az_strprintf("%s/save.txt", data_dir);
  const bool success = az_save_games_to_path(saved_games, save_path);
  free(save_path);
  return success;
}

/*===========================================================================*/
