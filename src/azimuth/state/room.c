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

#include "azimuth/state/room.h"

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "azimuth/constants.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

typedef struct {
  az_reader_t *reader;
  bool success;
  jmp_buf jump;
  int num_baddies, num_doors, num_gravfields, num_nodes, num_walls;
  az_room_t *room;
} az_load_room_t;

#ifdef NDEBUG
#define FAIL() longjmp(loader->jump, 1)
#else
#define FAIL() do{ \
    fprintf(stderr, "room.c: failure at line %d\n", __LINE__); \
    longjmp(loader->jump, 1); \
  } while (0)
#endif // NDEBUG

#define READ(...) do { \
    if (az_rscanf(loader->reader, __VA_ARGS__) < \
        AZ_COUNT_ARGS(__VA_ARGS__) - 1) { \
      FAIL(); \
    } \
  } while (false)

// Read the next non-whitespace character.  If it is '$', return true; if it is
// '!' or if we reach EOF, return false; otherwise, fail parsing.
static bool scan_to_script(az_load_room_t *loader) {
  char ch;
  if (az_rscanf(loader->reader, " %c", &ch) < 1) return false;
  else if (ch == '!') return false;
  else if (ch == '$') return true;
  else FAIL();
}

static az_script_t *maybe_parse_script(az_load_room_t *loader, char ch) {
  if (!scan_to_script(loader)) return NULL;
  if (az_rgetc(loader->reader) != ch) FAIL();
  if (az_rgetc(loader->reader) != ':') FAIL();
  az_script_t *script = az_read_script(loader->reader);
  if (script == NULL) FAIL();
  if (scan_to_script(loader)) FAIL();
  return script;
}

static void parse_room_header(az_load_room_t *loader) {
  int zone_index;
  unsigned int properties;
  READ("@R z%d p%u", &zone_index, &properties);
  if (zone_index < 0 || zone_index >= AZ_MAX_NUM_ZONES) FAIL();
  loader->room->zone_key = (az_zone_key_t)zone_index;
  loader->room->properties = (az_room_flags_t)properties;
  if (loader->room->properties & (AZ_ROOMF_MARK_IF_CLR |
                                  AZ_ROOMF_MARK_IF_SET)) {
    int marker_flag;
    READ("/%d", &marker_flag);
    if (marker_flag < 0 || marker_flag >= AZ_MAX_NUM_FLAGS) FAIL();
    loader->room->marker_flag = (az_flag_t)marker_flag;
  }
  int background_index;
  int num_baddies, num_doors, num_gravfields, num_nodes, num_walls;
  double min_r, r_span, min_theta, theta_span;
  READ(" k%d b%d d%d g%d n%d w%d\n  c(%lf,%lf,%lf,%lf)\n", &background_index,
       &num_baddies, &num_doors, &num_gravfields, &num_nodes, &num_walls,
       &min_r, &r_span, &min_theta, &theta_span);
  if (background_index < 0 || background_index >= AZ_NUM_BG_PATTERNS) FAIL();
  loader->room->background_pattern = (az_background_pattern_t)background_index;
  if (zone_index < 0 || zone_index >= AZ_MAX_NUM_ZONES) FAIL();
  loader->room->zone_key = (az_zone_key_t)zone_index;
  loader->room->properties = (az_room_flags_t)properties;
  if (min_r < 0.0 || r_span < 0.0 || theta_span < 0.0) FAIL();
  loader->room->camera_bounds.min_r = min_r;
  loader->room->camera_bounds.r_span = r_span;
  loader->room->camera_bounds.min_theta = min_theta;
  loader->room->camera_bounds.theta_span = theta_span;
  if (num_baddies < 0 || num_baddies > AZ_MAX_NUM_BADDIES) FAIL();
  loader->num_baddies = num_baddies;
  loader->room->num_baddies = 0;
  loader->room->baddies = AZ_ALLOC(num_baddies, az_baddie_spec_t);
  if (num_doors < 0 || num_doors > AZ_MAX_NUM_DOORS) FAIL();
  loader->num_doors = num_doors;
  loader->room->num_doors = 0;
  loader->room->doors = AZ_ALLOC(num_doors, az_door_spec_t);
  if (num_gravfields < 0 || num_gravfields > AZ_MAX_NUM_GRAVFIELDS) FAIL();
  loader->num_gravfields = num_gravfields;
  loader->room->num_gravfields = 0;
  loader->room->gravfields = AZ_ALLOC(num_gravfields, az_gravfield_spec_t);
  if (num_nodes < 0 || num_nodes > AZ_MAX_NUM_NODES) FAIL();
  loader->num_nodes = num_nodes;
  loader->room->num_nodes = 0;
  loader->room->nodes = AZ_ALLOC(num_nodes, az_node_spec_t);
  if (num_walls < 0 || num_walls > AZ_MAX_NUM_WALLS) FAIL();
  loader->num_walls = num_walls;
  loader->room->num_walls = 0;
  loader->room->walls = AZ_ALLOC(num_walls, az_wall_spec_t);
  loader->room->on_start = maybe_parse_script(loader, 's');
}

