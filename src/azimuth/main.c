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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h> // for main() renaming

#include "azimuth/constants.h"
#include "azimuth/control/title.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/room.h"
#include "azimuth/state/save.h"
#include "azimuth/state/space.h"
#include "azimuth/state/wall.h" // for az_init_wall_datas
#include "azimuth/system/resource.h"
#include "azimuth/tick/space.h" // for az_tick_space_state
#include "azimuth/util/random.h" // for az_init_random
#include "azimuth/util/vector.h"
#include "azimuth/view/space.h"
#include "azimuth/view/wall.h" // for az_init_wall_drawing

/*===========================================================================*/

static az_planet_t planet;
static az_saved_games_t saved_games;
static az_space_state_t state;

static bool load_scenario(void) {
  // Try to load the scenario data:
  const char *resource_dir = az_get_resource_directory();
  if (resource_dir == NULL) return false;
  if (!az_load_planet(resource_dir, &planet)) return false;
  state.planet = &planet;
  return true;
}

static void load_saved_games(void) {
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  if (!az_load_games_from_file(&planet, path_buffer, &saved_games)) {
    az_reset_saved_games(&saved_games);
  }
}

static void begin_saved_game(int index) {
  assert(index >= 0);
  assert(index < AZ_ARRAY_SIZE(saved_games.games));
  state.save_file_index = index;
  if (saved_games.games[index].present) {
    // Resume saved game:
    state.ship.player = saved_games.games[index].player;
    az_enter_room(&state, &planet.rooms[state.ship.player.current_room]);
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
    state.ship.player.current_room = planet.start_room;
    az_enter_room(&state, &planet.rooms[planet.start_room]);
    state.ship.position = planet.start_position;
    state.ship.angle = planet.start_angle;
  }
  state.ship.velocity = AZ_VZERO;
  state.camera = state.ship.position;
}

static bool save_current_game(void) {
  assert(state.save_file_index >= 0);
  assert(state.save_file_index < AZ_ARRAY_SIZE(saved_games.games));
  saved_games.games[state.save_file_index].present = true;
  saved_games.games[state.save_file_index].player = state.ship.player;
  const char *data_dir = az_get_app_data_directory();
  if (data_dir == NULL) return false;
  char path_buffer[strlen(data_dir) + 10u];
  sprintf(path_buffer, "%s/save.txt", data_dir);
  return az_save_games_to_file(&saved_games, path_buffer);
}

static void event_loop(int saved_game_index) {
  begin_saved_game(saved_game_index);

  while (true) {
    // Tick the state:
    az_tick_space_state(&state, 1.0/60.0);
    // Draw the screen:
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();

    if (state.mode == AZ_MODE_SAVING) {
      const bool ok = save_current_game();
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
            case AZ_KEY_X:
              state.ship.controls.util_pressed = true;
              state.ship.controls.util_held = true;
              break;
            case AZ_KEY_Z: state.ship.controls.burn = true; break;
            default: break;
          }
          break;
        case AZ_EVENT_KEY_UP:
          switch (event.key.name) {
            case AZ_KEY_UP_ARROW: state.ship.controls.up = false; break;
            case AZ_KEY_DOWN_ARROW: state.ship.controls.down = false; break;
            case AZ_KEY_LEFT_ARROW: state.ship.controls.left = false; break;
            case AZ_KEY_RIGHT_ARROW: state.ship.controls.right = false; break;
            case AZ_KEY_V: state.ship.controls.fire_held = false; break;
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

int main(int argc, char **argv) {
  az_init_random();
  az_init_wall_datas();
  az_register_gl_init_func(az_init_wall_drawing);
  az_init_gui(false);

  if (!load_scenario()) {
    printf("Failed to load scenario.\n");
    return EXIT_FAILURE;
  }

  load_saved_games();

  const az_title_action_t action = az_title_event_loop(&saved_games);
  switch (action.kind) {
    case AZ_TA_QUIT: return EXIT_SUCCESS;
    case AZ_TA_START_GAME:
      event_loop(action.slot_index);
      break;
  }

  return EXIT_SUCCESS;
}

/*===========================================================================*/
