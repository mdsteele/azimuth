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

#include "azimuth/tick/gravfield.h"

#include "azimuth/state/gravfield.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void apply_gravfield_to_ship(
    az_space_state_t *state, az_gravfield_t *gravfield, double time) {
  az_ship_t *ship = &state->ship;
  switch (gravfield->kind) {
    case AZ_GRAV_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_GRAV_TRAPEZOID:
      ship->velocity =
        az_vadd(ship->velocity, az_vpolar(gravfield->strength * time,
                                          gravfield->angle));
      break;
    case AZ_GRAV_SECTOR_PULL:
      ship->velocity =
        az_vadd(ship->velocity,
                az_vwithlen(az_vsub(gravfield->position, ship->position),
                            gravfield->strength * time));
      break;
    case AZ_GRAV_SECTOR_SPIN:
      {
        const az_vector_t delta =
          az_vsub(ship->position, gravfield->position);
        const double dtheta =
          time * gravfield->strength / (gravfield->size.sector.inner_radius +
                                        gravfield->size.sector.thickness);
        ship->angle = az_mod2pi(ship->angle + dtheta);
        ship->velocity =
          az_vadd(az_vrotate(ship->velocity, dtheta),
                  az_vpolar(dtheta * az_vnorm(delta),
                            az_vtheta(delta) + AZ_HALF_PI));
      }
      break;
  }
}

void az_tick_gravfields(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind == AZ_GRAV_NOTHING) continue;
    if (az_point_within_gravfield(gravfield, state->ship.position)) {
      apply_gravfield_to_ship(state, gravfield, time);
    }
  }
}

/*===========================================================================*/