static void parse_baddie_directive(az_load_room_t *loader) {
  if (loader->room->num_baddies >= loader->num_baddies) FAIL();
  int kind, uuid_slot;
  double x, y, angle;
  READ("%d x%lf y%lf a%lf u%d", &kind, &x, &y, &angle, &uuid_slot);
  if (kind <= 0 || kind > AZ_NUM_BADDIE_KINDS ||
      uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) FAIL();
  az_baddie_spec_t *baddie = &loader->room->baddies[loader->room->num_baddies];
  baddie->kind = (az_baddie_kind_t)kind;
  baddie->position = (az_vector_t){x, y};
  baddie->angle = angle;
  baddie->uuid_slot = uuid_slot;
  for (int i = 0; i < AZ_ARRAY_SIZE(baddie->cargo_slots); ++i) {
    if (az_rpeek(loader->reader) != ':') break;
    az_rgetc(loader->reader);  // skip past the ':'
    int cargo_slot;
    READ("%d", &cargo_slot);
    if (cargo_slot < 0 || cargo_slot > AZ_NUM_UUID_SLOTS) FAIL();
    baddie->cargo_slots[i] = cargo_slot;
  }
  baddie->on_kill = maybe_parse_script(loader, 'k');
  ++loader->room->num_baddies;
}

static void parse_door_directive(az_load_room_t *loader) {
  if (loader->room->num_doors >= loader->num_doors) FAIL();
  int kind, destination, uuid_slot;
  double x, y, angle;
  READ("%d x%lf y%lf a%lf r%d u%d\n",
       &kind, &x, &y, &angle, &destination, &uuid_slot);
  if (kind <= 0 || kind > AZ_NUM_DOOR_KINDS ||
      destination < 0 || destination >= AZ_MAX_NUM_ROOMS ||
      uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) FAIL();
  az_door_spec_t *door = &loader->room->doors[loader->room->num_doors];
  door->kind = (az_door_kind_t)kind;
  door->position = (az_vector_t){x, y};
  door->angle = angle;
  door->destination = (az_room_key_t)destination;
  door->uuid_slot = uuid_slot;
  door->on_open = maybe_parse_script(loader, 'o');
  ++loader->room->num_doors;
}

static void parse_gravfield_directive(az_load_room_t *loader) {
  if (loader->room->num_gravfields >= loader->num_gravfields) FAIL();
  int kind, uuid_slot;
  double x, y, angle;
  READ("%d x%lf y%lf a%lf ", &kind, &x, &y, &angle);
  if (kind <= 0 || kind > AZ_NUM_GRAVFIELD_KINDS) FAIL();
  az_gravfield_spec_t *gravfield =
    &loader->room->gravfields[loader->room->num_gravfields];
  gravfield->kind = (az_gravfield_kind_t)kind;
  gravfield->position = (az_vector_t){x, y};
  gravfield->angle = angle;
  if (!az_is_liquid(gravfield->kind)) {
    double strength;
    READ("s%lf ", &strength);
    gravfield->strength = strength;
  } else gravfield->strength = 1.0;
  if (az_is_trapezoidal(gravfield->kind)) {
    double front_offset, front_semiwidth, rear_semiwidth, semilength;
    READ("o%lf f%lf r%lf l%lf", &front_offset, &front_semiwidth,
         &rear_semiwidth, &semilength);
    if (semilength <= 0.0 || front_semiwidth < 0.0 ||
        rear_semiwidth < 0.0) FAIL();
    gravfield->size.trapezoid.semilength = semilength;
    gravfield->size.trapezoid.front_offset = front_offset;
    gravfield->size.trapezoid.front_semiwidth = front_semiwidth;
    gravfield->size.trapezoid.rear_semiwidth = rear_semiwidth;
  } else {
    double sweep_degrees, inner_radius, thickness;
    READ("w%lf i%lf t%lf", &sweep_degrees, &inner_radius, &thickness);
    if (inner_radius < 0.0 || thickness <= 0.0) FAIL();
    gravfield->size.sector.sweep_degrees = sweep_degrees;
    gravfield->size.sector.inner_radius = inner_radius;
    gravfield->size.sector.thickness = thickness;
  }
  READ(" u%d\n", &uuid_slot);
  gravfield->uuid_slot = uuid_slot;
  gravfield->on_enter = maybe_parse_script(loader, 'e');
  ++loader->room->num_gravfields;
}

