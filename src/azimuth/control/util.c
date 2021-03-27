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

#include <SDL_filesystem.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#if !defined(__WINDOWS__) && !defined(__APPLE__)
#include <string.h>
#include <sys/stat.h>
#include <wordexp.h>
#endif

#include "azimuth/gui/audio.h"
#include "azimuth/state/save.h"
#include "azimuth/system/resource.h"
#include "azimuth/util/prefs.h"
#include "azimuth/util/string.h"
#include "azimuth/view/prefs.h"

#define ORG_NAME ""
#define APP_NAME "Azimuth"

/*===========================================================================*/

// Get the path to the user-specific directory for storing persistent data for
// this application (e.g. preferences or save files) as a NUL-terminated string.
// If anything fails, this will return a NULL pointer.
// Free the returned string with SDL_free().
static char *az_get_app_data_directory(void) {
#if !defined(__WINDOWS__) && !defined(__APPLE__)
  // First, do tilde-expansion so we get a path in the user's homedir.
  wordexp_t words;
  wordexp("~/.azimuth-game", &words, 0);
  if (words.we_wordc < 1) {
    wordfree(&words);
    return NULL;
  }
  const size_t exp_len = strlen(words.we_wordv[0]);
  char* path_buffer = (char*)SDL_calloc(1, exp_len + 1);
  if (NULL == path_buffer) {
    wordfree(&words);
    return NULL;
  }
  strncpy(path_buffer, words.we_wordv[0], exp_len);
  wordfree(&words);
  struct stat stat_buffer;
  // If the path exists & is a directory, use it...
  if (stat(path_buffer, &stat_buffer) == 0 && S_ISDIR(stat_buffer.st_mode)) {
    return path_buffer;
  }
  // ...otherwise, use SDL's path.
  SDL_free(path_buffer);
#endif
  return SDL_GetPrefPath(ORG_NAME, APP_NAME);
}

void az_load_preferences(az_preferences_t *prefs) {
  assert(prefs != NULL);
  char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char *prefs_path = az_strprintf("%s/prefs.txt", data_dir);
  SDL_free(data_dir);
  if (!az_load_prefs_from_path(prefs_path, prefs)) {
    az_reset_prefs_to_defaults(prefs);
  }
  free(prefs_path);
}

bool az_save_preferences(const az_preferences_t *prefs) {
  assert(prefs != NULL);
  char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char *prefs_path = az_strprintf("%s/prefs.txt", data_dir);
  SDL_free(data_dir);
  const bool success = az_save_prefs_to_path(prefs, prefs_path);
  free(prefs_path);
  return success;
}

void az_update_preferences(const az_prefs_pane_t *pane,
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
      prefs->fullscreen_on_startup != pane->fullscreen_checkbox.checked ||
      prefs->enable_hints != pane->enable_hints_checkbox.checked) {
    prefs->speedrun_timer = pane->speedrun_timer_checkbox.checked;
    prefs->fullscreen_on_startup = pane->fullscreen_checkbox.checked;
    prefs->enable_hints = pane->enable_hints_checkbox.checked;
    *prefs_changed = true;
  }
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    if (prefs->key_for_control[i] != pane->pickers[i].key) {
      prefs->key_for_control[i] = pane->pickers[i].key;
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
  char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char *save_path = az_strprintf("%s/save.txt", data_dir);
  SDL_free(data_dir);
  if (!az_load_games_from_path(planet, save_path, saved_games)) {
    az_reset_saved_games(saved_games);
  }
  free(save_path);
}

bool az_save_saved_games(const az_saved_games_t *saved_games) {
  assert(saved_games != NULL);
  char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char *save_path = az_strprintf("%s/save.txt", data_dir);
  SDL_free(data_dir);
  const bool success = az_save_games_to_path(saved_games, save_path);
  free(save_path);
  return success;
}

/*===========================================================================*/
