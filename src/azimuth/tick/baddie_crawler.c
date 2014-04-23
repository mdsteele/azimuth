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

#include "azimuth/tick/baddie_crawler.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_cave_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CAVE_CRAWLER);
  az_crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
}

void az_tick_bad_spined_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SPINED_CRAWLER);
  if (baddie->state == 0) {
    baddie->param = baddie->angle;
    baddie->state = az_randint(1, 2);
  }
  if (baddie->state == 1 || baddie->state == 2) {
    const double angle = fabs(az_mod2pi(baddie->angle - baddie->param));
    az_crawl_around(state, baddie, time, (baddie->state == 1), 3.0,
                    50.0 - 40.0 * fmin(1.0, angle / AZ_HALF_PI), 100.0);
    if (angle > AZ_DEG2RAD(80) && baddie->cooldown <= 0.0) {
      baddie->state = (baddie->state == 1 ? 2 : 1);
      baddie->cooldown = 1.0;
    }
    if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 90) &&
        az_can_see_ship(state, baddie)) {
      const az_vector_t center =
        az_vadd(baddie->position, az_vpolar(-13, baddie->angle));
      for (int i = -2; i <= 2; ++i) {
        const double theta = baddie->angle + i * AZ_DEG2RAD(41);
        az_add_projectile(
            state, AZ_PROJ_STINGER,
            az_vadd(center, az_vpolar(10.0, theta)),
            theta + az_random(-1, 1) * AZ_DEG2RAD(20), 1.0, baddie->uid);
      }
      az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
      baddie->velocity = AZ_VZERO;
      baddie->state = 3;
      baddie->cooldown = 2.5;
    }
  } else if (baddie->state == 3) {
    baddie->velocity = AZ_VZERO;
    if (baddie->cooldown <= 0.0) baddie->state = az_randint(1, 2);
  } else baddie->state = 0;
}

void az_tick_bad_ice_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_ICE_CRAWLER);
  az_crawl_around(state, baddie, time, false, 3.0, 30.0, 100.0);
}

void az_tick_bad_fire_crawler(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_FIRE_CRAWLER);
  az_crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
  if (baddie->state <= 0) {
    baddie->state = az_randint(2, 5);
    baddie->cooldown = 2.0;
  } else if (baddie->cooldown <= 0.0 &&
             az_ship_in_range(state, baddie, 300) &&
             az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(
        state, baddie, AZ_PROJ_FIREBALL_SLOW, 0.0, 0.0,
        az_vtheta(az_vsub(state->ship.position, baddie->position)) +
        AZ_DEG2RAD(5) * az_random(-1, 1) - baddie->angle);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
    baddie->cooldown = 0.1;
    --baddie->state;
  }
}

/*===========================================================================*/
