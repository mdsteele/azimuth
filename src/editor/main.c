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
#include "azimuth/state/script.h"
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
  assert(room->on_start == NULL);
  const double current_r = az_vnorm(state.camera);
  const double theta_span = 200.0 / (80.0 + current_r);
  room->camera_bounds = (az_camera_bounds_t){
    .min_r = current_r - 100.0,
    .r_span = 200.0,
    .min_theta = az_mod2pi(az_vtheta(state.camera) - 0.5 * theta_span),
    .theta_span = theta_span
  };
  state.current_room = room_key;
  state.unsaved = true;
}

static void select_all(az_editor_room_t *room, bool selected) {
  AZ_LIST_LOOP(baddie, room->baddies) baddie->selected = selected;
  AZ_LIST_LOOP(door, room->doors) door->selected = selected;
  AZ_LIST_LOOP(gravfield, room->gravfields) gravfield->selected = selected;
  AZ_LIST_LOOP(node, room->nodes) node->selected = selected;
  AZ_LIST_LOOP(wall, room->walls) wall->selected = selected;
}

static void do_select(int x, int y, bool multi) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  double best_dist = INFINITY;
  az_editor_gravfield_t *best_gravfield = NULL;
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    double dist = az_vdist(pt, gravfield->spec.position);
    if (dist >= best_dist) continue;
    const az_gravfield_t real_gravfield = {
      .kind = gravfield->spec.kind,
      .position = gravfield->spec.position,
      .angle = gravfield->spec.angle,
      .strength = gravfield->spec.strength,
      .size = gravfield->spec.size
    };
    if (az_point_within_gravfield(&real_gravfield, pt)) {
      best_dist = dist;
      best_gravfield = gravfield;
    }
  }
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
  } else if (best_gravfield != NULL) {
    if (multi) {
      best_gravfield->selected = !best_gravfield->selected;
    } else if (!best_gravfield->selected) {
      select_all(room, false);
      best_gravfield->selected = true;
    }
    state.brush.angle = best_gravfield->spec.angle;
    state.brush.gravfield_kind = best_gravfield->spec.kind;
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
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    gravfield->spec.position = az_vadd(gravfield->spec.position, delta);
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
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    gravfield->spec.position = az_vadd(gravfield->spec.position, delta);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    node->spec.position = az_vadd(node->spec.position, delta);
  }
  AZ_LIST_LOOP(wall, room->walls) {
    wall->spec.position = az_vadd(wall->spec.position, delta);
  }
  state.unsaved = true;
}

static void rotate_around(az_vector_t *vec, az_vector_t around, double theta) {
   *vec = az_vadd(around, az_vrotate(az_vsub(*vec, around), theta));
}

