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

#include "azimuth/control/victory.h"

#include <assert.h>

#include "azimuth/control/util.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/music.h"
#include "azimuth/state/player.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/state/victory.h"
#include "azimuth/tick/victory.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/victory.h"

/*===========================================================================*/

#define AZ_LOW_PERCENT_COMPLETION 15

void az_victory_event_loop(az_saved_games_t *saved_games,
                           const az_player_t *player) {
  static az_victory_state_t state;
  AZ_ZERO_OBJECT(&state);
  state.clear_time = player->total_time;
  int num_upgrades = 0;
  for (int i = 0; i < AZ_NUM_UPGRADES; ++i) {
    if (az_has_upgrade(player, (az_upgrade_t)i)) {
      ++num_upgrades;
    }
  }
  AZ_STATIC_ASSERT(AZ_NUM_UPGRADES == 100);
  state.percent_completion = num_upgrades;

  // Update records.
  if (saved_games->highest_percentage < state.percent_completion) {
    saved_games->highest_percentage = state.percent_completion;
  }
  if (saved_games->lowest_percentage < 0 ||
      saved_games->lowest_percentage > state.percent_completion) {
    saved_games->lowest_percentage = state.percent_completion;
  }
  if (saved_games->best_any_percent_time <= 0.0 ||
      saved_games->best_any_percent_time > state.clear_time) {
    saved_games->best_any_percent_time = state.clear_time;
  }
  if (state.percent_completion >= 100 &&
      (saved_games->best_100_percent_time <= 0.0 ||
       saved_games->best_100_percent_time > state.clear_time)) {
    saved_games->best_100_percent_time = state.clear_time;
  }
  if (state.percent_completion <= AZ_LOW_PERCENT_COMPLETION &&
      (saved_games->best_low_percent_time <= 0.0 ||
       saved_games->best_low_percent_time > state.clear_time)) {
    saved_games->best_low_percent_time = state.clear_time;
  }
  az_save_saved_games(saved_games);

  az_change_music(&state.soundboard, AZ_MUS_TITLE);

  while (true) {
    // Tick the state and redraw the screen.
    az_tick_victory_state(&state, 1.0/60.0);
    az_tick_audio(&state.soundboard);
    az_start_screen_redraw(); {
      az_victory_draw_screen(&state);
    } az_finish_screen_redraw();

    // Get and process GUI events.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
        case AZ_EVENT_MOUSE_DOWN:
          if (state.step == AZ_VS_DONE) return;
          break;
        default: break;
      }
    }
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
