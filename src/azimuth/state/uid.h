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
#ifndef AZIMUTH_STATE_UID_H_
#define AZIMUTH_STATE_UID_H_

#include <stdint.h> // for uint64_t

/*===========================================================================*/

// A UID uniquely identifies a game object (of a particular type) stored in one
// of the arrays in az_space_state_t.  Each time we insert an object into one
// of those arrays, we call az_assign_uid to give it a UID.  Given only the
// UID, we can quickly find the object in the array and check if its still the
// same object (as opposed to a new object that was placed there after the old
// one was removed).
typedef uint64_t az_uid_t;

// A zeroed az_uid_t struct; a UID that will never be created by a call to
// az_assign_uid.
extern const az_uid_t AZ_NULL_UID;

// A special UID that represents the ship; it will never be created by a call
// to az_assign_uid.
extern const az_uid_t AZ_SHIP_UID;

// Reinitialize a UID with a new value.  The index should be the array index at
// which the object that this UID is for is stored.
void az_assign_uid(int index, az_uid_t *uid);

// Return the index that was passed to az_assign_uid when the UID was assigned.
int az_uid_index(az_uid_t uid);

/*===========================================================================*/

// A UUID pairs a UID with a tag indicating which object type the UID
// corresponds to.  A UUID uniquely identifies a single space object.

typedef enum {
  AZ_UUID_NOTHING = 0,
  AZ_UUID_BADDIE,
  AZ_UUID_DOOR,
  AZ_UUID_GRAVFIELD,
  AZ_UUID_NODE,
  AZ_UUID_SHIP,
  AZ_UUID_WALL
} az_uuid_type_t;

typedef struct {
  az_uuid_type_t type;
  az_uid_t uid;
} az_uuid_t;

// A zeroed az_uuid_t struct; a UUID that represents nothing.
extern const az_uuid_t AZ_NULL_UUID;

// The unique UUID that represents the ship.
extern const az_uuid_t AZ_SHIP_UUID;

/*===========================================================================*/

#endif // AZIMUTH_STATE_UID_H_