static void do_rotate(int x, int y, int dx, int dy) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_vector_t pt0 = az_pixel_to_position(&state, x - dx, y - dy);
  const az_vector_t pt1 = az_pixel_to_position(&state, x, y);
  az_vector_t center = {0, 0};
  int count = 0;
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    center = az_vadd(center, baddie->spec.position);
    ++count;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    center = az_vadd(center, door->spec.position);
    ++count;
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    center = az_vadd(center, gravfield->spec.position);
    ++count;
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    center = az_vadd(center, node->spec.position);
    ++count;
  }
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    center = az_vadd(center, wall->spec.position);
    ++count;
  }
  if (count <= 0) return;
  center = az_vdiv(center, count);
  const double dtheta =
    az_vtheta(az_vsub(pt1, center)) - az_vtheta(az_vsub(pt0, center));
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    rotate_around(&baddie->spec.position, center, dtheta);
    baddie->spec.angle = state.brush.angle =
      az_mod2pi(baddie->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    rotate_around(&door->spec.position, center, dtheta);
    door->spec.angle = state.brush.angle =
      az_mod2pi(door->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    rotate_around(&gravfield->spec.position, center, dtheta);
    gravfield->spec.angle = state.brush.angle =
      az_mod2pi(gravfield->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    rotate_around(&node->spec.position, center, dtheta);
    node->spec.angle = state.brush.angle =
      az_mod2pi(node->spec.angle + dtheta);
  }
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    rotate_around(&wall->spec.position, center, dtheta);
    wall->spec.angle = state.brush.angle =
      az_mod2pi(wall->spec.angle + dtheta);
  }
  state.unsaved = true;
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
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    gravfield->spec.position = az_vrotate(gravfield->spec.position, dtheta);
    gravfield->spec.angle = az_mod2pi(gravfield->spec.angle + dtheta);
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

static void do_add_gravfield(int x, int y) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  select_all(room, false);
  const az_vector_t pt = az_pixel_to_position(&state, x, y);
  az_editor_gravfield_t *gravfield = AZ_LIST_ADD(room->gravfields);
  gravfield->selected = true;
  gravfield->spec.kind = state.brush.gravfield_kind;
  gravfield->spec.position = pt;
  gravfield->spec.angle = state.brush.angle;
  gravfield->spec.strength = 100.0;
  gravfield->spec.size.trapezoid.front_offset = 0.0;
  gravfield->spec.size.trapezoid.front_semiwidth = 50.0;
  gravfield->spec.size.trapezoid.rear_semiwidth = 100.0;
  gravfield->spec.size.trapezoid.semilength = 100.0;
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
    AZ_LIST_DECLARE(az_editor_gravfield_t, temp_gravfields);
    AZ_LIST_INIT(temp_gravfields, 2);
    AZ_LIST_LOOP(gravfield, room->gravfields) {
      if (!gravfield->selected) *AZ_LIST_ADD(temp_gravfields) = *gravfield;
      else state.unsaved = true;
    }
    AZ_LIST_SWAP(temp_gravfields, room->gravfields);
    AZ_LIST_DESTROY(temp_gravfields);
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
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    const az_gravfield_kind_t new_kind =
      az_modulo((int)gravfield->spec.kind - 1 + delta,
                AZ_NUM_GRAVFIELD_KINDS) + 1;
    gravfield->spec.kind = new_kind;
    state.brush.gravfield_kind = new_kind;
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

static void begin_edit_gravfield(void) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  az_gravfield_spec_t *gravfield = NULL;
  AZ_LIST_LOOP(editor_gravfield, room->gravfields) {
    if (editor_gravfield->selected) {
      gravfield = &editor_gravfield->spec;
      break;
    }
  }
  if (gravfield == NULL) return;
  az_init_editor_text(
      &state, AZ_ETA_EDIT_GRAVFIELD, "s%.02f l%.02f o%.02f f%.02f r%.02f",
      gravfield->strength, gravfield->size.trapezoid.semilength,
      gravfield->size.trapezoid.front_offset,
      gravfield->size.trapezoid.front_semiwidth,
      gravfield->size.trapezoid.rear_semiwidth);
}

static void try_edit_gravfield(void) {
  assert(state.text.action == AZ_ETA_EDIT_GRAVFIELD);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  assert(state.text.buffer[state.text.length] == '\0');
  double strength, semilength, front_offset, front_semiwidth, rear_semiwidth;
  int count;
  if (sscanf(state.text.buffer, "s%lf l%lf o%lf f%lf r%lf%n", &strength,
             &semilength, &front_offset, &front_semiwidth, &rear_semiwidth,
             &count) < 5) return;
  if (count != state.text.length) return;
  if (strength == 0.0 || semilength <= 0.0 ||
      front_semiwidth < 0.0 || rear_semiwidth < 0.0) return;
  state.text.action = AZ_ETA_NOTHING;
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    gravfield->spec.strength = strength;
    gravfield->spec.size.trapezoid.front_offset = front_offset;
    gravfield->spec.size.trapezoid.front_semiwidth = front_semiwidth;
    gravfield->spec.size.trapezoid.rear_semiwidth = rear_semiwidth;
    gravfield->spec.size.trapezoid.semilength = semilength;
    state.unsaved = true;
  }
}

static void begin_edit_script(void) {
  assert(state.text.action == AZ_ETA_NOTHING);
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  const az_script_t *script = room->on_start;
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (baddie->selected) script = baddie->spec.on_kill;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (door->selected) script = door->spec.on_open;
  }
  if (script != NULL) {
    if (az_sprint_script(script, state.text.buffer,
                         AZ_ARRAY_SIZE(state.text.buffer))) {
      state.text.length = state.text.cursor = strlen(state.text.buffer);
      state.text.action = AZ_ETA_EDIT_SCRIPT;
    }
  } else {
    state.text.length = state.text.cursor = 0;
    state.text.buffer[0] = '\0';
    state.text.action = AZ_ETA_EDIT_SCRIPT;
  }
}

static void try_edit_script(void) {
  assert(state.text.action == AZ_ETA_EDIT_SCRIPT);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  assert(state.text.buffer[state.text.length] == '\0');
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  az_script_t *script = NULL;
  if (state.text.length > 0) {
    script = az_sscan_script(state.text.buffer, state.text.length);
    if (script == NULL) return;
  }
  az_script_t **dest = &room->on_start;
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (baddie->selected) dest = &baddie->spec.on_kill;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (door->selected) dest = &door->spec.on_open;
  }
  state.text.action = AZ_ETA_NOTHING;
  az_free_script(*dest);
  *dest = script;
  state.unsaved = true;
}

