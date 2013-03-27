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
#include <string.h>

#include "azimuth/constants.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

// TODO: move these elsewhere
#define AZ_MAX_NUM_BADDIES 192
#define AZ_MAX_NUM_DOORS 20
#define AZ_MAX_NUM_GRAVFIELDS 50
#define AZ_MAX_NUM_NODES 50
#define AZ_MAX_NUM_WALLS 250

typedef struct {
  FILE *file;
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
    if (fscanf(loader->file, __VA_ARGS__) < AZ_COUNT_ARGS(__VA_ARGS__) - 1) { \
      FAIL(); \
    } \
  } while (false)

// Read the next non-whitespace character.  If it is '$', return true; if it is
// '!' or if we reach EOF, return false; otherwise, fail parsing.
static bool scan_to_script(az_load_room_t *loader) {
  char ch;
  if (fscanf(loader->file, " %c", &ch) < 1) return false;
  else if (ch == '!') return false;
  else if (ch == '$') return true;
  else FAIL();
}

static az_script_t *maybe_parse_script(az_load_room_t *loader, char ch) {
  if (!scan_to_script(loader)) return NULL;
  if (fgetc(loader->file) != ch) FAIL();
  if (fgetc(loader->file) != ':') FAIL();
  az_script_t *script = az_fscan_script(loader->file);
  if (script == NULL) FAIL();
  if (scan_to_script(loader)) FAIL();
  return script;
}

static void parse_room_header(az_load_room_t *loader) {
  int zone_index;
  unsigned int properties;
  double min_r, r_span, min_theta, theta_span;
  int num_baddies, num_doors, num_gravfields, num_nodes, num_walls;
  READ("@R z%d p%u b%d d%d g%d n%d w%d\n  c(%lf,%lf,%lf,%lf)\n",
       &zone_index, &properties, &num_baddies, &num_doors, &num_gravfields,
       &num_nodes, &num_walls, &min_r, &r_span, &min_theta, &theta_span);
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
  READ("%d x%lf y%lf a%lf u%d\n", &kind, &x, &y, &angle, &uuid_slot);
  if (kind <= 0 || kind > AZ_NUM_BADDIE_KINDS ||
      uuid_slot < 0 || uuid_slot > AZ_NUM_UUID_SLOTS) FAIL();
  az_baddie_spec_t *baddie = &loader->room->baddies[loader->room->num_baddies];
  baddie->kind = (az_baddie_kind_t)kind;
  baddie->position = (az_vector_t){x, y};
  baddie->angle = angle;
  baddie->uuid_slot = uuid_slot;
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
  double x, y, angle, strength;
  READ("%d x%lf y%lf a%lf s%lf ", &kind, &x, &y, &angle, &strength);
  if (kind <= 0 || kind > AZ_NUM_GRAVFIELD_KINDS || strength == 0.0) FAIL();
  az_gravfield_spec_t *gravfield =
    &loader->room->gravfields[loader->room->num_gravfields];
  gravfield->kind = (az_gravfield_kind_t)kind;
  gravfield->position = (az_vector_t){x, y};
  gravfield->angle = angle;
  gravfield->strength = strength;
  if (az_trapezoidal(gravfield->kind)) {
    double front_offset, front_semiwidth, rear_semiwidth, semilength;
    READ("o%lf f%lf r%lf l%lf", &front_offset, &front_semiwidth,
         &rear_semiwidth, &semilength);
    if (semilength <= 0.0) FAIL();
    gravfield->size.trapezoid.semilength = semilength;
    gravfield->size.trapezoid.front_offset = front_offset;
    gravfield->size.trapezoid.front_semiwidth = front_semiwidth;
    gravfield->size.trapezoid.rear_semiwidth = rear_semiwidth;
  } else {
    double sweep_degrees, inner_radius, thickness;
    READ("w%lf i%lf t%lf", &sweep_degrees, &inner_radius, &thickness);
    gravfield->size.sector.sweep_degrees = sweep_degrees;
    gravfield->size.sector.inner_radius = inner_radius;
    gravfield->size.sector.thickness = thickness;
  }
  READ(" u%d\n", &uuid_slot);
  gravfield->uuid_slot = uuid_slot;
  ++loader->room->num_gravfields;
  if (scan_to_script(loader)) FAIL();
}

