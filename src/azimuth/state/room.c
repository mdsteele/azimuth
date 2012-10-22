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

#include "azimuth/state/wall.h"

/*===========================================================================*/

// TODO: move these elsewhere
#define AZ_MAX_NUM_ROOMS 192
#define AZ_MAX_NUM_WALLS 250

#define ALLOCATE(n, type) ((type *)_safe_calloc((n), sizeof(type)))

static void *_safe_calloc(size_t n, size_t size) {
  void *ptr = calloc(n, size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory.\n");
    abort();
  }
  return ptr;
}

typedef struct {
  FILE *file;
  az_room_t *room;
  jmp_buf jump;
} az_load_room_t;

#define FAIL() longjmp(loader->jump, 1)

static void parse_room_header(az_load_room_t *loader) {
  int room_num, num_walls;
  if (fscanf(loader->file, "!R%d w%d\n", &room_num, &num_walls) < 2) FAIL();
  if (room_num < 0 || room_num >= AZ_MAX_NUM_ROOMS) FAIL();
  loader->room->key = room_num;
  if (num_walls < 0 || num_walls > AZ_MAX_NUM_WALLS) FAIL();
  loader->room->max_num_walls = num_walls;
  loader->room->walls = ALLOCATE(num_walls, az_wall_t);
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
    case 'W': parse_wall_directive(loader); return true;
    case EOF: return false;
    default: FAIL();
  }
  assert(false); // unreachable
}

static void validate_room(az_load_room_t *loader) {
  if (loader->room->num_walls != loader->room->max_num_walls) FAIL();
}

#undef FAIL

static void parse_room(az_load_room_t *loader) {
  if (setjmp(loader->jump) != 0) {
    az_destroy_room(loader->room);
    loader->room = NULL;
    return;
  }
  parse_room_header(loader);
  while (parse_directive(loader));
  validate_room(loader);
}

az_room_t *az_load_room_from_file(const char *filepath) {
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return NULL;
  az_load_room_t loader = { .file = file, .room = ALLOCATE(1, az_room_t) };
  parse_room(&loader);
  fclose(file);
  return loader.room;
}

/*===========================================================================*/

#define WRITE(...) do {                               \
    if (fprintf(file, __VA_ARGS__) < 0) return false; \
  } while (false)

static bool write_room(const az_room_t *room, FILE *file) {
  WRITE("!R%d w%d\n", room->key, room->num_walls);
  for (int i = 0; i < room->num_walls; ++i) {
    const az_wall_t *wall = &room->walls[i];
    WRITE("W%d x%.02f y%.02f a%f\n", az_wall_data_index(wall->data),
          wall->position.x, wall->position.y, wall->angle);
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
  if (room == NULL) return;
  free(room->walls);
  free(room);
}

/*===========================================================================*/
