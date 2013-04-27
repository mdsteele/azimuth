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
#ifndef AZIMUTH_STATE_OBJECT_H_
#define AZIMUTH_STATE_OBJECT_H_

#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/door.h"
#include "azimuth/state/gravfield.h"
#include "azimuth/state/node.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_OBJ_NOTHING = 0,
  AZ_OBJ_BADDIE,
  AZ_OBJ_DOOR,
  AZ_OBJ_GRAVFIELD,
  AZ_OBJ_NODE,
  AZ_OBJ_SHIP,
  AZ_OBJ_WALL
} az_object_type_t;

typedef struct {
  az_object_type_t type;
  union {
    az_baddie_t *baddie;
    az_door_t *door;
    az_gravfield_t *gravfield;
    az_node_t *node;
    az_ship_t *ship;
    az_wall_t *wall;
  } obj;
} az_object_t;

// Get the object referred to by the given UUID and return true, or return
// false and set type to AZ_OBJ_NOTHING if it doesn't exist (or if uuid.type is
// AZ_UUID_NOTHING).
bool az_lookup_object(az_space_state_t *state, az_uuid_t uuid,
                      az_object_t *object_out);

// Get the position or angle of an object.  The object must exist (i.e. its
// type must not be AZ_OBJ_NOTHING, and the whatever it points to must also
// exist).
az_vector_t az_get_object_position(const az_object_t *object);
double az_get_object_angle(const az_object_t *object);

/*===========================================================================*/

#endif // AZIMUTH_STATE_OBJECT_H_
