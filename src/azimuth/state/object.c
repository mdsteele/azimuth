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
#include "azimuth/util/warning.h"

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

// Forward-declaration, needed for mutual recursion:
static void move_baddie_cargo_internal(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t delta_position,
    double delta_angle, int depth);

static void move_object_internal(
    az_space_state_t *state, az_object_t *object, az_vector_t delta_position,
    double delta_angle, int depth) {
  if (depth > 3) {
    AZ_WARNING_ONCE("az_move_object recursion overflow\n");
    return;
  }
  assert(object->type != AZ_OBJ_NOTHING);
  switch (object->type) {
    case AZ_OBJ_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_OBJ_BADDIE:
      assert(object->obj.baddie->kind != AZ_BAD_NOTHING);
      object->obj.baddie->position =
        az_vadd(object->obj.baddie->position, delta_position);
      object->obj.baddie->angle =
        az_mod2pi(object->obj.baddie->angle + delta_angle);
      // When a baddie is made to move, its cargo moves with it.
      move_baddie_cargo_internal(state, object->obj.baddie, delta_position,
                                 delta_angle, depth + 1);
      break;
    case AZ_OBJ_DOOR:
      assert(object->obj.door->kind != AZ_DOOR_NOTHING);
      object->obj.door->position =
        az_vadd(object->obj.door->position, delta_position);
      object->obj.door->angle =
        az_mod2pi(object->obj.door->angle + delta_angle);
      break;
    case AZ_OBJ_GRAVFIELD:
      assert(object->obj.gravfield->kind != AZ_GRAV_NOTHING);
      object->obj.gravfield->position =
        az_vadd(object->obj.gravfield->position, delta_position);
      object->obj.gravfield->angle =
        az_mod2pi(object->obj.gravfield->angle + delta_angle);
      break;
    case AZ_OBJ_NODE:
      assert(object->obj.node->kind != AZ_NODE_NOTHING);
      if (az_vnonzero(delta_position)) {
        const az_vector_t new_node_position =
          az_vadd(object->obj.node->position, delta_position);
        // If we move a tractor node while the ship is locked onto it, we have
        // to drag the ship along.
        if (object->obj.node->kind == AZ_NODE_TRACTOR &&
            state->ship.tractor_beam.active) {
          az_ship_t *ship = &state->ship;
          az_node_t *tractor_node = NULL;
          if (az_lookup_node(state, ship->tractor_beam.node_uid,
                             &tractor_node) &&
              tractor_node == object->obj.node) {
            // Figure out where the ship is begin dragged to.
            const az_vector_t new_ship_position =
              az_vadd(new_node_position, az_vwithlen(
                az_vsub(ship->position, new_node_position),
                ship->tractor_beam.distance));
            // Check whether pulling the ship makes it hit something along the
            // way.
            az_impact_flags_t impact_flags = AZ_IMPF_SHIP;
            if (ship->temp_invincibility > 0.0) impact_flags |= AZ_IMPF_BADDIE;
            az_impact_t impact;
            az_circle_impact(
                state, AZ_SHIP_DEFLECTOR_RADIUS, ship->position,
                az_vsub(new_ship_position, ship->position), impact_flags,
                AZ_SHIP_UID, &impact);
            ship->position = impact.position;
            // If the ship hit something, break the tractor beam.
            if (impact.type != AZ_IMP_NOTHING) {
              ship->position =
                az_vadd(ship->position, az_vwithlen(impact.normal, 0.5));
              ship->tractor_beam.active = false;
              ship->tractor_beam.node_uid = AZ_NULL_UID;
            }
          }
        }
        object->obj.node->position = new_node_position;
      }
      object->obj.node->angle =
        az_mod2pi(object->obj.node->angle + delta_angle);
      break;
    case AZ_OBJ_SHIP:
      assert(object->obj.ship == &state->ship);
      assert(az_ship_is_present(object->obj.ship));
      if (az_vnonzero(delta_position)) {
        // If the ship is forced to move while the tractor beam is active,
        // break the tractor beam.
        if (object->obj.ship->tractor_beam.active) {
          object->obj.ship->tractor_beam.active = false;
          object->obj.ship->tractor_beam.node_uid = AZ_NULL_UID;
        }
        object->obj.ship->position =
          az_vadd(object->obj.ship->position, delta_position);
      }
      object->obj.ship->angle =
        az_mod2pi(object->obj.ship->angle + delta_angle);
      break;
    case AZ_OBJ_WALL:
      assert(object->obj.wall->kind != AZ_WALL_NOTHING);
      object->obj.wall->position =
        az_vadd(object->obj.wall->position, delta_position);
      object->obj.wall->angle =
        az_mod2pi(object->obj.wall->angle + delta_angle);
      break;
  }
}

void az_move_object(az_space_state_t *state, az_object_t *object,
                    az_vector_t delta_position, double delta_angle) {
  move_object_internal(state, object, delta_position, delta_angle, 0);
}

static void move_baddie_cargo_internal(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t delta_position,
    double delta_angle, int depth) {
  AZ_ARRAY_LOOP(uuid, baddie->cargo_uuids) {
    az_object_t cargo;
    if (az_lookup_object(state, *uuid, &cargo)) {
      const az_vector_t old_rel =
        az_vsub(az_get_object_position(&cargo),
                az_vsub(baddie->position, delta_position));
      const az_vector_t new_rel = az_vrotate(old_rel, delta_angle);
      move_object_internal(
          state, &cargo,
          az_vadd(delta_position, az_vsub(new_rel, old_rel)), delta_angle,
          depth + 1);
    }
  }
}

void az_move_baddie_cargo(az_space_state_t *state, az_baddie_t *baddie,
                          az_vector_t delta_position, double delta_angle) {
  move_baddie_cargo_internal(state, baddie, delta_position, delta_angle, 1);
}

/*===========================================================================*/
