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

#include "azimuth/state/pickup.h"

#include <assert.h>

#include "azimuth/state/player.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

#define NOTHING_PROB 12
#define ROCKETS_PROB 6
#define BOMBS_PROB 4
#define SSHIELDS_PROB 12
#define MSHIELDS_PROB 6
#define LSHIELDS_PROB 1

az_pickup_kind_t az_choose_random_pickup_kind(
    const az_player_t *player, az_pickup_flags_t potential_pickups) {
  // Filter the permitted pickups; do not include pickups that the player
  // doesn't currently need.
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
  if (potential_pickups & AZ_PUPF_ROCKETS) {
    if (choice < ROCKETS_PROB) return AZ_PUP_ROCKETS;
    else choice -= ROCKETS_PROB;
  }
  if (potential_pickups & AZ_PUPF_BOMBS) {
    if (choice < BOMBS_PROB) return AZ_PUP_BOMBS;
    else choice -= BOMBS_PROB;
  }
  if (potential_pickups & AZ_PUPF_SMALL_SHIELDS) {
    if (choice < SSHIELDS_PROB) return AZ_PUP_SMALL_SHIELDS;
    else choice -= SSHIELDS_PROB;
  }
  if (potential_pickups & AZ_PUPF_MEDIUM_SHIELDS) {
    if (choice < MSHIELDS_PROB) return AZ_PUP_MEDIUM_SHIELDS;
    else choice -= MSHIELDS_PROB;
  }
  if (potential_pickups & AZ_PUPF_LARGE_SHIELDS) {
    if (choice < LSHIELDS_PROB) return AZ_PUP_LARGE_SHIELDS;
    else choice -= LSHIELDS_PROB;
  }
  assert(potential_pickups & AZ_PUPF_NOTHING);
  assert(choice < NOTHING_PROB);
  return AZ_PUP_NOTHING;
}

/*===========================================================================*/
