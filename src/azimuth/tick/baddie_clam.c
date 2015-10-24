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

#include "azimuth/tick/baddie_clam.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_clam(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_CLAM);
  const bool can_see = az_can_see_ship(state, baddie);
  const double ship_theta =
    az_vtheta(az_vsub(state->ship.position, baddie->position));
  if (can_see) {
    baddie->angle = az_angle_towards(baddie->angle, 0.2 * time, ship_theta);
  }
  const double max_angle = AZ_DEG2RAD(40);
  const double old_angle = baddie->components[0].angle;
  const double new_angle =
    (baddie->cooldown > 2.5 ||
     (baddie->cooldown <= 0.0 && can_see &&
      az_ship_in_range(state, baddie, 450.0) &&
      fabs(az_mod2pi(baddie->angle - ship_theta)) < AZ_DEG2RAD(10)) ?
     fmin(max_angle,
          max_angle - (max_angle - old_angle) * pow(0.00003, time)) :
     fmax(0.0, old_angle - 1.0 * time));
  baddie->components[0].angle = new_angle;
  baddie->components[1].angle = -new_angle;
  if (baddie->cooldown <= 0.0 && new_angle >= 0.95 * max_angle) {
    for (int i = -1; i <= 1; ++i) {
      az_fire_baddie_projectile(state, baddie,
          (i == 0 ? AZ_PROJ_FIREBALL_SLOW : AZ_PROJ_FIREBALL_FAST),
          0.5 * baddie->data->main_body.bounding_radius,
          AZ_DEG2RAD(i * 5), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_FIREBALL);
    baddie->cooldown = 3.0;
  }
}

/*===========================================================================*/

void az_tick_bad_grabber_plant(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_GRABBER_PLANT);
  const double max_jaw_angle = AZ_DEG2RAD(70);
  const double tongue_range = 250.0;
  const double tongue_max_length = 300.0;
  const double tongue_max_pull_time = 5.0;
  const double tongue_speed = 800.0;
  az_component_t *tongue = &baddie->components[9];
  bool open_jaws = true;
  // State 0: Wait for ship.
  if (baddie->state == 0) {
    if (baddie->cooldown <= 0.5 &&
        az_ship_in_range(state, baddie, tongue_range) &&
        az_can_see_ship(state, baddie)) {
      az_vector_t rel_impact;
      if (baddie->cooldown <= 0.0 && state->ship.temp_invincibility <= 0.0 &&
          az_ship_within_angle(state, baddie, 0.0,
                               baddie->components[0].angle) &&
          az_lead_target(az_vsub(state->ship.position, baddie->position),
                         az_vsub(state->ship.velocity, baddie->velocity),
                         tongue_speed, &rel_impact)) {
        tongue->position = AZ_VZERO;
        tongue->angle = az_mod2pi(az_vtheta(rel_impact) - baddie->angle);
        // TODO: Play sound for shooting tongue
        baddie->state = 1;
      }
    } else open_jaws = false;
  }
  // State 1: Shoot tongue out.
  else if (baddie->state == 1) {
    const az_vector_t abs_tongue_position =
      az_vadd(az_vrotate(tongue->position, baddie->angle),
              baddie->position);
    const double abs_tongue_angle =
      az_mod2pi(tongue->angle + baddie->angle);
    az_impact_t impact;
    az_ray_impact(state, abs_tongue_position,
                  az_vpolar(tongue_speed * time, abs_tongue_angle),
                  AZ_IMPF_BADDIE, baddie->uid, &impact);
    tongue->position =
      az_vrotate(az_vsub(impact.position, baddie->position),
                 -baddie->angle);
    if (impact.type == AZ_IMP_SHIP) {
      // TODO: Play sound for tongue impact
      baddie->state = 2;
      baddie->cooldown = tongue_max_pull_time;
    } else if (impact.type != AZ_IMP_NOTHING ||
               !az_vwithin(tongue->position, AZ_VZERO,
                           tongue_max_length)) {
      baddie->state = 3;
    }
  }
  // State 2: Pull ship in.
  else if (baddie->state == 2) {
    if (baddie->cooldown > 0.0 && state->ship.temp_invincibility <= 0.0 &&
        az_ship_in_range(state, baddie, tongue_max_length) &&
        !az_ship_in_range(state, baddie, 30.0) &&
        az_ship_within_angle(state, baddie, 0.0, max_jaw_angle) &&
        az_can_see_ship(state, baddie)) {
      tongue->position =
        az_vrotate(az_vsub(state->ship.position, baddie->position),
                   -baddie->angle);
      tongue->angle = az_vtheta(tongue->position);
      const az_vector_t delta =
        az_vsub(baddie->position, state->ship.position);
      const double accel =
        (450.0 * az_vnorm(delta) / tongue_max_length +
         260.0 * sqrt(baddie->cooldown / tongue_max_pull_time));
      az_vpluseq(&state->ship.velocity, az_vwithlen(delta, accel * time));
    } else baddie->state = 3;
  }
  // State 3: Retract tongue.
  else if (baddie->state == 3) {
    const double new_tongue_length =
      az_vnorm(tongue->position) - tongue_speed * time;
    if (new_tongue_length <= 0.0) {
      tongue->position = AZ_VZERO;
      tongue->angle = 0.0;
      baddie->state = 0;
      baddie->cooldown = 2.0;
    } else {
      tongue->position = az_vwithlen(tongue->position, new_tongue_length);
    }
  } else baddie->state = 0;
  // Open/close the jaws.
  const double old_angle = baddie->components[0].angle;
  const double new_angle =
    (open_jaws ? fmin(max_jaw_angle, old_angle + AZ_DEG2RAD(320) * time) :
     fmax(0.0, old_angle - AZ_DEG2RAD(740) * time));
  baddie->components[0].angle = new_angle;
  baddie->components[1].angle = -new_angle;
}

void az_on_grabber_plant_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  assert(baddie->kind == AZ_BAD_GRABBER_PLANT);
  // If the grabber gets frozen while the tongue is out, retract the tongue.
  if (baddie->frozen > 0 && baddie->state != 0) {
    baddie->state = 3;
  }
}

/*===========================================================================*/
