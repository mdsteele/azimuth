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
#include "azimuth/util/random.h"
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
        az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
        baddie->cooldown = 2.2;
      }
    }
  }
}

void az_tick_bad_mosquito(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_MOSQUITO);
  baddie->angle = az_vtheta(baddie->velocity);
  const double speed = 200.0;
  const double turn = AZ_DEG2RAD(120) * time;
  bool chasing_ship = false;
  if (az_ship_in_range(state, baddie, 200)) {
    const double goal_theta =
      az_vtheta(az_vsub(state->ship.position, baddie->position));
    if (fabs(az_mod2pi(goal_theta - baddie->angle)) <= AZ_DEG2RAD(45) &&
        az_can_see_ship(state, baddie)) {
      baddie->angle = az_angle_towards(baddie->angle, turn, goal_theta);
      chasing_ship = true;
    }
  }
  if (!chasing_ship) {
    bool avoiding = false;
    AZ_ARRAY_LOOP(other, state->baddies) {
      if (other->kind != AZ_BAD_MOSQUITO || other == baddie) continue;
      if (az_vwithin(baddie->position, other->position,
                     2.0 * baddie->data->overall_bounding_radius)) {
        const double rel =
          az_mod2pi(az_vtheta(az_vsub(other->position, baddie->position)) -
                    baddie->angle);
        baddie->angle = az_mod2pi(baddie->angle + (rel < 0 ? turn : -turn));
        avoiding = true;
        break;
      }
    }
    if (!avoiding) {
      const double left =
        az_baddie_dist_to_wall(state, baddie, 2000, AZ_DEG2RAD(40));
      const double right =
        az_baddie_dist_to_wall(state, baddie, 2000, AZ_DEG2RAD(-40));
      baddie->angle = az_mod2pi(baddie->angle + (left > right ? turn : -turn));
    }
  }
  baddie->velocity = az_vpolar(speed, baddie->angle);
}

void az_tick_bad_dragonfly(az_space_state_t *state, az_baddie_t *baddie,
                           double time) {
  assert(baddie->kind == AZ_BAD_DRAGONFLY);
  az_fly_towards_ship(state, baddie, time,
                      5.0, 300.0, 300.0, 200.0, 0.0, 100.0);
  if (az_ship_is_alive(&state->ship) &&
      state->ship.temp_invincibility > 0.0) {
    baddie->cooldown = 2.0;
  }
}

void az_tick_bad_hornet(az_space_state_t *state, az_baddie_t *baddie,
                        double time) {
  assert(baddie->kind == AZ_BAD_HORNET);
  // Fire (when the ship is behind us):
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, AZ_PI, AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_STINGER,
                              15.0, AZ_PI, 0.0);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
    baddie->cooldown = 0.5;
  }
  // Chase ship if state 0, flee in state 1:
  az_fly_towards_ship(state, baddie, time, 8.0, 200.0, 300.0, 200.0,
                      0.0, (baddie->state == 0 ? 100.0 : -100.0));
  // Switch from state 0 to state 1 if we're close to the ship; switch from
  // state 1 to state 0 if we're far from the ship.
  if (baddie->state == 0) {
    if (az_ship_in_range(state, baddie, 200)) baddie->state = 1;
  } else if (baddie->state == 1) {
    if (!az_ship_in_range(state, baddie, 400)) baddie->state = 0;
  } else baddie->state = 0;
}

void az_tick_bad_super_hornet(az_space_state_t *state, az_baddie_t *baddie,
                              double time) {
  assert(baddie->kind == AZ_BAD_SUPER_HORNET);
  // Fire (when the ship is behind us):
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, AZ_PI, AZ_DEG2RAD(6)) &&
      az_can_see_ship(state, baddie)) {
    az_fire_baddie_projectile(state, baddie, AZ_PROJ_STINGER, 15.0, AZ_PI,
                              az_random(-AZ_DEG2RAD(5), AZ_DEG2RAD(5)));
    az_play_sound(&state->soundboard, AZ_SND_FIRE_STINGER);
    baddie->cooldown = 0.1;
  }
  // Chase ship if state 0, flee in state 1:
  az_fly_towards_ship(state, baddie, time, 8.0, 200.0, 300.0, 200.0,
                      0.0, (baddie->state == 0 ? 100.0 : -100.0));
  // Switch from state 0 to state 1 if we're close to the ship; switch from
  // state 1 to state 0 if we're far from the ship.
  if (baddie->state == 0) {
    if (az_ship_in_range(state, baddie, 200)) baddie->state = 1;
  } else if (baddie->state == 1) {
    if (!az_ship_in_range(state, baddie, 400)) baddie->state = 0;
  } else baddie->state = 0;
}

void az_tick_bad_switcher(az_space_state_t *state, az_baddie_t *baddie,
                          bool bounced) {
  assert(baddie->kind == AZ_BAD_SWITCHER);
  const double speed = 250.0;
  if (baddie->state == 0) {
    if (az_ship_is_decloaked(&state->ship) &&
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
