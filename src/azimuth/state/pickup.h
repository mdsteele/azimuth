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
#ifndef AZIMUTH_STATE_PICKUP_H_
#define AZIMUTH_STATE_PICKUP_H_

#include <stdint.h>

#include "azimuth/state/player.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// Constants:
#define AZ_ROCKETS_PER_PICKUP 3
#define AZ_BOMBS_PER_PICKUP 1
#define AZ_SHIELDS_PER_SMALL_PICKUP 5
#define AZ_SHIELDS_PER_MEDIUM_PICKUP 15
#define AZ_SHIELDS_PER_LARGE_PICKUP 50

// A bitset of pickup flags, made from OR-ing together the below constants:
typedef uint_fast8_t az_pickup_flags_t;
#define AZ_PUPF_NOTHING (1u << AZ_PUP_NOTHING)
#define AZ_PUPF_ROCKETS (1u << AZ_PUP_ROCKETS)
#define AZ_PUPF_BOMBS (1u << AZ_PUP_BOMBS)
#define AZ_PUPF_SMALL_SHIELDS (1u << AZ_PUP_SMALL_SHIELDS)
#define AZ_PUPF_MEDIUM_SHIELDS (1u << AZ_PUP_MEDIUM_SHIELDS)
#define AZ_PUPF_LARGE_SHIELDS (1u << AZ_PUP_LARGE_SHIELDS)

typedef enum {
  AZ_PUP_NOTHING = 0,
  AZ_PUP_ROCKETS,
  AZ_PUP_BOMBS,
  AZ_PUP_SMALL_SHIELDS,
  AZ_PUP_MEDIUM_SHIELDS,
  AZ_PUP_LARGE_SHIELDS
} az_pickup_kind_t;

typedef struct {
  az_pickup_kind_t kind; // if AZ_PUP_NOTHING, this pickup is not present
  az_vector_t position;
  double age; // seconds
} az_pickup_t;

/*===========================================================================*/

// Choose a random pickup kind (which might be AZ_PUP_NOTHING) based on
// potential_pickups, which should be one or more AZ_PUPF_* flags bitwise-or'd
// together.  If the AZ_PUPF_NOTHING flag is included, then there's a chance
// that no pickup will be dropped.  Moreover, a pickup kind that the player
// does not currently need will never be chosen.
az_pickup_kind_t az_choose_random_pickup_kind(
    const az_player_t *player, az_pickup_flags_t potential_pickups);

/*===========================================================================*/

#endif // AZIMUTH_STATE_PICKUP_H_