static void parse_node_directive(az_load_room_t *loader) {
  if (loader->room->num_nodes >= loader->num_nodes) FAIL();
  int kind;
  READ("%d", &kind);
  if (kind <= 0 || kind > AZ_NUM_NODE_KINDS) FAIL();
  az_node_spec_t *node = &loader->room->nodes[loader->room->num_nodes];
  if (kind != AZ_NODE_TRACTOR) {
    int subkind;
    READ("/%d", &subkind);
    if (kind == AZ_NODE_CONSOLE) {
      if (subkind < 0 || subkind >= AZ_NUM_CONSOLE_KINDS) FAIL();
      node->subkind.console = (az_console_kind_t)subkind;
    } else if (kind == AZ_NODE_UPGRADE) {
      if (subkind < 0 || subkind >= AZ_NUM_UPGRADES) FAIL();
      node->subkind.upgrade = (az_upgrade_t)subkind;
    } else if (kind == AZ_NODE_DOODAD_FG || kind == AZ_NODE_DOODAD_BG) {
      if (subkind < 0 || subkind >= AZ_NUM_DOODAD_KINDS) FAIL();
      node->subkind.doodad = (az_doodad_kind_t)subkind;
    } else {
      assert(kind == AZ_NODE_FAKE_WALL_FG || kind == AZ_NODE_FAKE_WALL_BG);
      if (subkind < 0 || subkind >= AZ_NUM_WALL_DATAS) FAIL();
      node->subkind.fake_wall = az_get_wall_data(subkind);
    }
  }
  double x, y, angle;
  READ(" x%lf y%lf a%lf", &x, &y, &angle);
  node->kind = (az_node_kind_t)kind;
  node->position = (az_vector_t){x, y};
  node->angle = angle;
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
  switch (fgetc(loader->file)) {
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

bool az_load_room_from_file(const char *filepath, az_room_t *room_out) {
  assert(room_out != NULL);
  memset(room_out, 0, sizeof(*room_out));
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  az_load_room_t loader = {.file = file, .room = room_out, .success = false};
  parse_room(&loader);
  fclose(file);
  return loader.success;
}

/*===========================================================================*/

#define WRITE(...) do { \
    if (fprintf(file, __VA_ARGS__) < 0) return false; \
  } while (false)

static bool try_write_script(char ch, const az_script_t *script, FILE *file) {
  if (script != NULL) {
    WRITE("$%c:", ch);
    if (!az_fprint_script(script, file)) return false;
    WRITE("\n");
  }
  return true;
}

#define WRITE_SCRIPT(ch, script) do { \
    if (!try_write_script(ch, script, file)) return false; \
  } while (false)

static bool write_room(const az_room_t *room, FILE *file) {
  WRITE("@R z%d p%u b%d d%d g%d n%d w%d\n  c(%.02f,%.02f,%f,%f)\n",
        (int)room->zone_key, (unsigned int)room->properties,
        room->num_baddies, room->num_doors, room->num_gravfields,
        room->num_nodes, room->num_walls, room->camera_bounds.min_r,
        room->camera_bounds.r_span, room->camera_bounds.min_theta,
        room->camera_bounds.theta_span);
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
    WRITE("!G%d x%.02f y%.02f a%f s%.02f ",
          (int)gravfield->kind, gravfield->position.x, gravfield->position.y,
          gravfield->angle, gravfield->strength);
    if (az_trapezoidal(gravfield->kind)) {
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
  }
  for (int i = 0; i < room->num_nodes; ++i) {
    const az_node_spec_t *node = &room->nodes[i];
    WRITE("!N%d", (int)node->kind);
    if (node->kind == AZ_NODE_CONSOLE) {
      WRITE("/%d", (int)node->subkind.console);
    } else if (node->kind == AZ_NODE_UPGRADE) {
      WRITE("/%d", (int)node->subkind.upgrade);
    } else if (node->kind == AZ_NODE_DOODAD_FG ||
               node->kind == AZ_NODE_DOODAD_BG) {
      WRITE("/%d", (int)node->subkind.doodad);
    } else if (node->kind == AZ_NODE_FAKE_WALL_FG ||
               node->kind == AZ_NODE_FAKE_WALL_BG) {
      WRITE("/%d", az_wall_data_index(node->subkind.fake_wall));
    }
    WRITE(" x%.02f y%.02f a%f\n",
          node->position.x, node->position.y, node->angle);
    WRITE_SCRIPT('u', node->on_use);
  }
  for (int i = 0; i < room->num_baddies; ++i) {
    const az_baddie_spec_t *baddie = &room->baddies[i];
    WRITE("!B%d x%.02f y%.02f a%f u%d\n", (int)baddie->kind,
          baddie->position.x, baddie->position.y, baddie->angle,
          baddie->uuid_slot);
    WRITE_SCRIPT('k', baddie->on_kill);
  }
  return true;
}

#undef WRITE
#undef WRITE_SCRIPT

bool az_save_room_to_file(const az_room_t *room, const char *filepath) {
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = write_room(room, file);
  fclose(file);
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
  memset(room, 0, sizeof(*room));
}

/*===========================================================================*/
