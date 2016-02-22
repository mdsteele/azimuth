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

#include "editor/audit.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/script.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static int count_consoles(const az_room_t *room, az_console_kind_t *kind_out) {
  int num_consoles = 0;
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *node = &room->nodes[i];
    if (node->kind == AZ_NODE_CONSOLE) {
      ++num_consoles;
      *kind_out = node->subkind.console;
    }
  }
  return num_consoles;
}

static int room_contains_upgrade(const az_room_t *room, az_upgrade_t upgrade) {
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *node = &room->nodes[i];
    if (node->kind == AZ_NODE_UPGRADE &&
        node->subkind.upgrade == upgrade) {
      return true;
    }
  }
  return false;
}

static bool room_has_door_to(const az_room_t *room, int dest_index) {
  for (int i = 0; i < room->num_doors; ++i) {
    const az_door_spec_t *door = &room->doors[i];
    if (door->kind != AZ_DOOR_FORCEFIELD &&
        door->destination == dest_index) {
      return true;
    }
  }
  return false;
}

static bool script_contains_opcode(const az_script_t *script,
                                   az_opcode_t opcode) {
  if (script == NULL) return false;
  for (int i = 0; i < script->num_instructions; ++i) {
    if (script->instructions[i].opcode == opcode) return true;
  }
  return false;
}

static bool script_contains_instruction(const az_script_t *script,
                                        az_opcode_t opcode, double immediate) {
  if (script == NULL) return false;
  for (int i = 0; i < script->num_instructions; ++i) {
    const az_instruction_t ins = script->instructions[i];
    if (ins.opcode == opcode && ins.immediate == immediate) return true;
  }
  return false;
}

/*===========================================================================*/

#define GENERAL_ERROR(...) do { \
    printf("\x1b[31m"); \
    printf(__VA_ARGS__); \
    printf("\x1b[m\n"); \
    ok = false; \
  } while (0)

#define ROOM_ERROR(...) do { \
    printf("\x1b[31mRoom %d: ", room_index); \
    printf(__VA_ARGS__); \
    printf("\x1b[m\n"); \
    ok = false; \
  } while (0)

static bool audit_script(int room_index, const az_script_t *script) {
  if (script == NULL) return true;
  bool ok = true;
  const bool has_dlog = script_contains_opcode(script, AZ_OP_DLOG);
  const bool has_mlog = script_contains_opcode(script, AZ_OP_MLOG);
  const bool has_skip1 = script_contains_instruction(script, AZ_OP_SKIP, 1);
  // Check that dlog opcode is accompanied by dend.
  if (has_dlog && !script_contains_opcode(script, AZ_OP_DEND)) {
    ROOM_ERROR("Script has dlog opcode, but no dend");
  }
  // Check that mlog opcode is accompanied by mend.
  if (has_mlog && !script_contains_opcode(script, AZ_OP_MEND)) {
    ROOM_ERROR("Script has mlog opcode, but no mend");
  }
  // Check that dlog/mlog/cutscenes are accompanied by skip1.
  if ((has_dlog || has_mlog || script_contains_opcode(script, AZ_OP_SCENE) ||
       script_contains_opcode(script, AZ_OP_SCTXT)) &&
      !has_skip1) {
    ROOM_ERROR("Cutscene without skip1 opcode");
  }
  // Check that skip1 instruction is accompanied by skip0.
  if (has_skip1 && !script_contains_instruction(script, AZ_OP_SKIP, 0)) {
    ROOM_ERROR("Script has skip1 instruction, but no skip0");
  }
  return ok;
}

#define CHECK_SCRIPT(script) do { \
    if (!audit_script(room_index, (script))) ok = false; \
  } while (0)

