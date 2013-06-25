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
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "editor/list.h"

/*===========================================================================*/

#define SAVE_ALL_ROOMS false

void az_relabel_editor_room(az_editor_room_t *room) {
  room->label = AZ_ERL_NORMAL_ROOM;
  AZ_LIST_LOOP(node, room->nodes) {
    if (node->spec.kind == AZ_NODE_CONSOLE) {
      switch (node->spec.subkind.console) {
        case AZ_CONS_COMM: room->label = AZ_ERL_COMM_ROOM; break;
        case AZ_CONS_REFILL: room->label = AZ_ERL_REFILL_ROOM; break;
        case AZ_CONS_SAVE: room->label = AZ_ERL_SAVE_ROOM; break;
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
  state->brush.baddie_kind = AZ_BAD_MARKER;
  state->brush.door_kind = AZ_DOOR_NORMAL;
  state->brush.gravfield_kind = AZ_GRAV_TRAPEZOID;
  state->brush.gravfield_strength = 100.0;
  state->brush.gravfield_size.trapezoid.front_offset = 0.0;
  state->brush.gravfield_size.trapezoid.front_semiwidth = 50.0;
  state->brush.gravfield_size.trapezoid.rear_semiwidth = 100.0;
  state->brush.gravfield_size.trapezoid.semilength = 100.0;
  state->brush.node_kind = AZ_NODE_TRACTOR;
  state->brush.console_kind = AZ_CONS_SAVE;
  state->brush.doodad_kind = AZ_DOOD_WARNING_LIGHT;
  state->brush.upgrade_kind = AZ_UPG_GUN_CHARGE;
  state->brush.wall_kind = AZ_WALL_INDESTRUCTIBLE;
  AZ_LIST_INIT(state->clipboard, 0);

  az_planet_t planet;
  if (!az_load_planet("data", &planet)) return false;

  state->current_room = state->planet.start_room = planet.start_room;
  state->planet.start_position = planet.start_position;
  state->planet.start_angle = planet.start_angle;
  // Convert paragraphs:
  AZ_LIST_INIT(state->planet.paragraphs, planet.num_paragraphs);
  for (int i = 0; i < planet.num_paragraphs; ++i) {
    *AZ_LIST_ADD(state->planet.paragraphs) = strdup(planet.paragraphs[i]);
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
    eroom->on_start = az_clone_script(room->on_start);
    // Convert baddies:
    AZ_LIST_INIT(eroom->baddies, room->num_baddies);
    for (int i = 0; i < room->num_baddies; ++i) {
      az_editor_baddie_t *baddie = AZ_LIST_ADD(eroom->baddies);
      baddie->spec = room->baddies[i];
      baddie->spec.on_kill = az_clone_script(baddie->spec.on_kill);
    }
    // Convert doors:
    AZ_LIST_INIT(eroom->doors, room->num_doors);
    for (int i = 0; i < room->num_doors; ++i) {
      az_editor_door_t *door = AZ_LIST_ADD(eroom->doors);
      door->spec = room->doors[i];
      door->spec.on_open = az_clone_script(door->spec.on_open);
    }
    // Convert gravfields:
    AZ_LIST_INIT(eroom->gravfields, room->num_gravfields);
    for (int i = 0; i < room->num_gravfields; ++i) {
      az_editor_gravfield_t *gravfield = AZ_LIST_ADD(eroom->gravfields);
      gravfield->spec = room->gravfields[i];
      gravfield->spec.on_enter = az_clone_script(gravfield->spec.on_enter);
    }
    // Convert nodes:
    AZ_LIST_INIT(eroom->nodes, room->num_nodes);
    for (int i = 0; i < room->num_nodes; ++i) {
      az_editor_node_t *node = AZ_LIST_ADD(eroom->nodes);
      node->spec = room->nodes[i];
      node->spec.on_use = az_clone_script(node->spec.on_use);
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

static void summarize_scenario(const az_planet_t *planet) {
  bool linebreak = false;
  printf("\n");
  // Print rooms numbers and destinations of doors that don't have a legitimate
  // matching exit door:
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    for (int i = 0; i < room->num_doors; ++i) {
      const az_door_spec_t *door = &room->doors[i];
      if (door->kind == AZ_DOOR_FORCEFIELD) continue;
      if (door->destination == room_index) {
        printf("\x1b[31m%d -> %d\x1b[m\n", room_index, room_index);
        linebreak = true;
      } else {
        assert(door->destination >= 0);
        assert(door->destination < planet->num_rooms);
        const az_room_t *dest = &planet->rooms[door->destination];
        bool has_exit = false;
        for (int j = 0; j < dest->num_doors; ++j) {
          const az_door_spec_t *other_door = &dest->doors[j];
          if (other_door->kind != AZ_DOOR_FORCEFIELD &&
              other_door->destination == room_index) {
            has_exit = true;
            break;
          }
        }
        if (!has_exit) {
          printf("\x1b[31m%d -> %d\x1b[m\n", room_index, door->destination);
          linebreak = true;
        }
      }
    }
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print names of upgrades that aren't in the scenario, or that are in the
  // scenario multiple times:
  for (int upg = 0; upg < AZ_NUM_UPGRADES; ++upg) {
    int num_copies = 0, first_room = 0;
    for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
      const az_room_t *room = &planet->rooms[room_index];
      for (int i = 0; i < room->num_nodes; ++i) {
        const az_node_spec_t *node = &room->nodes[i];
        if (node->kind == AZ_NODE_UPGRADE &&
            node->subkind.upgrade == (az_upgrade_t)upg) {
          if (num_copies == 0) first_room = room_index;
          else if (num_copies == 1) {
            printf("\x1b[31m(%d) %s: %d %d", upg, az_upgrade_name(upg),
                   first_room, room_index);
          } else printf(" %d", room_index);
          ++num_copies;
        }
      }
    }
    if (num_copies == 0) {
      printf("\x1b[34m(%d) %s\x1b[m\n", upg, az_upgrade_name(upg));
    } else if (num_copies > 1) printf("\x1b[m\n");
    if (num_copies != 1) linebreak = true;
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print number of rooms in each zone (both total rooms in that zone, and
  // "populated" rooms in that zone, so we can tell how many more rooms need to
  // be fleshed out):
  printf("\x1b[33;1m        Zone  Pop  TTL  Frac\x1b[m\n");
  int total_pop_rooms = 0;
  for (int zone_index = 0; zone_index < planet->num_zones; ++zone_index) {
    const az_zone_t *zone = &planet->zones[zone_index];
    int num_rooms = 0, num_pop_rooms = 0;
    for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
      const az_room_t *room = &planet->rooms[room_index];
      if (room->zone_key == (az_zone_key_t)zone_index) {
        ++num_rooms;
        if (room->num_walls >= 8) {
          ++num_pop_rooms;
          ++total_pop_rooms;
        }
      }
    }
    printf("%12s  %3d  %3d  %3d%%\n", zone->name, num_pop_rooms, num_rooms,
           (int)(100.0 * (double)num_pop_rooms / (double)num_rooms));
  }
  printf("\x1b[1m       TOTAL  %3d  %3d  %3d%%\x1b[m\n",
         total_pop_rooms, planet->num_rooms,
         (int)(100.0 * (double)total_pop_rooms / (double)planet->num_rooms));
}

bool az_save_editor_state(az_editor_state_t *state, bool summarize) {
  assert(state != NULL);
  // Count unsaved rooms:
  int num_rooms_to_save = 0;
  AZ_LIST_LOOP(room, state->planet.rooms) {
    if (SAVE_ALL_ROOMS || room->unsaved) ++num_rooms_to_save;
  }
  // Convert planet:
  const int num_paragraphs = AZ_LIST_SIZE(state->planet.paragraphs);
  const int num_zones = AZ_LIST_SIZE(state->planet.zones);
  const int num_rooms = AZ_LIST_SIZE(state->planet.rooms);
  assert(num_rooms >= 0);
  az_planet_t planet = {
    .start_room = state->planet.start_room,
    .start_position = state->planet.start_position,
    .start_angle = state->planet.start_angle,
    .num_paragraphs = num_paragraphs,
    .paragraphs = AZ_ALLOC(num_paragraphs, char*),
    .num_zones = num_zones,
    .zones = AZ_ALLOC(num_zones, az_zone_t),
    .num_rooms = num_rooms,
    .rooms = AZ_ALLOC(num_rooms, az_room_t)
  };
  // Convert paragraphs:
  for (int i = 0; i < num_paragraphs; ++i) {
    planet.paragraphs[i] = strdup(*AZ_LIST_GET(state->planet.paragraphs, i));
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
    room->properties = eroom->properties &
      (AZ_ROOMF_DARK | AZ_ROOMF_HEATED | AZ_ROOMF_UNMAPPED);
    room->camera_bounds = eroom->camera_bounds;
    room->on_start = az_clone_script(eroom->on_start);
    // Convert baddies:
    room->num_baddies = AZ_LIST_SIZE(eroom->baddies);
    room->baddies = AZ_ALLOC(room->num_baddies, az_baddie_spec_t);
    for (int i = 0; i < room->num_baddies; ++i) {
      room->baddies[i] = AZ_LIST_GET(eroom->baddies, i)->spec;
      room->baddies[i].on_kill = az_clone_script(room->baddies[i].on_kill);
    }
    // Convert doors:
    room->num_doors = AZ_LIST_SIZE(eroom->doors);
    room->doors = AZ_ALLOC(room->num_doors, az_door_spec_t);
    for (int i = 0; i < room->num_doors; ++i) {
      room->doors[i] = AZ_LIST_GET(eroom->doors, i)->spec;
      room->doors[i].on_open = az_clone_script(room->doors[i].on_open);
    }
    // Convert gravfields:
    room->num_gravfields = AZ_LIST_SIZE(eroom->gravfields);
    room->gravfields = AZ_ALLOC(room->num_gravfields, az_gravfield_spec_t);
    for (int i = 0; i < room->num_gravfields; ++i) {
      room->gravfields[i] = AZ_LIST_GET(eroom->gravfields, i)->spec;
      room->gravfields[i].on_enter =
        az_clone_script(room->gravfields[i].on_enter);
    }
    // Convert nodes:
    room->num_nodes = AZ_LIST_SIZE(eroom->nodes);
    room->nodes = AZ_ALLOC(room->num_nodes, az_node_spec_t);
    for (int i = 0; i < room->num_nodes; ++i) {
      room->nodes[i] = AZ_LIST_GET(eroom->nodes, i)->spec;
      room->nodes[i].on_use = az_clone_script(room->nodes[i].on_use);
    }
    // Convert walls:
    room->num_walls = AZ_LIST_SIZE(eroom->walls);
    room->walls = AZ_ALLOC(room->num_walls, az_wall_spec_t);
    for (int i = 0; i < room->num_walls; ++i) {
      room->walls[i] = AZ_LIST_GET(eroom->walls, i)->spec;
    }
  }
  assert(num_rooms_to_save_so_far == num_rooms_to_save);
  // Summarize:
  if (summarize) summarize_scenario(&planet);
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
      az_vpluseq(&state->camera, az_vwithlen(state->camera, scroll_speed));
    } else {
      state->camera.y += scroll_speed;
    }
  }
  if (state->controls.down && !state->controls.up) {
    if (state->spin_camera) {
      if (az_vnorm(state->camera) <= scroll_speed) {
        state->camera = az_vpolar(0.0001, az_vtheta(state->camera));
      } else {
        az_vpluseq(&state->camera, az_vwithlen(state->camera, -scroll_speed));
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

void az_clear_clipboard(az_editor_state_t *state) {
  AZ_LIST_LOOP(object, state->clipboard) {
    switch (object->type) {
      case AZ_EOBJ_NOTHING: break;
      case AZ_EOBJ_BADDIE:
        az_free_script(object->spec.baddie.on_kill);
        break;
      case AZ_EOBJ_DOOR:
        az_free_script(object->spec.door.on_open);
        break;
      case AZ_EOBJ_GRAVFIELD:
        az_free_script(object->spec.gravfield.on_enter);
        break;
      case AZ_EOBJ_NODE:
        az_free_script(object->spec.node.on_use);
        break;
      case AZ_EOBJ_WALL: break;
    }
  }
  AZ_LIST_DESTROY(state->clipboard);
  AZ_LIST_INIT(state->clipboard, 0);
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
  az_clear_clipboard(state);
  AZ_LIST_DESTROY(state->clipboard);
  AZ_LIST_LOOP(room, state->planet.rooms) {
    az_free_script(room->on_start);
    AZ_LIST_LOOP(baddie, room->baddies) az_free_script(baddie->spec.on_kill);
    AZ_LIST_DESTROY(room->baddies);
    AZ_LIST_LOOP(door, room->doors) az_free_script(door->spec.on_open);
    AZ_LIST_DESTROY(room->doors);
    AZ_LIST_LOOP(grav, room->gravfields) az_free_script(grav->spec.on_enter);
    AZ_LIST_DESTROY(room->gravfields);
    AZ_LIST_LOOP(node, room->nodes) az_free_script(node->spec.on_use);
    AZ_LIST_DESTROY(room->nodes);
    AZ_LIST_DESTROY(room->walls);
  }
  AZ_LIST_DESTROY(state->planet.rooms);
}

/*===========================================================================*/

bool az_editor_object_next(az_editor_object_t *object) {
  az_editor_room_t *room = object->room;
  ++object->index;
  switch (object->type) {
    case AZ_EOBJ_NOTHING:
      object->index = 0;
      object->type = AZ_EOBJ_BADDIE;
      // fallthrough
    case AZ_EOBJ_BADDIE:
      if (object->index < AZ_LIST_SIZE(room->baddies)) {
        az_editor_baddie_t *baddie = AZ_LIST_GET(room->baddies, object->index);
        object->selected = &baddie->selected;
        object->position = &baddie->spec.position;
        object->angle = &baddie->spec.angle;
        return true;
      }
      object->index = 0;
      object->type = AZ_EOBJ_DOOR;
      // fallthrough
    case AZ_EOBJ_DOOR:
      if (object->index < AZ_LIST_SIZE(room->doors)) {
        az_editor_door_t *door = AZ_LIST_GET(room->doors, object->index);
        object->selected = &door->selected;
        object->position = &door->spec.position;
        object->angle = &door->spec.angle;
        return true;
      }
      object->index = 0;
      object->type = AZ_EOBJ_GRAVFIELD;
      // fallthrough
    case AZ_EOBJ_GRAVFIELD:
      if (object->index < AZ_LIST_SIZE(room->gravfields)) {
        az_editor_gravfield_t *gravfield =
          AZ_LIST_GET(room->gravfields, object->index);
        object->selected = &gravfield->selected;
        object->position = &gravfield->spec.position;
        object->angle = &gravfield->spec.angle;
        return true;
      }
      object->index = 0;
      object->type = AZ_EOBJ_NODE;
      // fallthrough
    case AZ_EOBJ_NODE:
      if (object->index < AZ_LIST_SIZE(room->nodes)) {
        az_editor_node_t *node = AZ_LIST_GET(room->nodes, object->index);
        object->selected = &node->selected;
        object->position = &node->spec.position;
        object->angle = &node->spec.angle;
        return true;
      }
      object->index = 0;
      object->type = AZ_EOBJ_WALL;
      // fallthrough
    case AZ_EOBJ_WALL:
      if (object->index < AZ_LIST_SIZE(room->walls)) {
        az_editor_wall_t *wall = AZ_LIST_GET(room->walls, object->index);
        object->selected = &wall->selected;
        object->position = &wall->spec.position;
        object->angle = &wall->spec.angle;
        return true;
      }
      return false;
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
