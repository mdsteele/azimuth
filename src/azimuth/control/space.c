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

#include "azimuth/control/space.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h> // for sprintf

#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/save.h"
#include "azimuth/state/space.h"
#include "azimuth/system/resource.h"
#include "azimuth/tick/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/space.h"

/*===========================================================================*/

static az_space_state_t state;

static void begin_saved_game(const az_planet_t *planet,
                             const az_saved_games_t *saved_games,
                             int saved_game_index) {
  assert(saved_game_index >= 0);
  assert(saved_game_index < AZ_ARRAY_SIZE(saved_games->games));
  const az_saved_game_t *saved_game = &saved_games->games[saved_game_index];

  memset(&state, 0, sizeof(state));
  state.planet = planet;
  state.save_file_index = saved_game_index;
  state.mode = AZ_MODE_NORMAL;

  if (saved_game->present) {
    // Resume saved game:
    state.ship.player = saved_game->player;
    az_enter_room(&state, &planet->rooms[state.ship.player.current_room]);
    AZ_ARRAY_LOOP(node, state.nodes) {
      if (node->kind == AZ_NODE_SAVE_POINT) {
        state.ship.position = node->position;
        state.ship.angle = node->angle;
        break;
      }
    }
  } else {
    // Begin new game:
    az_init_player(&state.ship.player);
    state.ship.player.current_room = planet->start_room;
    az_enter_room(&state, &planet->rooms[planet->start_room]);
    state.ship.position = planet->start_position;
    state.ship.angle = planet->start_angle;
  }

  state.ship.velocity = AZ_VZERO;
  state.camera = state.ship.position;
}

static bool save_current_game(az_saved_games_t *saved_games) {
  assert(state.save_file_index >= 0);
  assert(state.save_file_index < AZ_ARRAY_SIZE(saved_games->games));
  az_saved_game_t *saved_game = &saved_games->games[state.save_file_index];
  saved_game->present = true;
  saved_game->player = state.ship.player;
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  return az_save_games_to_file(saved_games, path_buffer);
}

az_space_action_t az_space_event_loop(const az_planet_t *planet,
                                      az_saved_games_t *saved_games,
                                      int saved_game_index) {
  begin_saved_game(planet, saved_games, saved_game_index);

  while (true) {
    // Tick the state:
    az_tick_space_state(&state, 1.0/60.0);
    // Draw the screen:
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();

    if (state.mode == AZ_MODE_GAME_OVER) {
      if (state.mode_data.game_over.step == AZ_GOS_FADE_OUT &&
          state.mode_data.game_over.progress >= 1.0) {
        return AZ_SA_GAME_OVER;
      }
    } else if (state.mode == AZ_MODE_SAVING) {
      const bool ok = save_current_game(saved_games);
      state.message.time_remaining = 4.0;
      if (ok) {
        state.message.string = "Saved game.";
        state.message.length = 11;
      } else {
        state.message.string = "Save failed.";
        state.message.length = 12;
      }
      state.mode = AZ_MODE_NORMAL;
    }

    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          switch (event.key.name) {
            case AZ_KEY_UP_ARROW: state.ship.controls.up = true; break;
            case AZ_KEY_DOWN_ARROW: state.ship.controls.down = true; break;
            case AZ_KEY_LEFT_ARROW: state.ship.controls.left = true; break;
            case AZ_KEY_RIGHT_ARROW: state.ship.controls.right = true; break;
            case AZ_KEY_V:
              state.ship.controls.fire_pressed = true;
              state.ship.controls.fire_held = true;
              break;
            case AZ_KEY_C:
              state.ship.controls.ordn_held = true;
              break;
            case AZ_KEY_X:
              state.ship.controls.util_pressed = true;
              state.ship.controls.util_held = true;
              break;
            case AZ_KEY_Z: state.ship.controls.burn = true; break;
            default: break;
          }
          break;
        case AZ_EVENT_KEY_UP:
          if (state.mode == AZ_MODE_UPGRADE &&
              state.mode_data.upgrade.step == AZ_UGS_MESSAGE) {
            state.mode_data.upgrade.step = AZ_UGS_CLOSE;
            state.mode_data.upgrade.progress = 0.0;
          }
          switch (event.key.name) {
            case AZ_KEY_UP_ARROW: state.ship.controls.up = false; break;
            case AZ_KEY_DOWN_ARROW: state.ship.controls.down = false; break;
            case AZ_KEY_LEFT_ARROW: state.ship.controls.left = false; break;
            case AZ_KEY_RIGHT_ARROW: state.ship.controls.right = false; break;
            case AZ_KEY_V: state.ship.controls.fire_held = false; break;
            case AZ_KEY_C: state.ship.controls.ordn_held = false; break;
            case AZ_KEY_X: state.ship.controls.util_held = false; break;
            case AZ_KEY_Z: state.ship.controls.burn = false; break;
            default: break;
          }
          break;
        default: break;
      }
    }
  }
}

/*===========================================================================*/
