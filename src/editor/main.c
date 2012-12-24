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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // for EXIT_SUCCESS

#include <SDL/SDL.h> // for main() renaming

#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/baddie.h" // for az_init_baddie_datas
#include "azimuth/state/planet.h"
#include "azimuth/state/room.h"
#include "azimuth/state/wall.h" // for az_init_wall_datas
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h" // for az_init_random
#include "azimuth/view/wall.h" // for az_init_wall_drawing
#include "editor/list.h"
#include "editor/state.h"
#include "editor/view.h"

/*===========================================================================*/

static az_editor_state_t state;

static void add_new_room(void) {
  const az_room_key_t room_key = AZ_LIST_SIZE(state.planet.rooms);
  az_editor_room_t *room = AZ_LIST_ADD(state.planet.rooms);
  const double current_r = az_vnorm(state.camera);
  const double theta_span = 200.0 / (80.0 + current_r);
  room->camera_bounds = (az_camera_bounds_t){
    .min_r = current_r - 100.0,
    .r_span = 200.0,
    .min_theta = az_mod2pi(az_vtheta(state.camera) - 0.5 * theta_span),
    .theta_span = theta_span
  };
  AZ_LIST_INIT(room->baddies, 5);
  AZ_LIST_INIT(room->doors, 5);
  AZ_LIST_INIT(room->nodes, 5);
  AZ_LIST_INIT(room->walls, 5);
  state.current_room = room_key;
  state.unsaved = true;
}

static void center_camera_on_current_room(void) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  double min_x = INFINITY, max_x = -INFINITY;
  double min_y = INFINITY, max_y = -INFINITY;
  AZ_LIST_LOOP(baddie, room->baddies) {
    min_x = fmin(min_x, baddie->spec.position.x);
    max_x = fmax(max_x, baddie->spec.position.x);
    min_y = fmin(min_y, baddie->spec.position.y);
    max_y = fmax(max_y, baddie->spec.position.y);
  }
  AZ_LIST_LOOP(door, room->doors) {
    min_x = fmin(min_x, door->spec.position.x);
    max_x = fmax(max_x, door->spec.position.x);
    min_y = fmin(min_y, door->spec.position.y);
    max_y = fmax(max_y, door->spec.position.y);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    min_x = fmin(min_x, node->spec.position.x);
    max_x = fmax(max_x, node->spec.position.x);
    min_y = fmin(min_y, node->spec.position.y);
    max_y = fmax(max_y, node->spec.position.y);
  }
  AZ_LIST_LOOP(wall, room->walls) {
    min_x = fmin(min_x, wall->spec.position.x);
    max_x = fmax(max_x, wall->spec.position.x);
    min_y = fmin(min_y, wall->spec.position.y);
    max_y = fmax(max_y, wall->spec.position.y);
  }
  if (min_x > max_x) return;
  else assert(min_y <= max_y);
  state.camera = (az_vector_t){(min_x + max_x) * 0.5, (min_y + max_y) * 0.5};
}

static void select_all(az_editor_room_t *room, bool selected) {
  AZ_LIST_LOOP(baddie, room->baddies) baddie->selected = selected;
  AZ_LIST_LOOP(door, room->doors) door->selected = selected;
  AZ_LIST_LOOP(node, room->nodes) node->selected = selected;
  AZ_LIST_LOOP(wall, room->walls) wall->selected = selected;
}

