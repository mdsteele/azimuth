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
#include <stdbool.h>

#include "azimuth/constants.h"
#include "azimuth/control/util.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/save.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/prefs.h"
#include "azimuth/view/title.h"

/*===========================================================================*/

static void erase_saved_game(az_saved_games_t *saved_games, int slot_index) {
  saved_games->games[slot_index].present = false;
  az_save_saved_games(saved_games);
}

static void copy_saved_game(az_saved_games_t *saved_games, int src_index,
                            int dest_index) {
  saved_games->games[dest_index] = saved_games->games[src_index];
  az_save_saved_games(saved_games);
}

static void clear_records(az_saved_games_t *saved_games) {
  az_clear_completion_records(saved_games);
  az_save_saved_games(saved_games);
}

az_title_action_t az_title_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    az_preferences_t *prefs, az_title_intro_t title_intro) {
  static az_title_state_t state;
  az_init_title_state(&state, planet, saved_games, prefs);
  if (title_intro != AZ_TI_SHOW_INTRO) {
    az_title_skip_intro(&state);
    state.mode = AZ_TMODE_NORMAL;
  }
  if (title_intro == AZ_TI_PLANET_DEBRIS) {
    state.show_planet_debris = true;
  }

  bool prefs_changed = false;

  while (true) {
    // Tick the state and redraw the screen.
    az_tick_title_state(&state, AZ_FRAME_TIME_SECONDS);
    az_tick_audio(&state.soundboard);
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
            az_prefs_try_pick_key(&state.prefs_pane, event.key.id,
                                  &state.soundboard);
            state.mode = AZ_TMODE_PREFS;
          } else if (state.mode == AZ_TMODE_INTRO) {
            if (event.key.id == AZ_KEY_ESCAPE ||
                event.key.id == AZ_KEY_RETURN) {
              az_title_skip_intro(&state);
            }
          } else if (state.mode == AZ_TMODE_READY) {
            state.mode = AZ_TMODE_NORMAL;
          } else if (state.mode == AZ_TMODE_NORMAL) {
            switch (event.key.id) {
              case AZ_KEY_1: az_title_start_game(&state, 0); break;
              case AZ_KEY_2: az_title_start_game(&state, 1); break;
              case AZ_KEY_3: az_title_start_game(&state, 2); break;
              case AZ_KEY_4: az_title_start_game(&state, 3); break;
              case AZ_KEY_5: az_title_start_game(&state, 4); break;
              case AZ_KEY_6: az_title_start_game(&state, 5); break;
              default: break;
            }
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          az_title_on_click(&state, event.mouse.x, event.mouse.y);
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
    // Check if we need to copy a saved game.
    if (state.mode == AZ_TMODE_COPYING &&
        state.mode_data.copying.src_index >= 0) {
      copy_saved_game(saved_games, state.mode_data.copying.src_index,
                      state.mode_data.copying.dest_index);
      state.mode = AZ_TMODE_NORMAL;
    }
    // Check if we need to clear records.
    if (state.mode == AZ_TMODE_CLEAR_RECORDS &&
        state.mode_data.clear_records.do_clear) {
      clear_records(saved_games);
      state.mode = AZ_TMODE_NORMAL;
    }

    az_update_prefefences(&state.prefs_pane, prefs, &prefs_changed);
  }
}

/*===========================================================================*/
