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

#include "azimuth/constants.h"
#include "azimuth/state/pickup.h"
#include "azimuth/state/player.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_tick_pickups(az_space_state_t *state, double time) {
  az_ship_t *ship = &state->ship;
  az_player_t *player = &ship->player;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    pickup->age += time;
    if (az_ship_is_alive(ship) &&
        az_vwithin(pickup->position, ship->position,
                   AZ_PICKUP_COLLECT_RANGE)) {
      switch (pickup->kind) {
        case AZ_PUP_NOTHING: AZ_ASSERT_UNREACHABLE();
        case AZ_PUP_ROCKETS:
          player->rockets = az_imin(player->max_rockets, player->rockets +
                                    AZ_ROCKETS_PER_PICKUP);
          az_play_sound(&state->soundboard, AZ_SND_PICKUP_ORDNANCE);
          break;
        case AZ_PUP_BOMBS:
          player->bombs = az_imin(player->max_bombs, player->bombs +
                                  AZ_BOMBS_PER_PICKUP);
          az_play_sound(&state->soundboard, AZ_SND_PICKUP_ORDNANCE);
          break;
        case AZ_PUP_SMALL_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_SMALL_PICKUP);
          az_play_sound(&state->soundboard, AZ_SND_PICKUP_SHIELDS);
          break;
        case AZ_PUP_MEDIUM_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_MEDIUM_PICKUP);
          az_play_sound(&state->soundboard, AZ_SND_PICKUP_SHIELDS);
          break;
        case AZ_PUP_LARGE_SHIELDS:
          player->shields = az_imin(player->max_shields, player->shields +
                                    AZ_SHIELDS_PER_LARGE_PICKUP);
          az_play_sound(&state->soundboard, AZ_SND_PICKUP_SHIELDS);
          break;
      }
      pickup->kind = AZ_PUP_NOTHING;
    } else if (pickup->age >= AZ_PICKUP_MAX_AGE) {
      pickup->kind = AZ_PUP_NOTHING;
    } else if (az_ship_is_alive(ship)) {
      const double attract_range =
        (az_has_upgrade(player, AZ_UPG_MAGNET_SWEEP) ?
         AZ_MAGNET_SWEEP_ATTRACT_RANGE : AZ_NORMAL_PICKUP_ATTRACT_RANGE);
      if (az_vwithin(pickup->position, ship->position, attract_range)) {
        const az_vector_t delta = az_vsub(ship->position, pickup->position);
        const double speed = 300.0 * (1.3 - az_vnorm(delta) / attract_range);
        pickup->position =
          az_vadd(pickup->position, az_vwithlen(delta, speed * time));
      }
    }
  }
}

/*===========================================================================*/
