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

#include "editor/state.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/state/camera.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/script.h"
#include "azimuth/util/vector.h"
#include "editor/list.h"

/*===========================================================================*/

#define SAVE_ALL_ROOMS false

static az_script_t *clone_script(const az_script_t *script) {
  if (script == NULL) return NULL;
  az_script_t *clone = AZ_ALLOC(1, az_script_t);
  clone->num_instructions = script->num_instructions;
  clone->instructions = AZ_ALLOC(clone->num_instructions, az_instruction_t);
  memcpy(clone->instructions, script->instructions,
         clone->num_instructions * sizeof(az_instruction_t));
  return clone;
}

void az_relabel_editor_room(az_editor_room_t *room) {
  room->label = AZ_ERL_NORMAL_ROOM;
  AZ_LIST_LOOP(node, room->nodes) {
    if (node->spec.kind == AZ_NODE_CONSOLE) {
      if (node->spec.subkind.console == AZ_CONS_COMM) {
        room->label = AZ_ERL_COMM_ROOM;
      } else if (node->spec.subkind.console == AZ_CONS_SAVE) {
        room->label = AZ_ERL_SAVE_ROOM;
      }
    } else if (node->spec.kind == AZ_NODE_UPGRADE) {
      room->label = AZ_ERL_UPGRADE_ROOM;
    }
  }
  if (room->on_start != NULL) {
    for (int i = 0; i < room->on_start->num_instructions; ++i) {
      if (room->on_start->instructions[i].opcode == AZ_OP_BOSS) {
        room->label = AZ_ERL_BOSS_ROOM;
        break;
      }
    }
  }
}

bool az_load_editor_state(az_editor_state_t *state) {
  state->spin_camera = true;
  state->zoom_level = 128.0;
  state->brush.baddie_kind = AZ_BAD_LUMP;
  state->brush.door_kind = AZ_DOOR_NORMAL;
  state->brush.gravfield_kind = AZ_GRAV_TRAPEZOID;
  state->brush.gravfield_strength = 100.0;
  state->brush.gravfield_size.trapezoid.front_offset = 0.0;
  state->brush.gravfield_size.trapezoid.front_semiwidth = 50.0;
  state->brush.gravfield_size.trapezoid.rear_semiwidth = 100.0;
  state->brush.gravfield_size.trapezoid.semilength = 100.0;
  state->brush.node_kind = AZ_NODE_TRACTOR;
  state->brush.doodad_kind = AZ_DOOD_WARNING_LIGHT;
  state->brush.upgrade_kind = AZ_UPG_GUN_CHARGE;
  state->brush.wall_kind = AZ_WALL_INDESTRUCTIBLE;

  az_planet_t planet;
  if (!az_load_planet("data", &planet)) return false;

  state->current_room = state->planet.start_room = planet.start_room;
  state->planet.start_position = planet.start_position;
  state->planet.start_angle = planet.start_angle;
  // Convert texts:
  AZ_LIST_INIT(state->planet.texts, planet.num_texts);
  for (int i = 0; i < planet.num_texts; ++i) {
    az_clone_text(&planet.texts[i], AZ_LIST_ADD(state->planet.texts));
  }
  // Convert zones:
  AZ_LIST_INIT(state->planet.zones, planet.num_zones);
  for (int i = 0; i < planet.num_zones; ++i) {
    az_zone_t *zone = AZ_LIST_ADD(state->planet.zones);
    *zone = planet.zones[i];
    zone->name = AZ_ALLOC(strlen(planet.zones[i].name + 1), char);
    strcpy(zone->name, planet.zones[i].name);
  }
  // Convert rooms:
  AZ_LIST_INIT(state->planet.rooms, planet.num_rooms);
  for (az_room_key_t key = 0; key < planet.num_rooms; ++key) {
    const az_room_t *room = &planet.rooms[key];
    az_editor_room_t *eroom = AZ_LIST_ADD(state->planet.rooms);
    eroom->zone_key = room->zone_key;
    eroom->properties = room->properties;
    eroom->camera_bounds = room->camera_bounds;
    eroom->on_start = clone_script(room->on_start);
    // Convert baddies:
    AZ_LIST_INIT(eroom->baddies, room->num_baddies);
    for (int i = 0; i < room->num_baddies; ++i) {
      az_editor_baddie_t *baddie = AZ_LIST_ADD(eroom->baddies);
      baddie->spec = room->baddies[i];
      baddie->spec.on_kill = clone_script(baddie->spec.on_kill);
    }
    // Convert doors:
    AZ_LIST_INIT(eroom->doors, room->num_doors);
    for (int i = 0; i < room->num_doors; ++i) {
      az_editor_door_t *door = AZ_LIST_ADD(eroom->doors);
      door->spec = room->doors[i];
      door->spec.on_open = clone_script(door->spec.on_open);
    }
    // Convert gravfields:
    AZ_LIST_INIT(eroom->gravfields, room->num_gravfields);
    for (int i = 0; i < room->num_gravfields; ++i) {
      az_editor_gravfield_t *gravfield = AZ_LIST_ADD(eroom->gravfields);
      gravfield->spec = room->gravfields[i];
    }
    // Convert nodes:
    AZ_LIST_INIT(eroom->nodes, room->num_nodes);
    for (int i = 0; i < room->num_nodes; ++i) {
      az_editor_node_t *node = AZ_LIST_ADD(eroom->nodes);
      node->spec = room->nodes[i];
      node->spec.on_use = clone_script(node->spec.on_use);
    }
    // Convert walls:
    AZ_LIST_INIT(eroom->walls, room->num_walls);
    for (int i = 0; i < room->num_walls; ++i) {
      az_editor_wall_t *wall = AZ_LIST_ADD(eroom->walls);
      wall->spec = room->walls[i];
    }
    az_relabel_editor_room(eroom);
  }

  az_destroy_planet(&planet);

  AZ_LIST_GET(state->planet.rooms, state->current_room)->selected = true;
  az_center_editor_camera_on_current_room(state);
  return true;
}

