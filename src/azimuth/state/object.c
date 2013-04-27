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

#include "azimuth/state/object.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/door.h"
#include "azimuth/state/gravfield.h"
#include "azimuth/state/node.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_lookup_object(az_space_state_t *state, az_uuid_t uuid,
                      az_object_t *object_out) {
  assert(object_out != NULL);
  object_out->type = AZ_OBJ_NOTHING;
  switch (uuid.type) {
    case AZ_UUID_NOTHING: break;
    case AZ_UUID_BADDIE:
      if (az_lookup_baddie(state, uuid.uid, &object_out->obj.baddie)) {
        object_out->type = AZ_OBJ_BADDIE;
        return true;
      }
      break;
    case AZ_UUID_DOOR:
      if (az_lookup_door(state, uuid.uid, &object_out->obj.door)) {
        object_out->type = AZ_OBJ_DOOR;
        return true;
      }
      break;
    case AZ_UUID_GRAVFIELD:
      if (az_lookup_gravfield(state, uuid.uid, &object_out->obj.gravfield)) {
        object_out->type = AZ_OBJ_GRAVFIELD;
        return true;
      }
      break;
    case AZ_UUID_NODE:
      if (az_lookup_node(state, uuid.uid, &object_out->obj.node)) {
        object_out->type = AZ_OBJ_NODE;
        return true;
      }
      break;
    case AZ_UUID_SHIP:
      assert(uuid.uid == AZ_SHIP_UID);
      if (az_ship_is_present(&state->ship)) {
        object_out->type = AZ_OBJ_SHIP;
        object_out->obj.ship = &state->ship;
        return true;
      }
      break;
    case AZ_UUID_WALL:
      if (az_lookup_wall(state, uuid.uid, &object_out->obj.wall)) {
        object_out->type = AZ_OBJ_WALL;
        return true;
      }
      break;
  }
  return false;
}

az_vector_t az_get_object_position(const az_object_t *object) {
  assert(object->type != AZ_OBJ_NOTHING);
  switch (object->type) {
    case AZ_OBJ_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_OBJ_BADDIE:
      assert(object->obj.baddie->kind != AZ_BAD_NOTHING);
      return object->obj.baddie->position;
    case AZ_OBJ_DOOR:
      assert(object->obj.door->kind != AZ_DOOR_NOTHING);
      return object->obj.door->position;
    case AZ_OBJ_GRAVFIELD:
      assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
      return object->obj.gravfield->position;
    case AZ_OBJ_NODE:
      assert(object->obj.node->kind != AZ_NODE_NOTHING);
      return object->obj.node->position;
    case AZ_OBJ_SHIP:
      assert(az_ship_is_present(object->obj.ship));
      return object->obj.ship->position;
    case AZ_OBJ_WALL:
      assert(object->obj.wall->kind != AZ_WALL_NOTHING);
      return object->obj.wall->position;
  }
  AZ_ASSERT_UNREACHABLE();
}

double az_get_object_angle(const az_object_t *object) {
  assert(object->type != AZ_OBJ_NOTHING);
  switch (object->type) {
    case AZ_OBJ_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_OBJ_BADDIE:
      assert(object->obj.baddie->kind != AZ_BAD_NOTHING);
      return object->obj.baddie->angle;
    case AZ_OBJ_DOOR:
      assert(object->obj.door->kind != AZ_DOOR_NOTHING);
      return object->obj.door->angle;
    case AZ_OBJ_GRAVFIELD:
      assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
      return object->obj.gravfield->angle;
    case AZ_OBJ_NODE:
      assert(object->obj.node->kind != AZ_NODE_NOTHING);
      return object->obj.node->angle;
    case AZ_OBJ_SHIP:
      assert(az_ship_is_present(object->obj.ship));
      return object->obj.ship->angle;
    case AZ_OBJ_WALL:
      assert(object->obj.wall->kind != AZ_WALL_NOTHING);
      return object->obj.wall->angle;
  }
  AZ_ASSERT_UNREACHABLE();
}

/*===========================================================================*/
