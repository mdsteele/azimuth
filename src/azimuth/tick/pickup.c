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

/*===========================================================================*/

void az_tick_pickups(az_space_state_t *state, double time) {
  az_player_t *player = &state->ship.player;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    pickup->age += time;
    if (az_vwithin(pickup->position, state->ship.position,
                   AZ_PICKUP_COLLECTION_RANGE)) {
      switch (pickup->kind) {
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
        default: assert(false);
      }
      pickup->kind = AZ_PUP_NOTHING;
    } else if (pickup->age >= AZ_PICKUP_MAX_AGE) {
      pickup->kind = AZ_PUP_NOTHING;
    }
  }
}

/*===========================================================================*/
