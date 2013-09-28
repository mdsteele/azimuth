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

#include "azimuth/tick/baddie_util.h"

#include <math.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_ship_in_range(
    const az_space_state_t *state, const az_baddie_t *baddie, double range) {
  return (az_ship_is_present(&state->ship) &&
          az_vwithin(baddie->position, state->ship.position, range));
}

bool az_ship_within_angle(
    const az_space_state_t *state, const az_baddie_t *baddie,
    double offset, double angle) {
  return (az_ship_is_present(&state->ship) &&
          fabs(az_mod2pi(baddie->angle + offset -
                         az_vtheta(az_vsub(state->ship.position,
                                           baddie->position)))) <= angle);
}

bool az_can_see_ship(az_space_state_t *state, const az_baddie_t *baddie) {
  if (!az_ship_is_present(&state->ship)) return false;
  az_impact_t impact;
  az_ray_impact(state, baddie->position,
                az_vsub(state->ship.position, baddie->position),
                AZ_IMPF_BADDIE, baddie->uid, &impact);
  return (impact.type == AZ_IMP_SHIP);
}

/*===========================================================================*/

void az_crawl_around(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    bool rightwards, double turn_rate, double max_speed, double accel) {
  // Adjust velocity in crawling direction.
  const az_vector_t unit =
    az_vpolar(1.0, baddie->angle + AZ_DEG2RAD(rightwards ? -115 : 115));
  az_vpluseq(&baddie->velocity, az_vmul(unit, accel * time));
  const double drag_coeff = accel / (max_speed * max_speed);
  const az_vector_t drag_force =
    az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
  az_vpluseq(&baddie->velocity, az_vmul(drag_force, time));
  // Rotate to point away from wall.
  az_impact_t impact;
  az_circle_impact(
      state, baddie->data->main_body.bounding_radius, baddie->position,
      az_vmul(unit, 100000.0), (AZ_IMPF_BADDIE | AZ_IMPF_SHIP),
      baddie->uid, &impact);
  if (impact.type != AZ_IMP_NOTHING) {
    baddie->angle = az_angle_towards(baddie->angle, turn_rate * time,
                                     az_vtheta(impact.normal));
  }
}

/*===========================================================================*/

az_projectile_t *az_fire_baddie_projectile(
    az_space_state_t *state, az_baddie_t *baddie, az_proj_kind_t kind,
    double forward, double firing_angle, double proj_angle_offset) {
  const double theta = firing_angle + baddie->angle;
  return az_add_projectile(
      state, kind, true, az_vadd(baddie->position, az_vpolar(forward, theta)),
      theta + proj_angle_offset, 1.0);
}

/*===========================================================================*/
