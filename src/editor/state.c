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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "azimuth/state/camera.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/script.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/string.h"
#include "azimuth/util/vector.h"
#include "editor/list.h"

/*===========================================================================*/

#define SAVE_ALL_ROOMS false

static const int wall_data_indices[] = {
  // Colony walls:
  0, 2, 1, 21,
  // Green metal:
  28, 29, 30,
  // Girders:
  35, 6, 20, 46, 51, // silver
  8, 7, // red
  50, 9, 10, // brown
  47, // blue
  100, // heavy red
  // Pipes:
  101, 102, 139, 140, 141, // blue
  32, 34, 33, 48, 63, // yellow
  38, 40, 39, 49, 64, // red
  // Brown/gray rocks:
  5, 11, 12, 13, 14, 41, // boulders
  36, 37, // stalactites
  // Shale rock:
  80, 81, 82, 83, // blue
  84, 85, 86, 87, // brown
  // Crystals:
  3, 4, // cyan crystals
  53, 52, // glowing hex crystals
  // Concrete:
  26, 27, 54, 55, 56,
  // Grass/dirt:
  61, 45, 59, 60, // dirt only
  62, 42, 43, 44, // with grass
  // Tree parts:
  67, 68, 69, 70, 71, 72,
  // Seashells:
  73, 76, 75, 77, 74, 78,
  // Water-smoothed boulders:
  88, 89,
  // Sand:
  90, 91, 92,
  // Green rocks:
  97, 98,
  // Ice:
  15, 16, 17, 57, 58, // boulders
  18, 19, // icicles
  99, // frozen layer of water
  // Snow walls:
  94, 93, 95, 96,
  // Cave walls:
  120, 119, 121, 122, 118,
  // Purple rocks:
  109, 110, // boulders
  111, 112, // stalactites
  // Parallelagram blocks:
  103, 104, 105, 106, 107, 108,
  // Green/purple hex blocks:
  24, 25, 65, 66,
  // Jungle grass walls:
  114, 115, 116, 117, 113, // green
  135, 136, 137, 138, 134, // orange
  // Red-glowing ash boulders:
  123, 124,
  // Volcanic rocks:
  128, 129, 130, // dark
  131, 132, 133, // light
  // Cinderbricks:
  22, 23, 31,
  // Green/gray metal blocks:
  125, 126, 127,
  149, 150, 151,
  // Silver bezel pipes and connectors:
  148, 79, 147, // pipes
  142, 143, 144, 145, 146 // connectors
};
static int reverse_wall_data_indices[AZ_ARRAY_SIZE(wall_data_indices)];

