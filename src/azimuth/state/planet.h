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

#pragma once
#ifndef AZIMUTH_STATE_PLANET_H_
#define AZIMUTH_STATE_PLANET_H_

#include <stdbool.h>

#include "azimuth/state/dialog.h"
#include "azimuth/state/player.h"
#include "azimuth/state/room.h"
#include "azimuth/state/music.h" // for az_music_key_t
#include "azimuth/state/script.h"
#include "azimuth/util/rw.h"

/*===========================================================================*/

typedef struct {
  char *name; // NUL-terminated; owned by zone object
  char *entering_message; // NUL-terminated; owned by zone object
  az_color_t color;
} az_zone_t;

// Hints are evaluated as:
//   ((true <op1> prereq1) <op2> prereq2) AND NOT result
// Each of prereq1/prereq2/result can be an upgrade or a flag, and each of
// op1/op2 can be AND or OR.
typedef uint8_t az_hint_flags_t;
#define AZ_HINTF_OP1_IS_AND      ((az_hint_flags_t)(1u << 1))
#define AZ_HINTF_PREREQ1_IS_FLAG ((az_hint_flags_t)(1u << 2))
#define AZ_HINTF_OP2_IS_AND      ((az_hint_flags_t)(1u << 3))
#define AZ_HINTF_PREREQ2_IS_FLAG ((az_hint_flags_t)(1u << 4))
#define AZ_HINTF_RESULT_IS_FLAG  ((az_hint_flags_t)(1u << 5))

typedef struct {
  az_hint_flags_t properties;
  uint8_t prereq1, prereq2, result;
  az_room_key_t target_room;
} az_hint_t;

typedef struct {
  az_room_key_t start_room;
  az_script_t *on_start;
  int num_paragraphs;
  char **paragraphs;
  int num_zones;
  az_zone_t *zones;
  int num_hints;
  az_hint_t *hints;
  int num_rooms;
  az_room_t *rooms;
} az_planet_t;

bool az_read_planet(az_resource_reader_fn_t resource_reader,
                    az_planet_t *planet_out);

bool az_write_planet(const az_planet_t *planet,
                     az_resource_writer_fn_t resource_writer,
                     const az_room_key_t *rooms_to_write,
                     int num_rooms_to_write);

// Delete the data arrays owned by a planet (but not the planet object itself).
void az_destroy_planet(az_planet_t *planet);

void az_clone_zone(const az_zone_t *original, az_zone_t *clone_out);

/*===========================================================================*/

bool az_hint_matches(const az_hint_t *hint, const az_player_t *player);

const az_hint_t *az_get_hint(const az_planet_t *planet,
                             const az_player_t *player);

/*===========================================================================*/

#endif // AZIMUTH_STATE_PLANET_H_