static void do_save(void) {
  // Convert planet:
  const int num_rooms = AZ_LIST_SIZE(state.planet.rooms);
  az_planet_t planet = {
    .start_room = state.planet.start_room,
    .start_position = state.planet.start_position,
    .start_angle = state.planet.start_angle,
    .num_rooms = num_rooms,
    .rooms = AZ_ALLOC(num_rooms, az_room_t)
  };
  // Convert rooms:
  for (az_room_key_t key = 0; key < num_rooms; ++key) {
    az_editor_room_t *eroom = AZ_LIST_GET(state.planet.rooms, key);
    az_room_t *room = &planet.rooms[key];
    room->camera_bounds = eroom->camera_bounds;
    // Convert baddies:
    room->num_baddies = AZ_LIST_SIZE(eroom->baddies);
    room->baddies = AZ_ALLOC(room->num_baddies, az_baddie_spec_t);
    for (int i = 0; i < room->num_baddies; ++i) {
      room->baddies[i] = AZ_LIST_GET(eroom->baddies, i)->spec;
    }
    // Convert doors:
    room->num_doors = AZ_LIST_SIZE(eroom->doors);
    room->doors = AZ_ALLOC(room->num_doors, az_door_spec_t);
    for (int i = 0; i < room->num_doors; ++i) {
      room->doors[i] = AZ_LIST_GET(eroom->doors, i)->spec;
    }
    // Convert nodes:
    room->num_nodes = AZ_LIST_SIZE(eroom->nodes);
    room->nodes = AZ_ALLOC(room->num_nodes, az_node_spec_t);
    for (int i = 0; i < room->num_nodes; ++i) {
      room->nodes[i] = AZ_LIST_GET(eroom->nodes, i)->spec;
    }
    // Convert walls:
    room->num_walls = AZ_LIST_SIZE(eroom->walls);
    room->walls = AZ_ALLOC(room->num_walls, az_wall_spec_t);
    for (int i = 0; i < room->num_walls; ++i) {
      room->walls[i] = AZ_LIST_GET(eroom->walls, i)->spec;
    }
  }
  // Write to disk:
  if (az_save_planet(&planet, "data")) {
    state.unsaved = false;
  } else {
    printf("Failed to save scenario.\n");
  }
  // Clean up:
  az_destroy_planet(&planet);
}

static void do_select(int x, int y, bool multi) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  double best_dist = INFINITY;
  az_editor_wall_t *best_wall = NULL;
  AZ_LIST_LOOP(wall, room->walls) {
    double dist = az_vdist(pt, wall->spec.position);
    if (dist <= wall->spec.data->bounding_radius && dist < best_dist) {
      best_dist = dist;
      best_wall = wall;
    }
  }
  az_editor_node_t *best_node = NULL;
  AZ_LIST_LOOP(node, room->nodes) {
    double dist = az_vdist(pt, node->spec.position);
    if (dist <= AZ_NODE_BOUNDING_RADIUS && dist < best_dist) {
      best_dist = dist;
      best_node = node;
    }
  }
  az_editor_baddie_t *best_baddie = NULL;
  AZ_LIST_LOOP(baddie, room->baddies) {
    double dist = az_vdist(pt, baddie->spec.position);
    if (dist <= az_get_baddie_data(baddie->spec.kind)->
                overall_bounding_radius &&
        dist < best_dist) {
      best_dist = dist;
      best_baddie = baddie;
    }
  }
  az_editor_door_t *best_door = NULL;
  AZ_LIST_LOOP(door, room->doors) {
    double dist = az_vdist(pt, door->spec.position);
    if (dist <= AZ_DOOR_BOUNDING_RADIUS && dist < best_dist) {
      best_dist = dist;
      best_door = door;
    }
  }
  // Select/deselect as appropriate:
  if (best_baddie != NULL) {
    if (multi) {
      best_baddie->selected = !best_baddie->selected;
    } else if (!best_baddie->selected) {
      select_all(room, false);
      best_baddie->selected = true;
    }
    state.brush.angle = best_baddie->spec.angle;
    state.brush.baddie_kind = best_baddie->spec.kind;
  } else if (best_door != NULL) {
    if (multi) {
      best_door->selected = !best_door->selected;
    } else if (!best_door->selected) {
      select_all(room, false);
      best_door->selected = true;
    }
    state.brush.angle = best_door->spec.angle;
    state.brush.door_kind = best_door->spec.kind;
  } else if (best_node != NULL) {
    if (multi) {
      best_node->selected = !best_node->selected;
    } else if (!best_node->selected) {
      select_all(room, false);
      best_node->selected = true;
    }
    state.brush.angle = best_node->spec.angle;
    state.brush.node_kind = best_node->spec.kind;
    if (best_node->spec.kind == AZ_NODE_UPGRADE) {
      state.brush.upgrade_kind = best_node->spec.upgrade;
    }
  } else if (best_wall != NULL) {
    if (multi) {
      best_wall->selected = !best_wall->selected;
    } else if (!best_wall->selected) {
      select_all(room, false);
      best_wall->selected = true;
    }
    state.brush.angle = best_wall->spec.angle;
    state.brush.wall_kind = best_wall->spec.kind;
    state.brush.wall_data_index = az_wall_data_index(best_wall->spec.data);
  } else if (!multi) {
    select_all(room, false);
  }
}

