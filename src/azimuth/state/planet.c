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

#include "azimuth/state/planet.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // for free
#include <string.h> // for strlen

#include "azimuth/constants.h"
#include "azimuth/state/room.h"
#include "azimuth/util/misc.h" // for AZ_ALLOC

/*===========================================================================*/

static bool parse_planet_header(FILE *file, az_planet_t *planet_out) {
  int num_rooms, start_room_num;
  double start_x, start_y, start_angle;
  if (fscanf(file, "!P r%d s%d x%lf y%lf a%lf\n", &num_rooms, &start_room_num,
             &start_x, &start_y, &start_angle) < 5) return false;
  if (fgetc(file) != EOF) return false;
  if (num_rooms < 1 || num_rooms > AZ_MAX_NUM_ROOMS) return false;
  if (start_room_num < 0 || start_room_num >= num_rooms) return false;
  planet_out->num_rooms = num_rooms;
  planet_out->rooms = AZ_ALLOC(num_rooms, az_room_t);
  planet_out->start_room = start_room_num;
  planet_out->start_position = (az_vector_t){.x = start_x, .y = start_y};
  planet_out->start_angle = start_angle;
  return true;
}

static bool load_planet_header(const char *filepath, az_planet_t *planet_out) {
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  const bool success = parse_planet_header(file, planet_out);
  fclose(file);
  return success;
}

bool az_load_planet(const char *resource_dir, az_planet_t *planet_out) {
  assert(resource_dir != NULL);
  assert(planet_out != NULL);
  const size_t dirlen = strlen(resource_dir);
  char path_buffer[dirlen + 20u];

  sprintf(path_buffer, "%s/rooms/planet.txt", resource_dir);
  if (!load_planet_header(path_buffer, planet_out)) return false;

  for (int i = 0; i < planet_out->num_rooms; ++i) {
    sprintf(path_buffer, "%s/rooms/room%03d.txt", resource_dir, i);
    if (!az_load_room_from_file(path_buffer, &planet_out->rooms[i])) {
      planet_out->num_rooms = i;
      az_destroy_planet(planet_out);
      return false;
    }
  }

  return true;
}

/*===========================================================================*/

static bool write_planet_header(const az_planet_t *planet, FILE *file) {
  if (fprintf(file, "!P r%d s%d x%.02f y%.02f a%f\n",
              planet->num_rooms, planet->start_room,
              planet->start_position.x, planet->start_position.y,
              planet->start_angle) < 0) return false;
  return true;
}

static bool save_planet_header(const az_planet_t *planet,
                               const char *filepath) {
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool success = write_planet_header(planet, file);
  fclose(file);
  return success;
}

bool az_save_planet(const az_planet_t *planet, const char *resource_dir) {
  assert(planet != NULL);
  assert(resource_dir != NULL);
  const size_t dirlen = strlen(resource_dir);
  char path_buffer[dirlen + 20u];

  for (int i = 0; i < planet->num_rooms; ++i) {
    sprintf(path_buffer, "%s/rooms/room%03d.txt", resource_dir, i);
    if (!az_save_room_to_file(&planet->rooms[i], path_buffer)) return false;
  }

  sprintf(path_buffer, "%s/rooms/planet.txt", resource_dir);
  return save_planet_header(planet, path_buffer);
}

/*===========================================================================*/

void az_destroy_planet(az_planet_t *planet) {
  assert(planet != NULL);
  for (int i = 0; i < planet->num_rooms; ++i) {
    az_destroy_room(&planet->rooms[i]);
  }
  free(planet->rooms);
  planet->num_rooms = 0;
  planet->rooms = NULL;
}

/*===========================================================================*/
