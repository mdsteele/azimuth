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
#include <math.h> // for INFINITY
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // for EXIT_SUCCESS

#include <SDL/SDL.h> // for main() renaming

#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/room.h"
#include "azimuth/util/random.h" // for az_init_random
#include "editor/state.h"
#include "editor/view.h"

/*===========================================================================*/

static az_editor_state_t state;

static void do_save(void) {
  if (az_save_room_to_file(state.room, "data/rooms/room000.txt")) {
    state.unsaved = false;
  } else {
    printf("Failed to save room.\n");
  }
}

static void do_select(int x, int y) {
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  double best_dist = INFINITY;
  az_wall_t *best_wall = NULL;
  for (int i = 0; i < state.room->num_walls; ++i) {
    az_wall_t *wall = &state.room->walls[i];
    double dist = az_vnorm(az_vsub(wall->position, pt));
    if (dist <= wall->data->bounding_radius && dist < best_dist) {
      best_dist = dist;
      best_wall = wall;
    }
  }
  state.selected_wall = best_wall;
}

static void do_move(int x, int y, int dx, int dy) {
  if (state.selected_wall == NULL) return;
  const az_vector_t pt0 = az_pixel_to_position(&state, x - dx, y - dy);
  const az_vector_t pt1 = az_pixel_to_position(&state, x, y);
  state.selected_wall->position =
    az_vadd(state.selected_wall->position, az_vsub(pt1, pt0));
  state.unsaved = true;
}

static void do_rotate(int x, int y, int dx, int dy) {
  if (state.selected_wall == NULL) return;
  const az_vector_t pt0 = az_pixel_to_position(&state, x - dx, y - dy);
  const az_vector_t pt1 = az_pixel_to_position(&state, x, y);
  state.selected_wall->angle =
    az_mod2pi(state.selected_wall->angle +
              az_vtheta(az_vsub(pt1, state.selected_wall->position)) -
              az_vtheta(az_vsub(pt0, state.selected_wall->position)));
  state.unsaved = true;
}

static void do_add(int x, int y) {
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_room_t *room = state.room;
  assert(room->num_walls <= room->max_num_walls);
  if (room->num_walls == room->max_num_walls) {
    const int new_max = room->max_num_walls * 2 - room->max_num_walls / 2;
    az_wall_t *new_walls = realloc(room->walls, new_max * sizeof(az_wall_t));
    if (new_walls == NULL) return;
    room->max_num_walls = new_max;
    room->walls = new_walls;
  }
  assert(room->num_walls < room->max_num_walls);
  az_wall_t *wall = &room->walls[room->num_walls];
  wall->kind = AZ_WALL_NORMAL;
  wall->data = az_get_wall_data(0); // TODO allow selecting wall data
  wall->position = pt;
  wall->angle = 0.0;
  ++room->num_walls;
  state.selected_wall = wall;
  state.unsaved = true;
}

static void do_remove(void) {
  if (state.selected_wall == NULL) return;
  az_room_t *room = state.room;
  for (int i = state.selected_wall + 1 - room->walls;
       i < room->num_walls; ++i) {
    room->walls[i - 1] = room->walls[i];
  }
  --room->num_walls;
  state.selected_wall = NULL;
  state.unsaved = true;
  // Shrink walls array if it's now way too big.
  if (room->num_walls < room->max_num_walls / 3) {
    const int new_max = room->max_num_walls - room->max_num_walls / 3;
    assert(room->num_walls <= new_max);
    az_wall_t *new_walls = realloc(room->walls, new_max * sizeof(az_wall_t));
    if (new_walls != NULL) {
      room->max_num_walls = new_max;
      room->walls = new_walls;
    }
  }
}

static void event_loop(void) {
  while (true) {
    az_tick_editor_state(&state);
    az_start_screen_redraw(); {
      az_editor_draw_screen(&state);
    } az_finish_screen_redraw();

    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          switch (event.key.name) {
            case AZ_KEY_A: state.tool = AZ_TOOL_ADD; break;
            case AZ_KEY_C: state.spin_camera = !state.spin_camera; break;
            case AZ_KEY_M: state.tool = AZ_TOOL_MOVE; break;
            case AZ_KEY_R: state.tool = AZ_TOOL_ROTATE; break;
            case AZ_KEY_S:
              if (event.key.command) do_save();
              break;
            case AZ_KEY_BACKSPACE: do_remove(); break;
            case AZ_KEY_UP_ARROW: state.controls.up = true; break;
            case AZ_KEY_DOWN_ARROW: state.controls.down = true; break;
            case AZ_KEY_LEFT_ARROW: state.controls.left = true; break;
            case AZ_KEY_RIGHT_ARROW: state.controls.right = true; break;
            default: break;
          }
          break;
        case AZ_EVENT_KEY_UP:
          switch (event.key.name) {
            case AZ_KEY_UP_ARROW: state.controls.up = false; break;
            case AZ_KEY_DOWN_ARROW: state.controls.down = false; break;
            case AZ_KEY_LEFT_ARROW: state.controls.left = false; break;
            case AZ_KEY_RIGHT_ARROW: state.controls.right = false; break;
            default: break;
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          switch (state.tool) {
            case AZ_TOOL_MOVE:
            case AZ_TOOL_ROTATE:
              do_select(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_ADD:
              do_add(event.mouse.x, event.mouse.y);
              break;
          }
          break;
        case AZ_EVENT_MOUSE_MOVE:
          if (event.mouse.pressed) {
            switch (state.tool) {
              case AZ_TOOL_ADD:
              case AZ_TOOL_MOVE:
                do_move(event.mouse.x, event.mouse.y,
                        event.mouse.dx, event.mouse.dy);
                break;
              case AZ_TOOL_ROTATE:
                do_rotate(event.mouse.x, event.mouse.y,
                          event.mouse.dx, event.mouse.dy);
                break;
            }
          }
          break;
        default: break;
      }
    }
  }
}

int main(int argc, char **argv) {
  az_init_random();
  az_init_gui(false);

  az_room_t *room = az_load_room_from_file("data/rooms/room000.txt");
  if (room == NULL) printf("Failed to open room.\n");
  else {
    printf("Loaded room with %d walls.\n", room->num_walls);
    state.room = room;
    event_loop();
    az_destroy_room(room);
  }
  return EXIT_SUCCESS;
}

/*===========================================================================*/