static void do_move(int x, int y, int dx, int dy) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_vector_t delta =
    az_vsub(az_pixel_to_position(&state, x, y),
            az_pixel_to_position(&state, x - dx, y - dy));
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    baddie->spec.position = az_vadd(baddie->spec.position, delta);
    state.unsaved = true;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    door->spec.position = az_vadd(door->spec.position, delta);
    state.unsaved = true;
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    node->spec.position = az_vadd(node->spec.position, delta);
    state.unsaved = true;
  }
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    wall->spec.position = az_vadd(wall->spec.position, delta);
    state.unsaved = true;
  }
}

static void do_mass_move(int x, int y, int dx, int dy) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  az_camera_bounds_t *camera_bounds = &room->camera_bounds;
  const az_vector_t normal =
    az_vpolar(1.0, camera_bounds->min_theta + 0.5 * camera_bounds->theta_span);
  const double dr =
    az_vdot(az_vsub(az_pixel_to_position(&state, x, y),
                    az_pixel_to_position(&state, x - dx, y - dy)), normal);
  // Change camera bounds:
  const double r1 = camera_bounds->min_r + 0.5 * camera_bounds->r_span;
  assert(r1 >= 0.0);
  camera_bounds->min_r = fmax(0.0, camera_bounds->min_r + dr);
  const double r2 = camera_bounds->min_r + 0.5 * camera_bounds->r_span;
  assert(r2 >= 0.0);
  if (r2 > 0.0) {
    const double new_span =
      2.0 * atan(tan(0.5 * camera_bounds->theta_span) * r1 / r2);
    camera_bounds->min_theta =
      az_mod2pi(camera_bounds->min_theta -
                0.5 * (new_span - camera_bounds->theta_span));
    camera_bounds->theta_span = new_span;
  }
  assert(camera_bounds->theta_span >= 0.0);
  // Move objects in the room:
  const az_vector_t delta = az_vmul(normal, dr);
  AZ_LIST_LOOP(baddie, room->baddies) {
    baddie->spec.position = az_vadd(baddie->spec.position, delta);
  }
  AZ_LIST_LOOP(door, room->doors) {
    door->spec.position = az_vadd(door->spec.position, delta);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    node->spec.position = az_vadd(node->spec.position, delta);
  }
  AZ_LIST_LOOP(wall, room->walls) {
    wall->spec.position = az_vadd(wall->spec.position, delta);
  }
  state.unsaved = true;
}