static void begin_set_current_room(void) {
  az_init_editor_text(&state, AZ_ETA_SET_CURRENT_ROOM, "%03d",
                      state.current_room);
}

static void try_set_current_room(void) {
  assert(state.text.action == AZ_ETA_SET_CURRENT_ROOM);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  assert(state.text.buffer[state.text.length] == '\0');
  az_room_key_t key;
  int count;
  if (sscanf(state.text.buffer, "%d%n", &key, &count) < 1) return;
  if (count != state.text.length) return;
  if (key < 0 || key >= AZ_LIST_SIZE(state.planet.rooms)) return;
  state.text.action = AZ_ETA_NOTHING;
  state.current_room = key;
  az_center_editor_camera_on_current_room(&state);
}

static void begin_set_door_dest(void) {
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
  az_init_editor_text(&state, AZ_ETA_SET_DOOR_DEST, "%03d", key);
}

static void try_set_door_dest(void) {
  assert(state.text.action == AZ_ETA_SET_DOOR_DEST);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  assert(state.text.buffer[state.text.length] == '\0');
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

static void begin_set_uuid_slot(void) {
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  int uuid_slot = -1;
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (baddie->selected) uuid_slot = baddie->spec.uuid_slot;
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (door->selected) uuid_slot = door->spec.uuid_slot;
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (gravfield->selected) uuid_slot = gravfield->spec.uuid_slot;
  }
  if (uuid_slot < 0) return;
  az_init_editor_text(&state, AZ_ETA_SET_UUID_SLOT, "%d", uuid_slot);
}

static void try_set_uuid_slot(void) {
  assert(state.text.action == AZ_ETA_SET_UUID_SLOT);
  assert(state.text.length < AZ_ARRAY_SIZE(state.text.buffer));
  assert(state.text.buffer[state.text.length] == '\0');
  int uuid_slot = 0;
  if (state.text.length > 0) {
    int count;
    if (sscanf(state.text.buffer, "%d%n", &uuid_slot, &count) < 1) return;
    if (count != state.text.length) return;
  }
  if (uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) return;
  az_editor_room_t *room = AZ_LIST_GET(state.planet.rooms, state.current_room);
  // Make sure this UUID slot isn't in use.
  if (uuid_slot != 0) {
    AZ_LIST_LOOP(baddie, room->baddies) {
      if (baddie->spec.uuid_slot == uuid_slot) return;
    }
    AZ_LIST_LOOP(door, room->doors) {
      if (door->spec.uuid_slot == uuid_slot) return;
    }
    AZ_LIST_LOOP(gravfield, room->gravfields) {
      if (gravfield->spec.uuid_slot == uuid_slot) return;
    }
  }
  state.text.action = AZ_ETA_NOTHING;
  state.unsaved = true;
  // Set the UUID slot for a single object.
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (baddie->selected) {
      baddie->spec.uuid_slot = uuid_slot;
      return;
    }
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (door->selected) {
      door->spec.uuid_slot = uuid_slot;
      return;
    }
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (gravfield->selected) {
      gravfield->spec.uuid_slot = uuid_slot;
      return;
    }
  }
}

static void do_save(void) {
  if (!az_save_editor_state(&state)) {
    printf("Failed to save scenario.\n");
  }
}