static void parse_node_directive(az_load_room_t *loader) {
  if (loader->room->num_nodes >= loader->num_nodes) FAIL();
  int kind;
  READ("%d", &kind);
  if (kind <= 0 || kind > AZ_NUM_NODE_KINDS) FAIL();
  az_room_t *room = loader->room;
  az_node_spec_t *node = &room->nodes[loader->room->num_nodes];
  node->kind = (az_node_kind_t)kind;
  if (node->kind != AZ_NODE_TRACTOR) {
    int subkind;
    READ("/%d", &subkind);
    switch (node->kind) {
      case AZ_NODE_NOTHING:
      case AZ_NODE_TRACTOR:
        AZ_ASSERT_UNREACHABLE();
      case AZ_NODE_CONSOLE:
        if (subkind < 0 || subkind >= AZ_NUM_CONSOLE_KINDS) FAIL();
        node->subkind.console = (az_console_kind_t)subkind;
        switch (node->subkind.console) {
          case AZ_CONS_COMM:   room->properties |= AZ_ROOMF_WITH_COMM;   break;
          case AZ_CONS_REFILL: room->properties |= AZ_ROOMF_WITH_REFILL; break;
          case AZ_CONS_SAVE:   room->properties |= AZ_ROOMF_WITH_SAVE;   break;
        }
        break;
      case AZ_NODE_UPGRADE:
        if (subkind < 0 || subkind >= AZ_NUM_UPGRADES) FAIL();
        node->subkind.upgrade = (az_upgrade_t)subkind;
        break;
      case AZ_NODE_DOODAD_FG:
      case AZ_NODE_DOODAD_BG:
        if (subkind < 0 || subkind >= AZ_NUM_DOODAD_KINDS) FAIL();
        node->subkind.doodad = (az_doodad_kind_t)subkind;
        break;
      case AZ_NODE_FAKE_WALL_FG:
      case AZ_NODE_FAKE_WALL_BG:
        if (subkind < 0 || subkind >= AZ_NUM_WALL_DATAS) FAIL();
        node->subkind.fake_wall = az_get_wall_data(subkind);
        break;
      case AZ_NODE_MARKER:
        node->subkind.marker = subkind;
        break;
      case AZ_NODE_SECRET:
        if (subkind < 0) FAIL();
        node->subkind.secret = (az_room_key_t)subkind;
        break;
    }
  }
  int uuid_slot;
  double x, y, angle;
  READ(" x%lf y%lf a%lf u%d\n", &x, &y, &angle, &uuid_slot);
  if (uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) FAIL();
  node->position = (az_vector_t){x, y};
  node->angle = angle;
  node->uuid_slot = uuid_slot;
  node->on_use = maybe_parse_script(loader, 'u');
  ++loader->room->num_nodes;
}

static void parse_wall_directive(az_load_room_t *loader) {
  if (loader->room->num_walls >= loader->num_walls) FAIL();
  int kind, data_index, uuid_slot;
  double x, y, angle;
  READ("%d d%d x%lf y%lf a%lf u%d\n",
       &kind, &data_index, &x, &y, &angle, &uuid_slot);
  if (kind <= 0 || kind > AZ_NUM_WALL_KINDS ||
      data_index < 0 || data_index >= AZ_NUM_WALL_DATAS ||
      uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) FAIL();
  az_wall_spec_t *wall = &loader->room->walls[loader->room->num_walls];
  wall->kind = (az_wall_kind_t)kind;
  wall->data = az_get_wall_data(data_index);
  wall->uuid_slot = uuid_slot;
  wall->position = (az_vector_t){x, y};
  wall->angle = angle;
  ++loader->room->num_walls;
  if (scan_to_script(loader)) FAIL();
}

