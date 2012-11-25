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

#include "azimuth/tick/pickup.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/constants.h"
#include "azimuth/state/pickup.h"
#include "azimuth/state/player.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

void az_tick_pickups(az_space_state_t *state, double time) {
  az_player_t *player = &state->ship.player;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    pickup->age += time;
    if (az_vwithin(pickup->position, state->ship.position,
                   AZ_PICKUP_COLLECTION_RANGE)) {
      switch (pickup->kind) {
        case AZ_PUP_NOTHING: AZ_ASSERT_UNREACHABLE();
        case AZ_PUP_ROCKETS:
          player->rockets = az_imin(player->max_rockets, player->rockets +
                                    AZ_ROCKETS_PER_PICKUP);
          break;
        case AZ_PUP_BOMBS:
          player->bombs = az_imin(player->max_bombs, player->bombs +
                                  AZ_BOMBS_PER_PICKUP);
          break;
        case AZ_PUP_SMALL_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_SMALL_PICKUP);
          break;
        case AZ_PUP_MEDIUM_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_MEDIUM_PICKUP);
          break;
        case AZ_PUP_LARGE_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_LARGE_PICKUP);
          break;
      }
      pickup->kind = AZ_PUP_NOTHING;
    } else if (pickup->age >= AZ_PICKUP_MAX_AGE) {
      pickup->kind = AZ_PUP_NOTHING;
    }
  }
}

/*===========================================================================*/

#define NOTHING_PROB 6
#define ROCKETS_PROB 3
#define BOMBS_PROB 2
#define SSHIELDS_PROB 6
#define MSHIELDS_PROB 3
#define LSHIELDS_PROB 1

static az_pickup_kind_t choose_kind(az_space_state_t *state,
                                    unsigned int potential_pickups) {
  // Filter the permitted pickups; do not include pickups that the player
  // doesn't currently need.
  const az_player_t *player = &state->ship.player;
  if (player->rockets >= player->max_rockets) {
    potential_pickups &= ~AZ_PUPF_ROCKETS;
  }
  if (player->bombs >= player->max_bombs) {
    potential_pickups &= ~AZ_PUPF_BOMBS;
  }
  if (player->shields >= player->max_shields) {
    potential_pickups &= ~(AZ_PUPF_SMALL_SHIELDS | AZ_PUPF_MEDIUM_SHIELDS |
                           AZ_PUPF_LARGE_SHIELDS);
  }

  // Determine the random range:
  int limit = 0;
  if (potential_pickups & AZ_PUPF_NOTHING) limit += NOTHING_PROB;
  if (potential_pickups & AZ_PUPF_ROCKETS) limit += ROCKETS_PROB;
  if (potential_pickups & AZ_PUPF_BOMBS) limit += BOMBS_PROB;
  if (potential_pickups & AZ_PUPF_SMALL_SHIELDS) limit += SSHIELDS_PROB;
  if (potential_pickups & AZ_PUPF_MEDIUM_SHIELDS) limit += MSHIELDS_PROB;
  if (potential_pickups & AZ_PUPF_LARGE_SHIELDS) limit += LSHIELDS_PROB;
  if (limit == 0) return AZ_PUP_NOTHING;

  // Select the pickup kind:
  int choice = az_randint(0, limit - 1);
  if ((potential_pickups & AZ_PUPF_ROCKETS) && choice < ROCKETS_PROB) {
    return AZ_PUP_ROCKETS;
  } else choice -= ROCKETS_PROB;
  if ((potential_pickups & AZ_PUPF_BOMBS) && choice < BOMBS_PROB) {
    return AZ_PUP_BOMBS;
  } else choice -= BOMBS_PROB;
  if ((potential_pickups & AZ_PUPF_SMALL_SHIELDS) && choice < SSHIELDS_PROB) {
    return AZ_PUP_SMALL_SHIELDS;
  } else choice -= SSHIELDS_PROB;
  if ((potential_pickups & AZ_PUPF_MEDIUM_SHIELDS) && choice < MSHIELDS_PROB) {
    return AZ_PUP_MEDIUM_SHIELDS;
  } else choice -= MSHIELDS_PROB;
  if ((potential_pickups & AZ_PUPF_LARGE_SHIELDS) && choice < LSHIELDS_PROB) {
    return AZ_PUP_LARGE_SHIELDS;
  } else choice -= LSHIELDS_PROB;
  assert(potential_pickups & AZ_PUPF_NOTHING);
  assert(choice < NOTHING_PROB);
  return AZ_PUP_NOTHING;
}

void az_add_random_pickup(az_space_state_t *state,
                          unsigned int potential_pickups,
                          az_vector_t position) {
  az_pickup_kind_t kind = choose_kind(state, potential_pickups);
  if (kind == AZ_PUP_NOTHING) return;
  az_try_add_pickup(state, kind, position);
}

/*===========================================================================*/
