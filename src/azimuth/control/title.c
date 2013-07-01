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

#include "azimuth/control/title.h"

#include <assert.h>
#include <stdio.h> // for sprintf
#include <string.h> // for memset

#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/save.h"
#include "azimuth/system/resource.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/title.h"

/*===========================================================================*/

static void erase_saved_game(az_saved_games_t *saved_games, int slot_index) {
  saved_games->games[slot_index].present = false;
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  (void)az_save_games_to_file(saved_games, path_buffer);
}

static void save_preferences(const az_preferences_t *prefs) {
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/prefs.txt", data_dir);
  (void)az_save_prefs_to_file(prefs, path_buffer);
}

static void try_start_game(az_title_state_t *state, int slot_index) {
  if (state->mode == AZ_TMODE_NORMAL) {
    az_title_start_game(state, slot_index);
  }
}

static bool try_pick_key(az_title_state_t *state, az_preferences_t *prefs,
                         az_key_id_t key_id) {
  assert(state->mode == AZ_TMODE_PICK_KEY);
  const int picker_index = state->mode_data.pick_key.picker_index;
  assert(picker_index >= 0 && picker_index < AZ_PREFS_NUM_KEYS);
  az_title_key_picker_t *picker = &state->pickers[picker_index];
  picker->selected = false;
  state->mode = AZ_TMODE_PREFS;
  if (az_is_valid_prefs_key(key_id)) {
    for (int i = 0; i < AZ_PREFS_NUM_KEYS; ++i) {
      if (i == picker_index) continue;
      const az_key_id_t other_key = prefs->keys[i];
      if (key_id == other_key) {
        prefs->keys[i] = state->pickers[i].key = picker->key;
        break;
      }
    }
    prefs->keys[picker_index] = picker->key = key_id;
    return true;
  } else return false;
}

az_title_action_t az_title_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    az_preferences_t *prefs) {
  static az_title_state_t state;
  memset(&state, 0, sizeof(state));
  state.planet = planet;
  state.saved_games = saved_games;
  state.music_slider.value = prefs->music_volume;
  state.sound_slider.value = prefs->sound_volume;
  for (int i = 0; i < AZ_PREFS_NUM_KEYS; ++i) {
    state.pickers[i].key = prefs->keys[i];
  }
  az_change_music(&state.soundboard, AZ_MUS_TITLE);

  bool prefs_changed = false;

  while (true) {
    // Tick the state and redraw the screen.
    az_tick_title_state(&state, 1.0/60.0);
    az_tick_audio_mixer(&state.soundboard);
    az_start_screen_redraw(); {
      az_title_draw_screen(&state);
    } az_finish_screen_redraw();

    // Check if we need to return with an action.
    if (state.mode == AZ_TMODE_QUITTING) {
      return (az_title_action_t){.kind = AZ_TA_QUIT};
    }
    if (state.mode == AZ_TMODE_STARTING &&
        state.mode_data.starting.progress >= 1.0) {
      return (az_title_action_t){.kind = AZ_TA_START_GAME,
          .slot_index = state.mode_data.starting.slot_index};
    }

    // Get and process GUI events.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          if (state.mode == AZ_TMODE_PICK_KEY) {
            if (try_pick_key(&state, prefs, event.key.id)) {
              prefs_changed = true;
            }
            break;
          }
          switch (event.key.id) {
            case AZ_KEY_1: try_start_game(&state, 0); break;
            case AZ_KEY_2: try_start_game(&state, 1); break;
            case AZ_KEY_3: try_start_game(&state, 2); break;
            case AZ_KEY_4: try_start_game(&state, 3); break;
            case AZ_KEY_5: try_start_game(&state, 4); break;
            case AZ_KEY_6: try_start_game(&state, 5); break;
            default: break;
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          az_title_on_click(&state, event.mouse.x, event.mouse.y);
          break;
        case AZ_EVENT_MOUSE_MOVE:
          az_title_on_hover(&state, event.mouse.x, event.mouse.y);
          break;
        default: break;
      }
    }

    // Check if we need to erase a saved game.
    if (state.mode == AZ_TMODE_ERASING &&
        state.mode_data.erasing.do_erase) {
      erase_saved_game(saved_games, state.mode_data.erasing.slot_index);
      state.mode = AZ_TMODE_NORMAL;
    }

    // Check if we need to change prefs.
    if (prefs->music_volume != state.music_slider.value) {
      prefs->music_volume = state.music_slider.value;
      az_set_global_music_volume(prefs->music_volume);
      prefs_changed = true;
    }
    if (prefs->sound_volume != state.sound_slider.value) {
      prefs->sound_volume = state.sound_slider.value;
      az_set_global_sound_volume(prefs->sound_volume);
      prefs_changed = true;
    }
    if (prefs_changed && !state.music_slider.grabbed &&
        !state.sound_slider.grabbed) {
      save_preferences(prefs);
      prefs_changed = false;
    }
  }
}

/*===========================================================================*/