static az_baddie_kind_t baddie_kinds[] = {
  AZ_BAD_MARKER,
  // Turrets:
  AZ_BAD_NORMAL_TURRET,
  AZ_BAD_BROKEN_TURRET,
  AZ_BAD_ARMORED_TURRET,
  AZ_BAD_HEAVY_TURRET,
  AZ_BAD_BEAM_TURRET,
  AZ_BAD_ROCKET_TURRET,
  AZ_BAD_CRAWLING_TURRET,
  AZ_BAD_CRAWLING_MORTAR,
  AZ_BAD_SECURITY_DRONE,
  // Zippers:
  AZ_BAD_ZIPPER,
  AZ_BAD_ARMORED_ZIPPER,
  AZ_BAD_MINI_ARMORED_ZIPPER,
  AZ_BAD_FIRE_ZIPPER,
  AZ_BAD_SWITCHER,
  AZ_BAD_DRAGONFLY,
  AZ_BAD_HORNET,
  AZ_BAD_SUPER_HORNET,
  AZ_BAD_MOSQUITO,
  // Bats:
  AZ_BAD_CAVE_SWOOPER,
  AZ_BAD_ECHO_SWOOPER,
  AZ_BAD_DEMON_SWOOPER,
  // Crawlers:
  AZ_BAD_CAVE_CRAWLER,
  AZ_BAD_SPINED_CRAWLER,
  AZ_BAD_ICE_CRAWLER,
  AZ_BAD_JUNGLE_CRAWLER,
  AZ_BAD_FIRE_CRAWLER,
  // Machines:
  AZ_BAD_GUN_SENSOR,
  AZ_BAD_BOMB_SENSOR,
  AZ_BAD_BEAM_SENSOR,
  AZ_BAD_SENSOR_LASER,
  AZ_BAD_BEAM_SENSOR_INV,
  AZ_BAD_BOX,
  AZ_BAD_ARMORED_BOX,
  AZ_BAD_TRAPDOOR,
  AZ_BAD_BEAM_WALL,
  // Pistons:
  AZ_BAD_PISTON,
  AZ_BAD_ARMORED_PISTON,
  AZ_BAD_ARMORED_PISTON_EXT,
  AZ_BAD_INCORPOREAL_PISTON,
  AZ_BAD_INCORPOREAL_PISTON_EXT,
  // Vehicles:
  AZ_BAD_COPTER_HORZ,
  AZ_BAD_SMALL_TRUCK,
  AZ_BAD_COPTER_VERT,
  AZ_BAD_SMALL_AUV,
  // Traps:
  AZ_BAD_HEAT_RAY,
  AZ_BAD_DEATH_RAY,
  AZ_BAD_PROXY_MINE,
  AZ_BAD_NUCLEAR_MINE,
  AZ_BAD_CREEPY_EYE,
  // Bouncers:
  AZ_BAD_BOUNCER,
  AZ_BAD_BOUNCER_90,
  AZ_BAD_FAST_BOUNCER,
  // Spinies:
  AZ_BAD_SPINER,
  AZ_BAD_SPINE_MINE,
  AZ_BAD_SUPER_SPINER,
  AZ_BAD_URCHIN,
  // Plants:
  AZ_BAD_CHOMPER_PLANT,
  AZ_BAD_AQUATIC_CHOMPER,
  AZ_BAD_JUNGLE_CHOMPER,
  AZ_BAD_FIRE_CHOMPER,
  AZ_BAD_GRABBER_PLANT,
  // Mushrooms:
  AZ_BAD_MYCOFLAKKER,
  AZ_BAD_MYCOSTALKER,
  AZ_BAD_PYROFLAKKER,
  AZ_BAD_PYROSTALKER,
  // Nightbugs:
  AZ_BAD_NIGHTBUG,
  AZ_BAD_NIGHTSHADE,
  // Miscellaneous:
  AZ_BAD_LEAPER,
  AZ_BAD_SMALL_FISH,
  AZ_BAD_CLAM,
  AZ_BAD_FIREBALL_MINE,
  AZ_BAD_ATOM,
  AZ_BAD_SPARK,
  AZ_BAD_ERUPTION,
  // Oth:
  AZ_BAD_OTH_RAZOR,
  AZ_BAD_OTH_CRAB_1,
  AZ_BAD_OTH_ORB_1,
  AZ_BAD_OTH_CRAB_2,
  AZ_BAD_OTH_ORB_2,
  AZ_BAD_OTH_CRAWLER,
  // Bosses and minions:
  AZ_BAD_BOSS_DOOR,
  AZ_BAD_OTH_SNAPDRAGON,
  AZ_BAD_ROCKWYRM,
  AZ_BAD_WYRM_EGG,
  AZ_BAD_WYRMLING,
  AZ_BAD_OTH_GUNSHIP,
  AZ_BAD_FORCEFIEND,
  AZ_BAD_FORCE_EGG,
  AZ_BAD_FORCELING,
  AZ_BAD_KILOFUGE,
  AZ_BAD_ICE_CRYSTAL,
  AZ_BAD_GNAT,
  AZ_BAD_NOCTURNE,
  AZ_BAD_ZENITH_CORE,
  AZ_BAD_POP_OPEN_TURRET,
};
static int reverse_baddie_kinds[AZ_ARRAY_SIZE(baddie_kinds)];
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(baddie_kinds) == AZ_NUM_BADDIE_KINDS);