static void event_loop(void) {
  while (true) {
    if (state.text.action == AZ_ETA_NOTHING) {
      state.controls.up = az_is_key_held(AZ_KEY_UP_ARROW);
      state.controls.down = az_is_key_held(AZ_KEY_DOWN_ARROW);
      state.controls.left = az_is_key_held(AZ_KEY_LEFT_ARROW);
      state.controls.right = az_is_key_held(AZ_KEY_RIGHT_ARROW);
    } else {
      state.controls.up = state.controls.down = state.controls.left =
        state.controls.right = false;
    }

    az_tick_editor_state(&state, 1.0/60.0);
    az_start_screen_redraw(); {
      az_editor_draw_screen(&state);
    } az_finish_screen_redraw();

    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
        case AZ_EVENT_KEY_DOWN:
          // If we're editing text, handle key events specially.
          if (state.text.action != AZ_ETA_NOTHING) {
            if (event.key.character >= ' ' &&
                event.key.character < '\x7f') {
              if (state.text.length < AZ_ARRAY_SIZE(state.text.buffer) - 1) {
                memmove(state.text.buffer + state.text.cursor + 1,
                        state.text.buffer + state.text.cursor,
                        state.text.length - state.text.cursor + 1);
                ++state.text.length;
                state.text.buffer[state.text.cursor++] = event.key.character;
              }
              break;
            }
            switch (event.key.name) {
              case AZ_KEY_RETURN:
                switch (state.text.action) {
                  case AZ_ETA_NOTHING: AZ_ASSERT_UNREACHABLE();
                  case AZ_ETA_EDIT_GRAVFIELD: try_edit_gravfield(); break;
                  case AZ_ETA_EDIT_SCRIPT: try_edit_script(); break;
                  case AZ_ETA_SET_CURRENT_ROOM: try_set_current_room(); break;
                  case AZ_ETA_SET_DOOR_DEST: try_set_door_dest(); break;
                  case AZ_ETA_SET_UUID_SLOT: try_set_uuid_slot(); break;
                }
                break;
              case AZ_KEY_ESCAPE:
                state.text.action = AZ_ETA_NOTHING;
                break;
              case AZ_KEY_BACKSPACE:
                if (state.text.cursor > 0) {
                  memmove(state.text.buffer + state.text.cursor - 1,
                          state.text.buffer + state.text.cursor,
                          state.text.length - state.text.cursor + 1);
                  --state.text.cursor;
                  --state.text.length;
                }
                break;
              case AZ_KEY_LEFT_ARROW:
                state.text.cursor = az_imax(state.text.cursor - 1, 0);
                break;
              case AZ_KEY_RIGHT_ARROW:
                state.text.cursor = az_imin(state.text.cursor + 1,
                                            state.text.length);
                break;
              default: break;
            }
            break;
          }
          // Otherwise, we're not editing text, so handle key events normally.
          switch (event.key.name) {
            case AZ_KEY_1: state.zoom_level = 1.0; break;
            case AZ_KEY_2:
              state.zoom_level = pow(2, (event.key.shift ? -0.5 : 0.5));
              break;
            case AZ_KEY_3:
              state.zoom_level = pow(2, (event.key.shift ? -1 : 1));
              break;
            case AZ_KEY_4:
              state.zoom_level = pow(2, (event.key.shift ? -1.5 : 1.5));
              break;
            case AZ_KEY_5:
              state.zoom_level = pow(2, (event.key.shift ? -2 : 2));
              break;
            case AZ_KEY_6:
              state.zoom_level = pow(2, (event.key.shift ? -2.5 : 3));
              break;
            case AZ_KEY_7:
              state.zoom_level = pow(2, (event.key.shift ? -3 : 4));
              break;
            case AZ_KEY_8:
              state.zoom_level = pow(2, (event.key.shift ? -3.5 : 5));
              break;
            case AZ_KEY_9:
              state.zoom_level = pow(2, (event.key.shift ? -4 : 6));
              break;
            case AZ_KEY_0:
              state.zoom_level = pow(2, (event.key.shift ? -4.5 : 7));
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
              if (event.key.command) begin_set_door_dest();
              else state.tool = AZ_TOOL_DOOR;
              break;
            case AZ_KEY_E:
              if (event.key.command) begin_edit_gravfield();
              break;
            case AZ_KEY_G: state.tool = AZ_TOOL_GRAVFIELD; break;
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
            case AZ_KEY_T:
              if (event.key.command) begin_edit_script();
              break;
            case AZ_KEY_U:
              if (event.key.command) begin_set_uuid_slot();
              break;
            case AZ_KEY_W: state.tool = AZ_TOOL_WALL; break;
            case AZ_KEY_BACKSPACE: do_remove(); break;
            default: break;
          }
          break;
        case AZ_EVENT_MOUSE_DOWN:
          if (state.text.action != AZ_ETA_NOTHING) break;
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
            case AZ_TOOL_GRAVFIELD:
              do_add_gravfield(event.mouse.x, event.mouse.y);
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
          if (state.text.action != AZ_ETA_NOTHING) break;
          if (event.mouse.pressed) {
            switch (state.tool) {
              case AZ_TOOL_MOVE:
              case AZ_TOOL_BADDIE:
              case AZ_TOOL_DOOR:
              case AZ_TOOL_GRAVFIELD:
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

int main(int argc, char **argv) {
  az_init_random();
  az_init_baddie_datas();
  az_init_wall_datas();
  az_register_gl_init_func(az_init_wall_drawing);
  az_init_gui(false, false);

  if (!az_load_editor_state(&state)) {
    printf("Failed to load scenario.\n");
    return EXIT_FAILURE;
  }
  event_loop();
  az_destroy_editor_state(&state);

  return EXIT_SUCCESS;
}

/*===========================================================================*/