static bool parse_directive(az_load_room_t *loader) {
  switch (az_rgetc(loader->reader)) {
    case 'B': parse_baddie_directive(loader); return true;
    case 'D': parse_door_directive(loader); return true;
    case 'G': parse_gravfield_directive(loader); return true;
    case 'N': parse_node_directive(loader); return true;
    case 'W': parse_wall_directive(loader); return true;
    case EOF: return false;
    default: FAIL();
  }
  AZ_ASSERT_UNREACHABLE();
}

static void validate_room(az_load_room_t *loader) {
  if (loader->room->num_baddies != loader->num_baddies ||
      loader->room->num_doors != loader->num_doors ||
      loader->room->num_gravfields != loader->num_gravfields ||
      loader->room->num_nodes != loader->num_nodes ||
      loader->room->num_walls != loader->num_walls) FAIL();
}

#undef READ
#undef FAIL

static void parse_room(az_load_room_t *loader) {
  if (setjmp(loader->jump) != 0) {
    az_destroy_room(loader->room);
    return;
  }
  parse_room_header(loader);
  while (parse_directive(loader));
  validate_room(loader);
  loader->success = true;
}

bool az_load_room_from_path(const char *filepath, az_room_t *room_out) {
  az_reader_t reader;
  if (!az_file_reader(filepath, &reader)) return false;
  const bool success = az_read_room(&reader, room_out);
  az_rclose(&reader);
  return success;
}

bool az_read_room(az_reader_t *reader, az_room_t *room_out) {
  assert(room_out != NULL);
  AZ_ZERO_OBJECT(room_out);
  az_load_room_t loader = {
    .reader = reader, .room = room_out, .success = false
  };
  parse_room(&loader);
  return loader.success;
}

/*===========================================================================*/

#define WRITE(...) do { \
    if (!az_wprintf(writer, __VA_ARGS__)) return false; \
  } while (false)

static bool try_write_script(char ch, const az_script_t *script,
                             az_writer_t *writer) {
  if (script != NULL) {
    WRITE("$%c:", ch);
    if (!az_write_script(script, writer)) return false;
    WRITE("\n");
  }
  return true;
}

#define WRITE_SCRIPT(ch, script) do { \
    if (!try_write_script(ch, script, writer)) return false; \
  } while (false)