static void do_rotate(int x, int y, int dx, int dy) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_vector_t pt0 = az_pixel_to_position(&state, x - dx, y - dy);
  const az_vector_t pt1 = az_pixel_to_position(&state, x, y);
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    baddie->spec.angle = state.brush.angle =
      az_mod2pi(baddie->spec.angle +
                az_vtheta(az_vsub(pt1, baddie->spec.position)) -
                az_vtheta(az_vsub(pt0, baddie->spec.position)));
    state.unsaved = true;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    door->spec.angle = state.brush.angle =
      az_mod2pi(door->spec.angle +
                az_vtheta(az_vsub(pt1, door->spec.position)) -
                az_vtheta(az_vsub(pt0, door->spec.position)));
    state.unsaved = true;
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    node->spec.angle = state.brush.angle =
      az_mod2pi(node->spec.angle +
                az_vtheta(az_vsub(pt1, node->spec.position)) -
                az_vtheta(az_vsub(pt0, node->spec.position)));
    state.unsaved = true;
  }
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    wall->spec.angle = state.brush.angle =
      az_mod2pi(wall->spec.angle +
                az_vtheta(az_vsub(pt1, wall->spec.position)) -
                az_vtheta(az_vsub(pt0, wall->spec.position)));
    state.unsaved = true;
  }
}

