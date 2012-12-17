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

#include <stdio.h> // for sprintf
#include <string.h> // for memset

#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/save.h"
#include "azimuth/system/resource.h"
#include "azimuth/util/audio.h"
#include "azimuth/view/title.h"

/*===========================================================================*/

static void erase_saved_game(az_saved_games_t *saved_games, int index) {
  saved_games->games[index].present = false;
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  (void)az_save_games_to_file(saved_games, path_buffer);
}

az_title_action_t az_title_event_loop(az_saved_games_t *saved_games) {
  static az_title_state_t state;
  memset(&state, 0, sizeof(state));
  state.saved_games = saved_games;
  az_change_music(&state.soundboard, AZ_MUS_TITLE);

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
  }
}

/*===========================================================================*/