bool az_write_room(const az_room_t *room, az_writer_t *writer) {
  WRITE("@R z%d p%u", (int)room->zone_key, (unsigned int)room->properties);
  if (room->properties & (AZ_ROOMF_MARK_IF_CLR | AZ_ROOMF_MARK_IF_SET)) {
    WRITE("/%d", (int)room->marker_flag);
  }
  WRITE(" k%d b%d d%d g%d n%d w%d\n  c(%.02f,%.02f,%f,%f)\n",
        room->background_pattern, room->num_baddies, room->num_doors,
        room->num_gravfields, room->num_nodes, room->num_walls,
        room->camera_bounds.min_r, room->camera_bounds.r_span,
        room->camera_bounds.min_theta, room->camera_bounds.theta_span);
  WRITE_SCRIPT('s', room->on_start);
  for (int i = 0; i < room->num_walls; ++i) {
    const az_wall_spec_t *wall = &room->walls[i];
    WRITE("!W%d d%d x%.02f y%.02f a%f u%d\n", (int)wall->kind,
          az_wall_data_index(wall->data), wall->position.x, wall->position.y,
          wall->angle, wall->uuid_slot);
  }
  for (int i = 0; i < room->num_doors; ++i) {
    const az_door_spec_t *door = &room->doors[i];
    WRITE("!D%d x%.02f y%.02f a%f r%d u%d\n", (int)door->kind,
          door->position.x, door->position.y, door->angle, door->destination,
          door->uuid_slot);
    WRITE_SCRIPT('o', door->on_open);
  }
  for (int i = 0; i < room->num_gravfields; ++i) {
    const az_gravfield_spec_t *gravfield = &room->gravfields[i];
    WRITE("!G%d x%.02f y%.02f a%f ",
          (int)gravfield->kind, gravfield->position.x, gravfield->position.y,
          gravfield->angle);
    if (!az_is_liquid(gravfield->kind)) {
      WRITE("s%.02f ", gravfield->strength);
    }
    if (az_is_trapezoidal(gravfield->kind)) {
      WRITE("o%.02f f%.02f r%.02f l%.02f",
            gravfield->size.trapezoid.front_offset,
            gravfield->size.trapezoid.front_semiwidth,
            gravfield->size.trapezoid.rear_semiwidth,
            gravfield->size.trapezoid.semilength);
    } else {
      WRITE("w%.02f i%.02f t%.02f", gravfield->size.sector.sweep_degrees,
            gravfield->size.sector.inner_radius,
            gravfield->size.sector.thickness);
    }
    WRITE(" u%d\n", gravfield->uuid_slot);
    WRITE_SCRIPT('e', gravfield->on_enter);
  }
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *node = &room->nodes[i];
    WRITE("!N%d", (int)node->kind);
    if (node->kind != AZ_NODE_TRACTOR) {
      int subkind = 0;
      switch (node->kind) {
        case AZ_NODE_NOTHING:
        case AZ_NODE_TRACTOR:
          AZ_ASSERT_UNREACHABLE();
        case AZ_NODE_CONSOLE:
          subkind = (int)node->subkind.console;
          break;
        case AZ_NODE_UPGRADE:
          subkind = (int)node->subkind.upgrade;
          break;
        case AZ_NODE_DOODAD_FG:
        case AZ_NODE_DOODAD_BG:
          subkind = (int)node->subkind.doodad;
          break;
        case AZ_NODE_FAKE_WALL_FG:
        case AZ_NODE_FAKE_WALL_BG:
          subkind = az_wall_data_index(node->subkind.fake_wall);
          break;
        case AZ_NODE_MARKER:
          subkind = node->subkind.marker;
          break;
        case AZ_NODE_SECRET:
          subkind = (int)node->subkind.secret;
          break;
      }
      WRITE("/%d", subkind);
    }
    WRITE(" x%.02f y%.02f a%f u%d\n",
          node->position.x, node->position.y, node->angle, node->uuid_slot);
    WRITE_SCRIPT('u', node->on_use);
  }
  for (int i = 0; i < room->num_baddies; ++i) {
    const az_baddie_spec_t *baddie = &room->baddies[i];
    WRITE("!B%d x%.02f y%.02f a%f u%d", (int)baddie->kind,
          baddie->position.x, baddie->position.y, baddie->angle,
          baddie->uuid_slot);
    AZ_ARRAY_LOOP(slot, baddie->cargo_slots) {
      if (*slot != 0) WRITE(":%d", *slot);
    }
    WRITE("\n");
    WRITE_SCRIPT('k', baddie->on_kill);
  }
  return true;
}

#undef WRITE
#undef WRITE_SCRIPT

bool az_save_room_to_path(const az_room_t *room, const char *filepath) {
  az_writer_t writer;
  if (!az_file_writer(filepath, &writer)) return false;
  const bool ok = az_write_room(room, &writer);
  az_wclose(&writer);
  return ok;
}

/*===========================================================================*/

void az_destroy_room(az_room_t *room) {
  assert(room != NULL);
  az_free_script(room->on_start);
  free(room->baddies);
  free(room->doors);
  free(room->nodes);
  free(room->walls);
  AZ_ZERO_OBJECT(room);
}

az_vector_t az_room_center(const az_room_t *room) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  return (bounds->theta_span >= 6.28 && bounds->min_r < AZ_SCREEN_HEIGHT ?
          AZ_VZERO : az_bounds_center(bounds));
}

bool az_test_room_mapped(const az_player_t *player, az_room_key_t room_key,
                         const az_room_t *room) {
  return (az_test_room_visited(player, room_key) ||
          (!(room->properties & AZ_ROOMF_UNMAPPED) &&
           az_test_zone_mapped(player, room->zone_key)));
}

bool az_should_mark_room(const az_player_t *player, az_room_key_t room_key,
                         const az_room_t *room) {
  return (((room->properties & AZ_ROOMF_MARK_IF_SET) &&
           az_test_flag(player, room->marker_flag)) ||
          ((room->properties & AZ_ROOMF_MARK_IF_CLR) &&
           !az_test_flag(player, room->marker_flag) &&
           az_test_room_mapped(player, room_key, room)));
}

/*===========================================================================*/