static void do_mass_rotate(int x, int y, int dx, int dy) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const double dtheta =
    az_mod2pi(az_vtheta(az_pixel_to_position(&state, x, y)) -
              az_vtheta(az_pixel_to_position(&state, x - dx, y - dy)));
  room->camera_bounds.min_theta =
    az_mod2pi(room->camera_bounds.min_theta + dtheta);
  AZ_LIST_LOOP(baddie, room->baddies) {
    baddie->spec.position = az_vrotate(baddie->spec.position, dtheta);
    baddie->spec.angle = az_mod2pi(baddie->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(door, room->doors) {
    door->spec.position = az_vrotate(door->spec.position, dtheta);
    door->spec.angle = az_mod2pi(door->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    node->spec.position = az_vrotate(node->spec.position, dtheta);
    node->spec.angle = az_mod2pi(node->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(wall, room->walls) {
    wall->spec.position = az_vrotate(wall->spec.position, dtheta);
    wall->spec.angle = az_mod2pi(wall->spec.angle + dtheta);
  }
  state.unsaved = true;
}

static void do_set_camera_bounds(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  az_camera_bounds_t *bounds = &room->camera_bounds;
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  const double new_r = az_vnorm(pt);
  const double new_theta = az_vtheta(pt);
  // Update r-bounds:
  if (new_r < bounds->min_r + 0.5 * bounds->r_span) {
    bounds->r_span += bounds->min_r - new_r;
    bounds->min_r = new_r;
  } else {
    bounds->r_span = new_r - bounds->min_r;
  }
  // Update theta-bounds:
  if (az_mod2pi(new_theta -
                (bounds->min_theta + 0.5 * bounds->theta_span)) < 0.0) {
    bounds->theta_span += az_mod2pi(bounds->min_theta - new_theta);
    bounds->min_theta = new_theta;
  } else {
    bounds->theta_span = az_mod2pi(new_theta - bounds->min_theta);
    if (bounds->theta_span < 0.0) bounds->theta_span += AZ_TWO_PI;
  }
  // Verify that the new camera bounds are still valid:
  assert(bounds->min_r >= 0.0);
  assert(bounds->r_span >= 0.0);
  assert(bounds->min_theta >= -AZ_PI);
  assert(bounds->min_theta <= AZ_PI);
  assert(bounds->theta_span >= 0.0);
  assert(bounds->theta_span <= AZ_TWO_PI);
  state.unsaved = true;
}

static void do_add_baddie(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  select_all(room, false);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_editor_baddie_t *baddie = AZ_LIST_ADD(room->baddies);
  baddie->selected = true;
  baddie->spec.kind = state.brush.baddie_kind;
  baddie->spec.position = pt;
  baddie->spec.angle = state.brush.angle;
  state.unsaved = true;
}

static void do_add_door(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  select_all(room, false);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_editor_door_t *door = AZ_LIST_ADD(room->doors);
  door->selected = true;
  door->spec.kind = state.brush.door_kind;
  door->spec.position = pt;
  door->spec.angle = state.brush.angle;
  door->spec.destination = state.current_room;
  state.unsaved = true;
}

static void do_add_node(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  select_all(room, false);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_editor_node_t *node = AZ_LIST_ADD(room->nodes);
  node->selected = true;
  node->spec.kind = state.brush.node_kind;
  node->spec.position = pt;
  node->spec.angle = state.brush.angle;
  node->spec.upgrade = state.brush.upgrade_kind;
  state.unsaved = true;
}

static void do_add_wall(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  select_all(room, false);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_editor_wall_t *wall = AZ_LIST_ADD(room->walls);
  wall->selected = true;
  wall->spec.kind = state.brush.wall_kind;
  wall->spec.data = az_get_wall_data(state.brush.wall_data_index);
  wall->spec.position = pt;
  wall->spec.angle = state.brush.angle;
  state.unsaved = true;
}

static void do_remove(void) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  {
    AZ_LIST_DECLARE(az_editor_baddie_t, temp_baddies);
    AZ_LIST_INIT(temp_baddies, 2);
    AZ_LIST_LOOP(baddie, room->baddies) {
      if (!baddie->selected) *AZ_LIST_ADD(temp_baddies) = *baddie;
      else state.unsaved = true;
    }
    AZ_LIST_SWAP(temp_baddies, room->baddies);
    AZ_LIST_DESTROY(temp_baddies);
  }
  {
    AZ_LIST_DECLARE(az_editor_door_t, temp_doors);
    AZ_LIST_INIT(temp_doors, 2);
    AZ_LIST_LOOP(door, room->doors) {
      if (!door->selected) *AZ_LIST_ADD(temp_doors) = *door;
      else state.unsaved = true;
    }
    AZ_LIST_SWAP(temp_doors, room->doors);
    AZ_LIST_DESTROY(temp_doors);
  }
  {
    AZ_LIST_DECLARE(az_editor_node_t, temp_nodes);
    AZ_LIST_INIT(temp_nodes, 2);
    AZ_LIST_LOOP(node, room->nodes) {
      if (!node->selected) *AZ_LIST_ADD(temp_nodes) = *node;
      else state.unsaved = true;
    }
    AZ_LIST_SWAP(temp_nodes, room->nodes);
    AZ_LIST_DESTROY(temp_nodes);
  }
  {
    AZ_LIST_DECLARE(az_editor_wall_t, temp_walls);
    AZ_LIST_INIT(temp_walls, 2);
    AZ_LIST_LOOP(wall, room->walls) {
      if (!wall->selected) *AZ_LIST_ADD(temp_walls) = *wall;
      else state.unsaved = true;
    }
    AZ_LIST_SWAP(temp_walls, room->walls);
    AZ_LIST_DESTROY(temp_walls);
  }
}

static void do_change_data(int delta, bool secondary) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    const az_baddie_kind_t new_kind =
      az_modulo((int)baddie->spec.kind - 1 + delta, AZ_NUM_BADDIE_KINDS) + 1;
    baddie->spec.kind = new_kind;
    state.brush.baddie_kind = new_kind;
    state.unsaved = true;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    const az_door_kind_t new_kind =
      az_modulo((int)door->spec.kind - 1 + delta, AZ_NUM_DOOR_KINDS) + 1;
    door->spec.kind = new_kind;
    state.brush.door_kind = new_kind;
    state.unsaved = true;
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    if (node->spec.kind == AZ_NODE_UPGRADE && secondary) {
      const az_upgrade_t new_upgrade =
        az_modulo((int)node->spec.upgrade + delta, AZ_NUM_UPGRADES);
      node->spec.upgrade = new_upgrade;
      state.brush.upgrade_kind = new_upgrade;
    } else {
      const az_node_kind_t new_kind =
        az_modulo((int)node->spec.kind - 1 + delta, AZ_NUM_NODE_KINDS) + 1;
      node->spec.kind = new_kind;
      state.brush.node_kind = new_kind;
    }
    state.unsaved = true;
  }
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    if (secondary) {
      const az_wall_kind_t new_kind =
        az_modulo((int)wall->spec.kind - 1 + delta, AZ_NUM_WALL_KINDS) + 1;
      wall->spec.kind = new_kind;
      state.brush.wall_kind = new_kind;
    } else {
      const int old_index = az_wall_data_index(wall->spec.data);
      const int new_index = az_modulo(old_index + delta, AZ_NUM_WALL_DATAS);
      wall->spec.data = az_get_wall_data(new_index);
      state.brush.wall_data_index = new_index;
    }
    state.unsaved = true;
  }
}

