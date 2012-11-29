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

#include "azimuth/tick/baddie.h"

#include <assert.h>
#include <math.h> // for fmax
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// How long it takes a baddie's armor flare to die down, in seconds:
#define AZ_BADDIE_ARMOR_FLARE_TIME 0.3

static void tick_baddie(az_space_state_t *state, az_baddie_t *baddie,
                        double time) {
  // Allow the armor flare to die down a bit.
  assert(baddie->armor_flare >= 0.0);
  assert(baddie->armor_flare <= 1.0);
  baddie->armor_flare =
    fmax(0.0, baddie->armor_flare - time / AZ_BADDIE_ARMOR_FLARE_TIME);
  // Cool down the baddie's weapon.
  baddie->cooldown = fmax(0.0, baddie->cooldown - time);
  // Apply velocity.
  baddie->position = az_vadd(baddie->position,
                             az_vmul(baddie->velocity, time));
  // Perform kind-specific logic.
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_LUMP:
      baddie->angle = az_mod2pi(baddie->angle + 3.0 * time);
      break;
    case AZ_BAD_TURRET:
      // Aim gun:
      {
        const double gun_angle = baddie->components[0].angle;
        const double delta =
          az_mod2pi(baddie->angle + gun_angle -
                    az_vtheta(az_vsub(state->ship.position,
                                      baddie->position)));
        if (delta < 0.0 && gun_angle <= 1.0) {
          baddie->components[0].angle += 2.0 * time;
        } else if (delta > 0.0 && gun_angle >= -1.0) {
          baddie->components[0].angle -= 2.0 * time;
        }
      }
      // Fire:
      if (baddie->cooldown <= 0.0) {
        // TODO: Only fire if it might hit the ship.
        az_projectile_t *proj;
        if (az_insert_projectile(state, &proj)) {
          const double angle = baddie->angle + baddie->components[0].angle;
          az_init_projectile(proj, AZ_PROJ_GUN_NORMAL, true,
                             az_vadd(baddie->position, az_vpolar(20.0, angle)),
                             angle);
          baddie->cooldown = 0.5;
        }
      }
      break;
  }
}

void az_tick_baddies(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    assert(baddie->health > 0.0);
    tick_baddie(state, baddie, time);
  }
}

/*===========================================================================*/