bool az_save_editor_state(az_editor_state_t *state) {
  assert(state != NULL);
  // Count unsaved rooms:
  int num_rooms_to_save = 0;
  AZ_LIST_LOOP(room, state->planet.rooms) {
    if (SAVE_ALL_ROOMS || room->unsaved) ++num_rooms_to_save;
  }
  // Convert planet:
  const int num_texts = AZ_LIST_SIZE(state->planet.texts);
  const int num_zones = AZ_LIST_SIZE(state->planet.zones);
  const int num_rooms = AZ_LIST_SIZE(state->planet.rooms);
  assert(num_rooms >= 0);
  az_planet_t planet = {
    .start_room = state->planet.start_room,
    .start_position = state->planet.start_position,
    .start_angle = state->planet.start_angle,
    .num_texts = num_texts,
    .texts = AZ_ALLOC(num_texts, az_text_t),
    .num_zones = num_zones,
    .zones = AZ_ALLOC(num_zones, az_zone_t),
    .num_rooms = num_rooms,
    .rooms = AZ_ALLOC(num_rooms, az_room_t)
  };
  // Convert texts:
  for (int i = 0; i < num_texts; ++i) {
    az_clone_text(AZ_LIST_GET(state->planet.texts, i), &planet.texts[i]);
  }
  // Convert zones:
  for (int i = 0; i < num_zones; ++i) {
    az_zone_t *zone = AZ_LIST_GET(state->planet.zones, i);
    planet.zones[i] = *zone;
    planet.zones[i].name = AZ_ALLOC(strlen(zone->name) + 1, char);
    strcpy(planet.zones[i].name, zone->name);
  }
  // Convert rooms:
  az_room_key_t *rooms_to_save = AZ_ALLOC(num_rooms_to_save, az_room_key_t);
  int num_rooms_to_save_so_far = 0;
  for (az_room_key_t key = 0; key < num_rooms; ++key) {
    az_editor_room_t *eroom = AZ_LIST_GET(state->planet.rooms, key);
    if (SAVE_ALL_ROOMS || eroom->unsaved) {
      assert(num_rooms_to_save_so_far < num_rooms_to_save);
      rooms_to_save[num_rooms_to_save_so_far] = key;
      ++num_rooms_to_save_so_far;
    }
    az_room_t *room = &planet.rooms[key];
    room->zone_key = eroom->zone_key;
    room->properties = eroom->properties;
    room->camera_bounds = eroom->camera_bounds;
    room->on_start = clone_script(eroom->on_start);
    // Convert baddies:
    room->num_baddies = AZ_LIST_SIZE(eroom->baddies);
    room->baddies = AZ_ALLOC(room->num_baddies, az_baddie_spec_t);
    for (int i = 0; i < room->num_baddies; ++i) {
      room->baddies[i] = AZ_LIST_GET(eroom->baddies, i)->spec;
      room->baddies[i].on_kill = clone_script(room->baddies[i].on_kill);
    }
    // Convert doors:
    room->num_doors = AZ_LIST_SIZE(eroom->doors);
    room->doors = AZ_ALLOC(room->num_doors, az_door_spec_t);
    for (int i = 0; i < room->num_doors; ++i) {
      room->doors[i] = AZ_LIST_GET(eroom->doors, i)->spec;
      room->doors[i].on_open = clone_script(room->doors[i].on_open);
    }
    // Convert gravfields:
    room->num_gravfields = AZ_LIST_SIZE(eroom->gravfields);
    room->gravfields = AZ_ALLOC(room->num_gravfields, az_gravfield_spec_t);
    for (int i = 0; i < room->num_gravfields; ++i) {
      room->gravfields[i] = AZ_LIST_GET(eroom->gravfields, i)->spec;
    }
    // Convert nodes:
    room->num_nodes = AZ_LIST_SIZE(eroom->nodes);
    room->nodes = AZ_ALLOC(room->num_nodes, az_node_spec_t);
    for (int i = 0; i < room->num_nodes; ++i) {
      room->nodes[i] = AZ_LIST_GET(eroom->nodes, i)->spec;
      room->nodes[i].on_use = clone_script(room->nodes[i].on_use);
    }
    // Convert walls:
    room->num_walls = AZ_LIST_SIZE(eroom->walls);
    room->walls = AZ_ALLOC(room->num_walls, az_wall_spec_t);
    for (int i = 0; i < room->num_walls; ++i) {
      room->walls[i] = AZ_LIST_GET(eroom->walls, i)->spec;
    }
  }
  assert(num_rooms_to_save_so_far == num_rooms_to_save);
  // Write to disk:
  const bool success =
    az_save_planet(&planet, "data", rooms_to_save, num_rooms_to_save);
  // Clean up:
  free(rooms_to_save);
  az_destroy_planet(&planet);
  if (success) {
    state->unsaved = false;
    AZ_LIST_LOOP(room, state->planet.rooms) room->unsaved = false;
  }
  return success;
}

