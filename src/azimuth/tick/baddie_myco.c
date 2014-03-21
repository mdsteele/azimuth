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

#include "azimuth/tick/baddie_myco.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void fire_spores(az_space_state_t *state, az_baddie_t *baddie,
                        double time, double limit, double spread) {
  if (az_ship_in_range(state, baddie, 350) &&
      az_can_see_ship(state, baddie)) {
    if (baddie->cooldown <= 0.0) {
      const double angle =
        fmin(fmax(az_mod2pi(az_vtheta(az_vsub(state->ship.position,
                                              baddie->position)) -
                            baddie->angle), -limit), limit);
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_MYCOSPORE, 10.0,
                                angle, spread * az_random(-1, 1));
      baddie->cooldown = 0.3;
    }
    baddie->state = 1;
  } else baddie->state = 0;
}

/*===========================================================================*/

void az_tick_bad_mycoflakker(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_MYCOFLAKKER);
  fire_spores(state, baddie, time, AZ_DEG2RAD(20), AZ_DEG2RAD(60));
}

void az_tick_bad_mycostalker(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_MYCOSTALKER);
  az_crawl_around(state, baddie, time, az_ship_is_decloaked(&state->ship) &&
                  az_vcross(az_vsub(state->ship.position, baddie->position),
                            az_vpolar(1.0, baddie->angle)) > 0.0,
                  1.0, 30.0, 100.0);
  fire_spores(state, baddie, time, AZ_DEG2RAD(60), AZ_DEG2RAD(30));
}

/*===========================================================================*/