bool az_audit_scenario(const az_planet_t *planet) {
  bool ok = audit_script(-1, planet->on_start);
  bool upgrade_exists[AZ_NUM_UPGRADES] = {false};
  for (int room_index = 0; room_index < planet->num_rooms; ++room_index) {
    const az_room_t *room = &planet->rooms[room_index];
    // Check background pattern.
    if (room->background_pattern == AZ_BG_SOLID_BLACK) {
      ROOM_ERROR("No background pattern");
    }
    // Check on_start script.
    CHECK_SCRIPT(room->on_start);
    // Check consoles.
    az_console_kind_t console_kind;
    const int num_consoles = count_consoles(room, &console_kind);
    if (num_consoles > 1) ROOM_ERROR("Multiple consoles");
    if (num_consoles > 0) {
      if (console_kind == AZ_CONS_SAVE) {
        if (!script_contains_instruction(room->on_start, AZ_OP_MSG, 0)) {
          ROOM_ERROR("Save point without msg0");
        }
        if (!script_contains_opcode(room->on_start, AZ_OP_MUS)) {
          ROOM_ERROR("Save point without music");
        }
      } else if (console_kind == AZ_CONS_REFILL) {
        if (!script_contains_instruction(room->on_start, AZ_OP_MSG, 1)) {
          ROOM_ERROR("Repair bay without msg1");
        }
      }
    }
    // Check walls.
    for (int i = 0; i < room->num_walls; ++i) {
      const az_wall_spec_t *wall = &room->walls[i];
      // Icicles should always be charge-destructible.
      const int wall_data_index = az_wall_data_index(wall->data);
      if ((wall_data_index == 18 || wall_data_index == 19) &&
          wall->kind != AZ_WALL_DESTRUCTIBLE_CHARGED) {
        ROOM_ERROR("Non-charge-destructible icicle at (%.02f, %.02f)",
                   wall->position.x, wall->position.y);
      }
      // Check for duplicate walls.
      for (int j = i + 1; j < room->num_walls; ++j) {
        const az_wall_spec_t *other_wall = &room->walls[j];
        if (other_wall->data == wall->data &&
            az_vwithin(other_wall->position, wall->position, 10.0) &&
            fabs(az_mod2pi(other_wall->angle - wall->angle)) < AZ_DEG2RAD(1)) {
          ROOM_ERROR("Duplicate wall at (%.02f, %.02f)",
                     wall->position.x, wall->position.y);
        }
      }
    }
    // Check for duplicate fake-wall nodes.
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
          ROOM_ERROR("Duplicate node at (%.02f, %.02f)",
                     node->position.x, node->position.y);
        }
      }
    }
    // Check baddies.
    for (int i = 0; i < room->num_baddies; ++i) {
      const az_baddie_spec_t *baddie = &room->baddies[i];
      // Check on_kill script.
      CHECK_SCRIPT(baddie->on_kill);
    }
    // Check doors.
    for (int i = 0; i < room->num_doors; ++i) {
      const az_door_spec_t *door = &room->doors[i];
      // Check that forcefield doors don't have scripts.
      if (door->kind == AZ_DOOR_FORCEFIELD) {
        if (door->on_open != NULL) {
          ROOM_ERROR("Forcefield door with an on_open script");
        }
        continue;
      }
      // Check on_open script.
      CHECK_SCRIPT(door->on_open);
      // Check that door destination is legitimate.
      if (door->destination == room_index) {
        ROOM_ERROR("Door at (%.02f, %.02f) leads to itself",
                   door->position.x, door->position.y);
        continue;
      }
      // Check that destination room has a door leading back here.
      const int dest_index = door->destination;
      assert(dest_index >= 0);
      assert(dest_index < planet->num_rooms);
      const az_room_t *dest_room = &planet->rooms[door->destination];
      if (!room_has_door_to(dest_room, room_index)) {
        ROOM_ERROR("Door to room %d doesn't have an exit", dest_index);
      }
      // Check that if this room is next to another zone, it sets the music.
      if (dest_room->zone_key != room->zone_key &&
          !script_contains_opcode(room->on_start, AZ_OP_MUS)) {
        ROOM_ERROR("Next to another zone, but doesn't set music");
      }
      if (door->kind == AZ_DOOR_ROCKET || door->kind == AZ_DOOR_HYPER_ROCKET ||
          door->kind == AZ_DOOR_BOMB || door->kind == AZ_DOOR_MEGA_BOMB) {
        // Check that each ordnance door has a UUID.
        if (door->uuid_slot == 0) {
          ROOM_ERROR("Door at (%.02f, %.02f) doesn't have a UUID",
                     door->position.x, door->position.y);
        } else {
          // Check that opening an ordnance door sets a flag.
          if (!script_contains_opcode(door->on_open, AZ_OP_SET)) {
            ROOM_ERROR("Door with UUID %d doesn't set a flag when opened",
                       door->uuid_slot);
          }
          // Check that on_start script can unlock the door.
          if (!script_contains_instruction(room->on_start, AZ_OP_UNLOCK,
                                           door->uuid_slot)) {
            ROOM_ERROR("Door with UUID %d doesn't get unlocked by on_start",
                       door->uuid_slot);
          }
        }
      }
    }
    // Check gravfields.
    for (int i = 0; i < room->num_gravfields; ++i) {
      const az_gravfield_spec_t *gravfield = &room->gravfields[i];
      // Check on_enter script.
      CHECK_SCRIPT(gravfield->on_enter);
      // Check that liquids are vertical.
      if (az_is_liquid(gravfield->kind)) {
        const double expected_angle =
          (az_vdot(az_vpolar(1, gravfield->angle), gravfield->position) >= 0 ?
           az_vtheta(gravfield->position) :
           az_vtheta(az_vneg(gravfield->position)));
        if (fabs(az_mod2pi(gravfield->angle - expected_angle)) > 0.00001) {
          ROOM_ERROR("Liquid at (%.02f, %.02f) not vertical",
                     gravfield->position.x, gravfield->position.y);
        }
      }
    }
    // Check nodes.
    for (int i = 0; i < room->num_nodes; ++i) {
      const az_node_spec_t *node = &room->nodes[i];
      // Check the node's on_use script.
      if (node->kind == AZ_NODE_CONSOLE || node->kind == AZ_NODE_UPGRADE) {
        CHECK_SCRIPT(node->on_use);
      } else if (node->on_use != NULL) {
        ROOM_ERROR("Node kind %d with an on_use script", (int)node->kind);
      }
      // Check that comm consoles have dialogue.
      if (node->kind == AZ_NODE_CONSOLE &&
          node->subkind.console == AZ_CONS_COMM &&
          !script_contains_opcode(node->on_use, AZ_OP_DLOG)) {
        ROOM_ERROR("Comm console without dialogue");
      }
      // Check upgrades.
      if (node->kind == AZ_NODE_UPGRADE) {
        const az_upgrade_t upgrade = node->subkind.upgrade;
        assert((int)upgrade >= 0 && upgrade < AZ_ARRAY_SIZE(upgrade_exists));
        upgrade_exists[upgrade] = true;
        // Check that getting the upgrade plays mus14 or snd5.
        if (!script_contains_instruction(node->on_use, AZ_OP_MUS, 14) &&
            !script_contains_instruction(node->on_use, AZ_OP_SND, 5)) {
          ROOM_ERROR("Upgrade #%d (%s) doesn't play mus14 or snd5",
                     (int)upgrade, az_upgrade_name(upgrade));
        }
        // Check for duplicate upgrades.
        for (int other_room_index = room_index + 1;
             other_room_index < planet->num_rooms; ++other_room_index) {
          const az_room_t *other_room = &planet->rooms[other_room_index];
          if (room_contains_upgrade(other_room, upgrade)) {
            ROOM_ERROR("Upgrade #%d (%s) also appears in room %d",
                       (int)upgrade, az_upgrade_name(upgrade),
                       other_room_index);
          }
        }
      }
    }
  }
  // Check that all upgrades exist.
  for (int i = 0; i < AZ_ARRAY_SIZE(upgrade_exists); ++i) {
    if (!upgrade_exists[i]) {
      GENERAL_ERROR("Missing upgrade #%d (%s)", i,
                    az_upgrade_name((az_upgrade_t)i));
    }
  }
  return ok;
}

/*===========================================================================*/