static void begin_set_current_room(void) {
  assert(state.text.action == AZ_ETA_NOTHING);
  const int length =
    snprintf(state.text.buffer, AZ_ARRAY_SIZE(state.text.buffer),
             "%03d", state.current_room);
  state.text.length = az_imin(length, AZ_ARRAY_SIZE(state.text.buffer));
  state.text.action = AZ_ETA_SET_CURRENT_ROOM;
}

static void try_set_current_room(void) {
  assert(state.text.action == AZ_ETA_SET_CURRENT_ROOM);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  state.text.buffer[state.text.length] = '\0';
  az_room_key_t key;
  int count;
  if (sscanf(state.text.buffer, "%d%n", &key, &count) < 1) return;
  if (count != state.text.length) return;
  if (key < 0 || key >= AZ_LIST_SIZE(state.planet.rooms)) return;
  state.text.action = AZ_ETA_NOTHING;
  state.current_room = key;
  center_camera_on_current_room();
}

static void begin_set_door_dest(void) {
  assert(state.text.action == AZ_ETA_NOTHING);
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  bool any_doors = false;
  az_room_key_t key = 0;
  AZ_LIST_LOOP(door, room->doors) {
    if (door->selected) {
      any_doors = true;
      key = door->spec.destination;
      break;
    }
  }
  if (!any_doors) return;
  const int length =
    snprintf(state.text.buffer, AZ_ARRAY_SIZE(state.text.buffer), "%03d", key);
  state.text.length = az_imin(length, AZ_ARRAY_SIZE(state.text.buffer));
  state.text.action = AZ_ETA_SET_DOOR_DEST;
}

static void try_set_door_dest(void) {
  assert(state.text.action == AZ_ETA_SET_DOOR_DEST);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  state.text.buffer[state.text.length] = '\0';
  az_room_key_t key;
  int count;
  if (sscanf(state.text.buffer, "%d%n", &key, &count) < 1) return;
  if (count != state.text.length) return;
  if (key < 0 || key >= AZ_LIST_SIZE(state.planet.rooms)) return;
  state.text.action = AZ_ETA_NOTHING;
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    door->spec.destination = key;
    state.unsaved = true;
  }
}

