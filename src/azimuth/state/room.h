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
#ifndef AZIMUTH_STATE_ROOM_H_
#define AZIMUTH_STATE_ROOM_H_

#include "azimuth/state/baddie.h" // for az_baddie_kind_t
#include "azimuth/state/player.h" // for az_room_key_t
#include "azimuth/state/wall.h"

/*===========================================================================*/

typedef struct {
  az_baddie_kind_t kind; // if AZ_BAD_NOTHING, this baddie is not present
  az_vector_t position;
  double angle;
} az_baddie_spec_t;

typedef struct {
  az_room_key_t key;
  int num_baddies, max_num_baddies;
  az_baddie_spec_t *baddies;
  int num_walls, max_num_walls;
  az_wall_t *walls;
} az_room_t;

// Attempt to open the file located at the given path and load room data from
// it.  Return a pointer to the newly allocated room object (which must be
// destroyed later with az_destroy_room), or NULL if anything went wrong.
az_room_t *az_load_room_from_file(const char *filepath);

// Attempt to save a room to the file located at the given path.  Return true
// on success, or false on failure.
bool az_save_room_to_file(const az_room_t *room, const char *filepath);

// Delete a room object and all its data arrays.
void az_destroy_room(az_room_t *room);

/*===========================================================================*/

#endif // AZIMUTH_STATE_ROOM_H_
