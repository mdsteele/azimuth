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
#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h" // for AZ_ALLOC

/*===========================================================================*/

// TODO: move these elsewhere
#define AZ_MAX_NUM_BADDIES 192
#define AZ_MAX_NUM_DOORS 20
#define AZ_MAX_NUM_WALLS 250

typedef struct {
  FILE *file;
  az_room_t *room;
  bool success;
  jmp_buf jump;
} az_load_room_t;

#define FAIL() longjmp(loader->jump, 1)

static void parse_room_header(az_load_room_t *loader) {
  int room_num, num_baddies, num_doors, num_walls;
  double min_r, r_span, min_theta, theta_span;
  if (fscanf(loader->file, "!R%d c(%lf,%lf,%lf,%lf) b%d d%d w%d\n",
             &room_num, &min_r, &r_span, &min_theta, &theta_span,
             &num_baddies, &num_doors, &num_walls) < 8) FAIL();
  if (room_num < 0 || room_num >= AZ_MAX_NUM_ROOMS) FAIL();
  loader->room->key = room_num;
  if (min_r < 0.0 || r_span < 0.0 || theta_span < 0.0) FAIL();
  loader->room->camera_bounds.min_r = min_r;
  loader->room->camera_bounds.r_span = r_span;
  loader->room->camera_bounds.min_theta = min_theta;
  loader->room->camera_bounds.theta_span = theta_span;
  if (num_baddies < 0 || num_baddies > AZ_MAX_NUM_BADDIES) FAIL();
  loader->room->max_num_baddies = num_baddies;
  loader->room->baddies = AZ_ALLOC(num_baddies, az_baddie_spec_t);
  if (num_doors < 0 || num_doors > AZ_MAX_NUM_DOORS) FAIL();
  loader->room->max_num_doors = num_doors;
  loader->room->doors = AZ_ALLOC(num_doors, az_door_spec_t);
  if (num_walls < 0 || num_walls > AZ_MAX_NUM_WALLS) FAIL();
  loader->room->max_num_walls = num_walls;
  loader->room->walls = AZ_ALLOC(num_walls, az_wall_t);
}

static void parse_baddie_directive(az_load_room_t *loader) {
  if (loader->room->num_baddies >= loader->room->max_num_baddies) FAIL();
  int index;
  double x, y, angle;
  if (fscanf(loader->file, "%d x%lf y%lf a%lf\n",
             &index, &x, &y, &angle) < 4) FAIL();
  if (index <= 0 || index > AZ_NUM_BADDIE_KINDS) FAIL();
  az_baddie_spec_t *baddie = &loader->room->baddies[loader->room->num_baddies];
  baddie->kind = (az_baddie_kind_t)index;
  baddie->position = (az_vector_t){x, y};
  baddie->angle = angle;
  ++loader->room->num_baddies;
}

static void parse_door_directive(az_load_room_t *loader) {
  if (loader->room->num_doors >= loader->room->max_num_doors) FAIL();
  int index, destination;
  double x, y, angle;
  if (fscanf(loader->file, "%d x%lf y%lf a%lf r%d\n",
             &index, &x, &y, &angle, &destination) < 5) FAIL();
  if (index <= 0 || index > AZ_NUM_DOOR_KINDS) FAIL();
  if (destination < 0 || destination >= AZ_MAX_NUM_ROOMS) FAIL();
  az_door_spec_t *door = &loader->room->doors[loader->room->num_doors];
  door->kind = (az_door_kind_t)index;
  door->position = (az_vector_t){x, y};
  door->angle = angle;
  door->destination = (az_room_key_t)destination;
  ++loader->room->num_doors;
}

static void parse_wall_directive(az_load_room_t *loader) {
  if (loader->room->num_walls >= loader->room->max_num_walls) FAIL();
  int index;
  double x, y, angle;
  if (fscanf(loader->file, "%d x%lf y%lf a%lf\n",
             &index, &x, &y, &angle) < 4) FAIL();
  if (index < 0 || index >= AZ_NUM_WALL_DATAS) FAIL();
  az_wall_t *wall = &loader->room->walls[loader->room->num_walls];
  wall->kind = AZ_WALL_NORMAL;
  wall->data = az_get_wall_data(index);
  wall->position = (az_vector_t){x, y};
  wall->angle = angle;
  ++loader->room->num_walls;
}

static bool parse_directive(az_load_room_t *loader) {
  switch (fgetc(loader->file)) {
    case 'B': parse_baddie_directive(loader); return true;
    case 'D': parse_door_directive(loader); return true;
    case 'W': parse_wall_directive(loader); return true;
    case EOF: return false;
    default: FAIL();
  }
  assert(false); // unreachable
}

static void validate_room(az_load_room_t *loader) {
  if (loader->room->num_baddies != loader->room->max_num_baddies) FAIL();
  if (loader->room->num_walls != loader->room->max_num_walls) FAIL();
}

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
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  az_load_room_t loader = { .file = file, .room = room_out, .success = false};
  parse_room(&loader);
  fclose(file);
  return loader.success;
}

/*===========================================================================*/

#define WRITE(...) do {                               \
    if (fprintf(file, __VA_ARGS__) < 0) return false; \
  } while (false)

static bool write_room(const az_room_t *room, FILE *file) {
  WRITE("!R%d c(%.02f,%.02f,%f,%f) b%d d%d w%d\n", room->key,
        room->camera_bounds.min_r, room->camera_bounds.r_span,
        room->camera_bounds.min_theta, room->camera_bounds.theta_span,
        room->num_baddies, room->num_doors, room->num_walls);
  for (int i = 0; i < room->num_walls; ++i) {
    const az_wall_t *wall = &room->walls[i];
    WRITE("W%d x%.02f y%.02f a%f\n", az_wall_data_index(wall->data),
          wall->position.x, wall->position.y, wall->angle);
  }
  for (int i = 0; i < room->num_doors; ++i) {
    const az_door_spec_t *door = &room->doors[i];
    WRITE("D%d x%.02f y%.02f a%f r%d\n", (int)door->kind,
          door->position.x, door->position.y, door->angle, door->destination);
  }
  for (int i = 0; i < room->num_baddies; ++i) {
    const az_baddie_spec_t *baddie = &room->baddies[i];
    WRITE("B%d x%.02f y%.02f a%f\n", (int)baddie->kind,
          baddie->position.x, baddie->position.y, baddie->angle);
  }
  return true;
}

#undef WRITE

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

  free(room->baddies);
  room->num_baddies = room->max_num_baddies = 0;
  room->baddies = NULL;

  free(room->doors);
  room->num_doors = room->max_num_doors = 0;
  room->doors = NULL;

  free(room->walls);
  room->num_walls = room->max_num_walls = 0;
  room->walls = NULL;
}

/*===========================================================================*/