static void event_loop(void) {
  while (true) {
    state.controls.up = az_is_key_held(AZ_KEY_UP_ARROW);
    state.controls.down = az_is_key_held(AZ_KEY_DOWN_ARROW);
    state.controls.left = az_is_key_held(AZ_KEY_LEFT_ARROW);
    state.controls.right = az_is_key_held(AZ_KEY_RIGHT_ARROW);

    az_tick_editor_state(&state);
    az_start_screen_redraw(); {
      az_editor_draw_screen(&state);
    } az_finish_screen_redraw();

    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          if (state.text.action != AZ_ETA_NOTHING) {
            if (event.key.name == AZ_KEY_RETURN) {
              switch (state.text.action) {
                case AZ_ETA_NOTHING: AZ_ASSERT_UNREACHABLE();
                case AZ_ETA_SET_CURRENT_ROOM: try_set_current_room(); break;
                case AZ_ETA_SET_DOOR_DEST: try_set_door_dest(); break;
              }
            } else if (event.key.name == AZ_KEY_ESCAPE) {
              state.text.action = AZ_ETA_NOTHING;
            } else if (event.key.name == AZ_KEY_BACKSPACE) {
              if (state.text.length > 0) {
                --state.text.length;
              }
            } else if (event.key.character >= ' ' &&
                       event.key.character < '\x7f') {
              if (state.text.length < AZ_ARRAY_SIZE(state.text.buffer) - 1) {
                state.text.buffer[state.text.length++] = event.key.character;
              }
            }
          } else {
            switch (event.key.name) {
              case AZ_KEY_1: state.zoom_level = 1.0; break;
              case AZ_KEY_2:
                state.zoom_level = (event.key.shift ? 0.7 : 1.4);
                break;
              case AZ_KEY_3:
                state.zoom_level = (event.key.shift ? 0.5 : 2.0);
                break;
              case AZ_KEY_4:
                state.zoom_level = (event.key.shift ? 0.35 : 2.8);
                break;
              case AZ_KEY_5:
                state.zoom_level = (event.key.shift ? 0.25 : 4.0);
                break;
              case AZ_KEY_6:
                state.zoom_level = (event.key.shift ? 0.18 : 5.6);
                break;
              case AZ_KEY_7:
                state.zoom_level = (event.key.shift ? 0.125 : 8.0);
                break;
              case AZ_KEY_8:
                state.zoom_level = (event.key.shift ? 0.0884 : 11.3);
                break;
              case AZ_KEY_9:
                state.zoom_level = (event.key.shift ? 0.0625 : 16.0);
                break;
              case AZ_KEY_0:
                state.zoom_level = (event.key.shift ? 0.0442 : 22.6);
                break;
              case AZ_KEY_A:
                if (event.key.command) {
                  select_all(AZ_LIST_GET(state.planet.rooms,
                                         state.current_room), true);
                }
                break;
              case AZ_KEY_B: state.tool = AZ_TOOL_BADDIE; break;
              case AZ_KEY_C: state.tool = AZ_TOOL_CAMERA; break;
              case AZ_KEY_D:
                if (event.key.shift) begin_set_door_dest();
                else state.tool = AZ_TOOL_DOOR;
                break;
              case AZ_KEY_M:
                if (event.key.shift) state.tool = AZ_TOOL_MASS_MOVE;
                else state.tool = AZ_TOOL_MOVE;
                break;
              case AZ_KEY_N:
                if (event.key.command) add_new_room();
                else state.tool = AZ_TOOL_NODE;
                break;
              case AZ_KEY_O: do_change_data(1, event.key.shift); break;
              case AZ_KEY_P: do_change_data(-1, event.key.shift); break;
              case AZ_KEY_R:
                if (event.key.command) begin_set_current_room();
                else if (event.key.shift) state.tool = AZ_TOOL_MASS_ROTATE;
                else state.tool = AZ_TOOL_ROTATE;
                break;
              case AZ_KEY_S:
                if (event.key.command) do_save();
                else state.spin_camera = !state.spin_camera;
                break;
              case AZ_KEY_W: state.tool = AZ_TOOL_WALL; break;
              case AZ_KEY_BACKSPACE: do_remove(); break;
              default: break;
            }
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          switch (state.tool) {
            case AZ_TOOL_MOVE:
            case AZ_TOOL_ROTATE:
              do_select(event.mouse.x, event.mouse.y, az_is_shift_key_held());
              break;
            case AZ_TOOL_CAMERA:
              do_set_camera_bounds(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_BADDIE:
              do_add_baddie(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_DOOR:
              do_add_door(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_NODE:
              do_add_node(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_WALL:
              do_add_wall(event.mouse.x, event.mouse.y);
              break;
            case AZ_TOOL_MASS_MOVE:
            case AZ_TOOL_MASS_ROTATE:
              break;
          }
          break;
        case AZ_EVENT_MOUSE_MOVE:
          if (event.mouse.pressed) {
            switch (state.tool) {
              case AZ_TOOL_MOVE:
              case AZ_TOOL_BADDIE:
              case AZ_TOOL_DOOR:
              case AZ_TOOL_NODE:
              case AZ_TOOL_WALL:
                do_move(event.mouse.x, event.mouse.y,
                        event.mouse.dx, event.mouse.dy);
                break;
              case AZ_TOOL_MASS_MOVE:
                do_mass_move(event.mouse.x, event.mouse.y,
                             event.mouse.dx, event.mouse.dy);
                break;
              case AZ_TOOL_ROTATE:
                do_rotate(event.mouse.x, event.mouse.y,
                          event.mouse.dx, event.mouse.dy);
                break;
              case AZ_TOOL_MASS_ROTATE:
                do_mass_rotate(event.mouse.x, event.mouse.y,
                               event.mouse.dx, event.mouse.dy);
                break;
              case AZ_TOOL_CAMERA:
                do_set_camera_bounds(event.mouse.x, event.mouse.y);
                break;
            }
          }
          break;
        default: break;
      }
    }
  }
}

static bool load_and_init_state(void) {
  state.spin_camera = true;
  state.zoom_level = 1.0;
  state.brush.baddie_kind = AZ_BAD_LUMP;
  state.brush.door_kind = AZ_DOOR_NORMAL;
  state.brush.node_kind = AZ_NODE_TRACTOR;
  state.brush.upgrade_kind = AZ_UPG_GUN_CHARGE;
  state.brush.wall_kind = AZ_WALL_INDESTRUCTIBLE;

  az_planet_t planet;
  if (!az_load_planet("data", &planet)) return false;

  state.current_room = state.planet.start_room = planet.start_room;
  state.planet.start_position = planet.start_position;
  state.planet.start_angle = planet.start_angle;
  AZ_LIST_INIT(state.planet.rooms, planet.num_rooms);
  for (az_room_key_t key = 0; key < planet.num_rooms; ++key) {
    const az_room_t *room = &planet.rooms[key];
    az_editor_room_t *eroom = AZ_LIST_ADD(state.planet.rooms);
    eroom->camera_bounds = room->camera_bounds;
    AZ_LIST_INIT(eroom->baddies, room->num_baddies);
    for (int i = 0; i < room->num_baddies; ++i) {
      az_editor_baddie_t *baddie = AZ_LIST_ADD(eroom->baddies);
      baddie->spec = room->baddies[i];
    }
    AZ_LIST_INIT(eroom->doors, room->num_doors);
    for (int i = 0; i < room->num_doors; ++i) {
      az_editor_door_t *door = AZ_LIST_ADD(eroom->doors);
      door->spec = room->doors[i];
    }
    AZ_LIST_INIT(eroom->nodes, room->num_nodes);
    for (int i = 0; i < room->num_nodes; ++i) {
      az_editor_node_t *node = AZ_LIST_ADD(eroom->nodes);
      node->spec = room->nodes[i];
    }
    AZ_LIST_INIT(eroom->walls, room->num_walls);
    for (int i = 0; i < room->num_walls; ++i) {
      az_editor_wall_t *wall = AZ_LIST_ADD(eroom->walls);
      wall->spec = room->walls[i];
    }
  }

  az_destroy_planet(&planet);

  center_camera_on_current_room();
  return true;
}

static void destroy_state(void) {
  AZ_LIST_LOOP(room, state.planet.rooms) {
    AZ_LIST_DESTROY(room->baddies);
    AZ_LIST_DESTROY(room->doors);
    AZ_LIST_DESTROY(room->walls);
  }
  AZ_LIST_DESTROY(state.planet.rooms);
}

int main(int argc, char **argv) {
  az_init_random();
  az_init_baddie_datas();
  az_init_wall_datas();
  az_register_gl_init_func(az_init_wall_drawing);
  az_init_gui(false, false);

  if (!load_and_init_state()) {
    printf("Failed to load scenario.\n");
    return EXIT_FAILURE;
  }
  event_loop();
  destroy_state();

  return EXIT_SUCCESS;
}

/*===========================================================================*/
