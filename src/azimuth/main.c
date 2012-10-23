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

#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h> // for main() renaming

#include "azimuth/constants.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/player.h"
#include "azimuth/state/room.h"
#include "azimuth/state/space.h"
#include "azimuth/state/wall.h"
#include "azimuth/system/resource.h"
#include "azimuth/tick/space.h" // for az_tick_space_state
#include "azimuth/util/random.h" // for az_init_random
#include "azimuth/util/vector.h"
#include "azimuth/view/space.h"
#include "azimuth/view/wall.h" // for az_init_wall_drawing

/*===========================================================================*/

static az_space_state_t state;

static bool load_scenario(void) {
  // Try to load the scenario data:
  const char *resource_dir = az_get_resource_directory();
  if (resource_dir == NULL) return false;
  char buffer[strlen(resource_dir) + 20];
  sprintf(buffer, "%s/rooms/room000.txt", resource_dir);
  az_room_t *room = az_load_room_from_file(buffer);
  if (room == NULL) return false;
  for (int i = 0; i < room->num_walls; ++i) {
    az_wall_t *wall;
    if (az_insert_wall(&state, &wall)) {
      *wall = room->walls[i];
    }
  }
  for (int i = 0; i < room->num_baddies; ++i) {
    const az_baddie_spec_t *spec = &room->baddies[i];
    az_baddie_t *baddie;
    if (az_insert_baddie(&state, &baddie)) {
      az_init_baddie(baddie, spec->kind, spec->position, spec->angle);
    }
  }
  az_destroy_room(room);

  // Set up the state:
  state.timer.active_for = -1;
  state.ship = (az_ship_t){
    .player = {
      .shields = 55, .max_shields = 400, .energy = 98, .max_energy = 400,
      .gun1 = AZ_GUN_HOMING, .gun2 = AZ_GUN_BURST, .ordnance = AZ_ORDN_NONE
    },
    .position = {50, 150}
  };
  state.camera = state.ship.position;
  az_give_upgrade(&state.ship.player, AZ_UPG_LATERAL_THRUSTERS);
  az_give_upgrade(&state.ship.player, AZ_UPG_RETRO_THRUSTERS);
  az_give_upgrade(&state.ship.player, AZ_UPG_ROCKET_AMMO_00);
  az_node_t *node;
  if (az_insert_node(&state, &node)) {
    node->kind = AZ_NODE_TRACTOR;
    node->position = (az_vector_t){150, -150};
    node->angle = 0;
  }

  return true;
}

static void event_loop(void) {
  while (true) {
    // Tick the state:
    az_tick_space_state(&state, 1.0/60.0);
    // Draw the screen:
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();

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
            case AZ_KEY_X: state.ship.controls.util = true; break;
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
            case AZ_KEY_X: state.ship.controls.util = false; break;
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
  az_init_gui(false);
  az_init_random();
  az_init_wall_datas();
  az_init_wall_drawing();

  if (!load_scenario()) {
    printf("Failed to load scenario.\n");
    return EXIT_FAILURE;
  }

  event_loop();

  return EXIT_SUCCESS;
}

/*===========================================================================*/