/*===========================================================================*/

void az_tick_editor_state(az_editor_state_t *state, double time) {
  ++state->clock;
  state->total_time += time;
  const double scroll_speed = 300.0 * state->zoom_level * time;
  const double spin_radius = 80.0 * state->zoom_level;

  if (state->controls.up && !state->controls.down) {
    if (state->spin_camera) {
      state->camera = az_vadd(state->camera,
                              az_vpolar(scroll_speed,
                                        az_vtheta(state->camera)));
    } else {
      state->camera.y += scroll_speed;
    }
  }
  if (state->controls.down && !state->controls.up) {
    if (state->spin_camera) {
      if (az_vnorm(state->camera) <= scroll_speed) {
        state->camera = az_vpolar(0.0001, az_vtheta(state->camera));
      } else {
        state->camera = az_vsub(state->camera,
                                az_vpolar(scroll_speed,
                                          az_vtheta(state->camera)));
      }
    } else {
      state->camera.y -= scroll_speed;
    }
  }
  if (state->controls.left && !state->controls.right) {
    if (state->spin_camera) {
      state->camera = az_vrotate(state->camera, scroll_speed /
                                 (spin_radius + az_vnorm(state->camera)));
    } else {
      state->camera.x -= scroll_speed;
    }
  }
  if (state->controls.right && !state->controls.left) {
    if (state->spin_camera) {
      state->camera = az_vrotate(state->camera, -scroll_speed /
                                 (spin_radius + az_vnorm(state->camera)));
    } else {
      state->camera.x += scroll_speed;
    }
  }
}

void az_init_editor_text(
    az_editor_state_t *state, az_editor_text_action_t action,
    const char *format, ...) {
  assert(state->text.action == AZ_ETA_NOTHING);
  va_list args;
  va_start(args, format);
  const int length =
    vsnprintf(state->text.buffer, AZ_ARRAY_SIZE(state->text.buffer),
              format, args);
  va_end(args);
  state->text.length = state->text.cursor =
    az_imin(length, AZ_ARRAY_SIZE(state->text.buffer));
  state->text.action = action;
}

bool az_editor_is_in_minimap_mode(const az_editor_state_t *state) {
  return state->zoom_level >= 8.0;
}

void az_center_editor_camera_on_current_room(az_editor_state_t *state) {
  const az_camera_bounds_t *bounds =
    &AZ_LIST_GET(state->planet.rooms, state->current_room)->camera_bounds;
  state->camera = az_bounds_center(bounds);
}

void az_destroy_editor_state(az_editor_state_t *state) {
  AZ_LIST_LOOP(room, state->planet.rooms) {
    az_free_script(room->on_start);
    AZ_LIST_LOOP(baddie, room->baddies) az_free_script(baddie->spec.on_kill);
    AZ_LIST_DESTROY(room->baddies);
    AZ_LIST_LOOP(door, room->doors) az_free_script(door->spec.on_open);
    AZ_LIST_DESTROY(room->doors);
    AZ_LIST_DESTROY(room->gravfields);
    AZ_LIST_DESTROY(room->nodes);
    AZ_LIST_DESTROY(room->walls);
  }
  AZ_LIST_DESTROY(state->planet.rooms);
}

/*===========================================================================*/
