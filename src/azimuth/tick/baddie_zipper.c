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

#include "azimuth/tick/baddie_zipper.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_zipper(az_space_state_t *state, az_baddie_t *baddie,
                        bool bounced) {
  assert(baddie->kind == AZ_BAD_ZIPPER ||
         baddie->kind == AZ_BAD_ARMORED_ZIPPER ||
         baddie->kind == AZ_BAD_MINI_ARMORED_ZIPPER);
  if (bounced) baddie->angle = az_mod2pi(baddie->angle + AZ_PI);
  baddie->velocity =
    az_vpolar((baddie->kind == AZ_BAD_MINI_ARMORED_ZIPPER ? 250.0 : 200.0),
              baddie->angle);
 }

void az_tick_bad_fire_zipper(az_space_state_t *state, az_baddie_t *baddie,
                             bool bounced) {
  assert(baddie->kind == AZ_BAD_FIRE_ZIPPER);
  // Bounce back and forth.
  if (bounced) baddie->angle = az_mod2pi(baddie->angle + AZ_PI);
  baddie->velocity = az_vpolar(150.0 - 25.0 * baddie->cooldown,
                               baddie->angle);
  // Shoot fireballs as we pass by the ship.
  if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 300)) {
    const az_vector_t rel_position =
      az_vsub(state->ship.position, baddie->position);
    if (az_vdot(rel_position, baddie->velocity) <= 0.0 &&
        az_can_see_ship(state, baddie)) {
      az_vector_t rel_impact;
      if (az_lead_target(rel_position, state->ship.velocity, 260,
                         &rel_impact)) {
        const double center_angle = az_vtheta(rel_impact) - baddie->angle;
        for (int i = -1; i <= 1; ++i) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_FIREBALL_SLOW,
                                    0.0, center_angle, i * AZ_DEG2RAD(10));
        }
        baddie->cooldown = 2.2;
      }
    }
  }
}

void az_tick_bad_switcher(az_space_state_t *state, az_baddie_t *baddie,
                          bool bounced) {
  const double speed = 250.0;
  if (baddie->state == 0) {
    if (az_ship_is_present(&state->ship) &&
        az_vnonzero(state->ship.velocity)) {
      az_impact_t impact;
      az_circle_impact(
          state, baddie->data->overall_bounding_radius,
          baddie->position, az_vpolar(5000.0, baddie->angle),
          (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
      az_vector_t intersect_pos;
      if (az_ray_hits_line_segment(
              baddie->position, impact.position, state->ship.position,
              az_vmul(state->ship.velocity, 10.0), &intersect_pos, NULL)) {
        const double intersect_time =
          az_vdist(state->ship.position, intersect_pos) /
          az_vnorm(state->ship.velocity);
        const double travel_time =
          az_vdist(baddie->position, intersect_pos) / speed;
        if (fabs(travel_time - intersect_time) <= 0.1) {
          baddie->velocity = az_vpolar(speed, baddie->angle);
          baddie->state = 1;
        }
      }
    }
  } else if (bounced) {
    baddie->velocity = AZ_VZERO;
    baddie->angle = az_mod2pi(baddie->angle + AZ_PI);
    baddie->state = 0;
  }
}

/*===========================================================================*/