static bool reverse_indices_initialized = false;

static void init_reverse_indices(void) {
  if (reverse_indices_initialized) return;
  assert(AZ_ARRAY_SIZE(wall_data_indices) == AZ_NUM_WALL_DATAS);
  for (int i = 0; i < AZ_NUM_WALL_DATAS; ++i) {
    reverse_wall_data_indices[wall_data_indices[i]] = i;
  }
  for (int i = 0; i < AZ_NUM_BADDIE_KINDS; ++i) {
    reverse_baddie_kinds[baddie_kinds[i] - 1] = i;
  }
  reverse_indices_initialized = true;
}

/*===========================================================================*/

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
  init_reverse_indices();
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
    *AZ_LIST_ADD(state->planet.paragraphs) = az_strdup(planet.paragraphs[i]);
  }
  // Convert zones:
  AZ_LIST_INIT(state->planet.zones, planet.num_zones);
  for (int i = 0; i < planet.num_zones; ++i) {
    az_zone_t *zone = AZ_LIST_ADD(state->planet.zones);
    *zone = planet.zones[i];
    zone->name = az_strdup(planet.zones[i].name);
  }
  // Convert rooms:
  AZ_LIST_INIT(state->planet.rooms, planet.num_rooms);
  for (az_room_key_t key = 0; key < planet.num_rooms; ++key) {
    const az_room_t *room = &planet.rooms[key];
    az_editor_room_t *eroom = AZ_LIST_ADD(state->planet.rooms);
    eroom->zone_key = room->zone_key;
    eroom->properties = room->properties;
    eroom->marker_flag = room->marker_flag;
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
  // Print save rooms that don't have appropriate room scripts.
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    bool found_save_point = false;
    for (int i = 0; i < room->num_nodes; ++i) {
      const az_node_spec_t *node = &room->nodes[i];
      if (node->kind == AZ_NODE_CONSOLE &&
          node->subkind.console == AZ_CONS_SAVE) {
        if (found_save_point) {
          printf("\x1b[31mRoom %d: Multiple save points\x1b[m\n", room_index);
          linebreak = true;
        }
        found_save_point = true;
      }
    }
    if (!found_save_point) continue;
    bool found_msg0 = false, found_mus = false;
    if (room->on_start != NULL) {
      for (int i = 0; i < room->on_start->num_instructions; ++i) {
        const az_instruction_t ins = room->on_start->instructions[i];
        if (ins.opcode == AZ_OP_MSG && ins.immediate == 0) found_msg0 = true;
        if (ins.opcode == AZ_OP_MUS) found_mus = true;
      }
    }
    if (!found_msg0) {
      printf("\x1b[31mRoom %d: Save point without msg0\x1b[m\n", room_index);
      linebreak = true;
    }
    if (!found_mus) {
      printf("\x1b[31mRoom %d: Save point without music\x1b[m\n", room_index);
      linebreak = true;
    }
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print walls/nodes that have a duplicate wall/node right on top.
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    for (int i = 0; i < room->num_walls; ++i) {
      const az_wall_spec_t *wall = &room->walls[i];
      for (int j = i + 1; j < room->num_walls; ++j) {
        const az_wall_spec_t *other_wall = &room->walls[j];
        if (other_wall->data == wall->data &&
            az_vwithin(other_wall->position, wall->position, 10.0) &&
            fabs(az_mod2pi(other_wall->angle - wall->angle)) < AZ_DEG2RAD(1)) {
          printf("\x1b[31mRoom %d: duplicate wall at (%.02f, %.02f)\x1b[m\n",
                 room_index, wall->position.x, wall->position.y);
          linebreak = true;
        }
      }
    }
    for (int i = 0; i < room->num_nodes; ++i) {
      const az_node_spec_t *node = &room->nodes[i];
      if (node->kind != AZ_NODE_FAKE_WALL_FG &&
          node->kind != AZ_NODE_FAKE_WALL_BG) continue;
      for (int j = i + 1; j < room->num_nodes; ++j) {
        const az_node_spec_t *other_node = &room->nodes[j];
        if (other_node->kind == node->kind &&
            other_node->subkind.fake_wall == node->subkind.fake_wall &&
            az_vwithin(other_node->position, node->position, 10.0) &&
            fabs(az_mod2pi(other_node->angle - node->angle)) < AZ_DEG2RAD(1)) {
          printf("\x1b[31mRoom %d: duplicate node at (%.02f, %.02f)\x1b[m\n",
                 room_index, node->position.x, node->position.y);
          linebreak = true;
        }
      }
    }
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print liquid gravfields that aren't oriented correctly:
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    for (int i = 0; i < room->num_gravfields; ++i) {
      const az_gravfield_spec_t *gravfield = &room->gravfields[i];
      if (!az_is_liquid(gravfield->kind)) continue;
      const double expected =
        (az_vdot(az_vpolar(1, gravfield->angle), gravfield->position) >= 0 ?
         az_vtheta(gravfield->position) :
         az_vtheta(az_vneg(gravfield->position)));
      if (fabs(az_mod2pi(gravfield->angle - expected)) > 0.00001) {
        printf("\x1b[31mRoom %d: liquid at (%.02f, %.02f) not vertical\n"
               "  (%.06f instead of %.06f)\x1b[m\n",
               room_index, gravfield->position.x, gravfield->position.y,
               gravfield->angle, expected);
        linebreak = true;
      }
    }
  }
  if (linebreak) { printf("\n"); linebreak = false; }
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
            printf("\x1b[31mDuplicate upgrade %d (%s) in rooms: %d %d",
                   upg, az_upgrade_name(upg), first_room, room_index);
          } else printf(" %d", room_index);
          ++num_copies;
        }
      }
    }
    if (num_copies == 0) {
      printf("\x1b[31mMissing upgrade %d (%s)\x1b[m\n",
             upg, az_upgrade_name(upg));
    } else if (num_copies > 1) printf("\x1b[m\n");
    if (num_copies != 1) linebreak = true;
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print upgrades that don't play upgrade sound/music:
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    for (int i = 0; i < room->num_nodes; ++i) {
      const az_node_spec_t *node = &room->nodes[i];
      if (node->kind != AZ_NODE_UPGRADE) continue;
      bool found_sound = false;
      if (node->on_use != NULL) {
        for (int j = 0; j < node->on_use->num_instructions; ++j) {
          const az_instruction_t ins = node->on_use->instructions[j];
          if ((ins.opcode == AZ_OP_MUS && ins.immediate == 14) ||
              (ins.opcode == AZ_OP_SND && ins.immediate == 5)) {
            found_sound = true;
            break;
          }
        }
      }
      if (!found_sound) {
        printf("\x1b[31mRoom %d: Upgrade %d (%s) doesn't play upgrade"
               " sound/music\x1b[m\n", room_index, (int)node->subkind.upgrade,
               az_upgrade_name(node->subkind.upgrade));
        linebreak = true;
      }
    }
  }
  if (linebreak) { printf("\n"); linebreak = false; }
  // Print number of rooms in each zone (both total rooms in that zone, and
  // "populated" rooms in that zone, so we can tell how many more rooms need to
  // be fleshed out):
  printf("\x1b[33;1m        Zone  Rem  Pop  TTL  Frac"
         "   \x1b[31mRk \x1b[34mBm \x1b[36mSh \x1b[35mCp\x1b[m\n");
  int total_pop_rooms = 0;
  for (int zone_index = 0; zone_index < planet->num_zones; ++zone_index) {
    const az_zone_t *zone = &planet->zones[zone_index];
    int num_rooms = 0, num_pop_rooms = 0;
    int num_rockets = 0, num_bombs = 0, num_shields = 0, num_capacitors = 0;
    for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
      const az_room_t *room = &planet->rooms[room_index];
      if (room->zone_key != (az_zone_key_t)zone_index) continue;
      ++num_rooms;
      if (room->num_walls >= 8) {
        ++num_pop_rooms;
        ++total_pop_rooms;
      }
      for (int node_index = 0; node_index < room->num_nodes; ++node_index) {
        const az_node_spec_t *node = &room->nodes[node_index];
        if (node->kind != AZ_NODE_UPGRADE) continue;
        const az_upgrade_t upgrade = node->subkind.upgrade;
        if (upgrade >= AZ_UPG_ROCKET_AMMO_00 &&
            upgrade <= AZ_UPG_ROCKET_AMMO_MAX) ++num_rockets;
        if (upgrade >= AZ_UPG_BOMB_AMMO_00 &&
            upgrade <= AZ_UPG_BOMB_AMMO_MAX) ++num_bombs;
        if (upgrade >= AZ_UPG_SHIELD_BATTERY_00 &&
            upgrade <= AZ_UPG_SHIELD_BATTERY_MAX) ++num_shields;
        if (upgrade >= AZ_UPG_CAPACITOR_00 &&
            upgrade <= AZ_UPG_CAPACITOR_MAX) ++num_capacitors;
      }
    }
    printf("%12s  %3d  %3d  %3d  %3d%%   %2d %2d %2d %2d\n", zone->name,
           (num_rooms - num_pop_rooms), num_pop_rooms, num_rooms,
           (int)(100.0 * (double)num_pop_rooms / (double)num_rooms),
           num_rockets, num_bombs, num_shields, num_capacitors);
  }
  printf("\x1b[1m       TOTAL  %3d  %3d  %3d  %3d%%\x1b[m\n",
         (planet->num_rooms - total_pop_rooms), total_pop_rooms,
         planet->num_rooms,
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
    planet.paragraphs[i] =
      az_strdup(*AZ_LIST_GET(state->planet.paragraphs, i));
  }
  // Convert zones:
  for (int i = 0; i < num_zones; ++i) {
    az_zone_t *zone = AZ_LIST_GET(state->planet.zones, i);
    planet.zones[i] = *zone;
    planet.zones[i].name = az_strdup(zone->name);
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
      (AZ_ROOMF_HEATED | AZ_ROOMF_MARK_IF_CLR | AZ_ROOMF_MARK_IF_SET |
       AZ_ROOMF_UNMAPPED);
    room->marker_flag = eroom->marker_flag;
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

az_baddie_kind_t az_advance_baddie_kind(az_baddie_kind_t kind, int delta) {
  assert(reverse_indices_initialized);
  const int kind_index = (int)kind - 1;
  assert(0 <= kind_index && kind_index < AZ_NUM_BADDIE_KINDS);
  const int old = reverse_baddie_kinds[kind_index];
  return baddie_kinds[az_modulo(old + delta, AZ_NUM_BADDIE_KINDS)];
}

int az_advance_wall_data_index(int wall_data_index, int delta) {
  assert(reverse_indices_initialized);
  assert(0 <= wall_data_index && wall_data_index < AZ_NUM_WALL_DATAS);
  const int old = reverse_wall_data_indices[wall_data_index];
  return wall_data_indices[az_modulo(old + delta, AZ_NUM_WALL_DATAS)];
}

/*===========================================================================*/

bool az_circle_hits_editor_walls(
    const az_editor_state_t *state, double circle_radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *normal_out) {
  az_editor_room_t *room =
    AZ_LIST_GET(state->planet.rooms, state->current_room);
  bool hit_anything = false;
  AZ_LIST_LOOP(editor_wall, room->walls) {
    const az_wall_t real_wall = {
      .kind = editor_wall->spec.kind,
      .data = editor_wall->spec.data,
      .position = editor_wall->spec.position,
      .angle = editor_wall->spec.angle
    };
    az_vector_t pos;
    if (az_circle_hits_wall(&real_wall, circle_radius, start, delta,
                            &pos, normal_out)) {
      if (pos_out != NULL) *pos_out = pos;
      delta = az_vsub(pos, start);
      hit_anything = true;
    }
  }
  return hit_anything;
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
