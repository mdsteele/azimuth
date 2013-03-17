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
#include "azimuth/state/camera.h" // for az_camera_bounds_t
#include "azimuth/state/door.h" // for az_door_kind_t
#include "azimuth/state/gravfield.h"
#include "azimuth/state/node.h" // for az_node_kind_t
#include "azimuth/state/player.h" // for az_room_key_t
#include "azimuth/state/script.h"
#include "azimuth/state/wall.h"

/*===========================================================================*/

// The size of the UUID table:
#define AZ_NUM_UUID_SLOTS 20

typedef struct {
  az_baddie_kind_t kind;
  az_script_t *on_kill; // owned; NULL if no script
  az_vector_t position;
  double angle;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_baddie_spec_t;

typedef struct {
  az_door_kind_t kind;
  az_script_t *on_open; // owned; NULL if no script
  az_vector_t position;
  double angle;
  az_room_key_t destination;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_door_spec_t;

typedef struct {
  az_gravfield_kind_t kind;
  az_vector_t position;
  double angle;
  double strength;
  az_gravfield_size_t size;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_gravfield_spec_t;

typedef struct {
  az_node_kind_t kind;
  az_node_subkind_t subkind;
  az_script_t *on_use; // owned; NULL if no script
  az_vector_t position;
  double angle;
} az_node_spec_t;

typedef struct {
  az_wall_kind_t kind;
  const az_wall_data_t *data;
  az_vector_t position;
  double angle;
  int uuid_slot; // 0 if none, otherwise from 1 to AZ_NUM_UUID_SLOTS inclusive
} az_wall_spec_t;

// Represents one room of the planetoid.  This sturct owns all of its pointers.
typedef struct {
  az_room_key_t key;
  int zone_index;
  az_camera_bounds_t camera_bounds;
  az_script_t *on_start; // NULL if no script
  // Initial room objects:
  int num_baddies;
  az_baddie_spec_t *baddies;
  int num_doors;
  az_door_spec_t *doors;
  int num_gravfields;
  az_gravfield_spec_t *gravfields;
  int num_nodes;
  az_node_spec_t *nodes;
  int num_walls;
  az_wall_spec_t *walls;
} az_room_t;

// Attempt to open the file located at the given path and load room data from
// it.  Returns true on success, false on failure.
bool az_load_room_from_file(const char *filepath, az_room_t *room_out);

// Attempt to save a room to the file located at the given path.  Return true
// on success, or false on failure.
bool az_save_room_to_file(const az_room_t *room, const char *filepath);

// Delete the data arrays owned by a room (but not the room object itself).
void az_destroy_room(az_room_t *room);

/*===========================================================================*/

#endif // AZIMUTH_STATE_ROOM_H_
