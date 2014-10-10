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

#include "azimuth/tick/baddie_machine.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void tick_sensor(az_space_state_t *state, az_baddie_t *baddie) {
  if (baddie->state == 0) {
    if (baddie->health < baddie->data->max_health) {
      baddie->state = 1;
      az_schedule_script(state, baddie->on_kill);
    }
  }
  baddie->health = baddie->data->max_health;
}

void az_tick_bad_gun_sensor(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_GUN_SENSOR);
  tick_sensor(state, baddie);
}

void az_tick_bad_beam_sensor(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BEAM_SENSOR);
  tick_sensor(state, baddie);
}

void az_tick_bad_beam_sensor_inv(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BEAM_SENSOR_INV);
  if (baddie->state != 1) {
    if (baddie->health < baddie->data->max_health) {
      baddie->state = 0;
    } else if (baddie->state == 0) {
      baddie->state = 2;
    } else {
      baddie->state = 1;
      az_schedule_script(state, baddie->on_kill);
    }
  }
  baddie->health = baddie->data->max_health;
}

void az_tick_bad_sensor_laser(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SENSOR_LASER);
  const az_vector_t beam_start =
    az_vadd(baddie->position, az_vpolar(6, baddie->angle));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                (az_ship_is_decloaked(&state->ship) ?
                 AZ_IMPF_NONE : AZ_IMPF_SHIP), baddie->uid, &impact);
  if (impact.type == AZ_IMP_BADDIE &&
      (impact.target.baddie.baddie->kind == AZ_BAD_BEAM_SENSOR ||
       impact.target.baddie.baddie->kind == AZ_BAD_BEAM_SENSOR_INV)) {
    az_try_damage_baddie(state, impact.target.baddie.baddie,
                         impact.target.baddie.component,
                         AZ_DMGF_BEAM, 0.01);
  }
  if (az_clock_mod(2, 2, state->clock)) {
    az_add_beam(state, (az_color_t){255, 128, 128, 128}, beam_start,
                impact.position, 0.0, 1.0);
  }
}

void az_tick_bad_heat_ray(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_HEAT_RAY);
  // State 0: Fire beam until cooldown expires.
  if (baddie->state == 0) {
    if (baddie->cooldown > 0.0) {
      // Fire a beam.
      double beam_damage = 38.0 * time;
      const az_vector_t beam_start =
        az_vadd(baddie->position, az_vpolar(15, baddie->angle));
      az_impact_t impact;
      az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                    AZ_IMPF_NONE, baddie->uid, &impact);
      if (impact.type == AZ_IMP_BADDIE) {
        if (impact.target.baddie.baddie->kind == AZ_BAD_BEAM_WALL) {
          beam_damage /= 6;
        }
        az_try_damage_baddie(state, impact.target.baddie.baddie,
                             impact.target.baddie.component,
                             AZ_DMGF_BEAM, beam_damage);
      } else if (impact.type == AZ_IMP_SHIP) {
        az_damage_ship(state, beam_damage, false);
      }
      // Add particles for the beam.
      const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
      const az_color_t beam_color = {255, 128, alt, 192};
      az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                  4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
      az_add_speck(state, AZ_WHITE, 1.0, impact.position,
                   az_vpolar(az_random(20.0, 70.0),
                             az_vtheta(impact.normal) +
                             az_random(-AZ_HALF_PI, AZ_HALF_PI)));
      az_loop_sound(&state->soundboard, AZ_SND_BEAM_FREEZE);
    } else {
      baddie->state = 1;
      baddie->cooldown = az_random(0.5, 3.0);
    }
  }
  // State 1: Recharge until cooldown expires.
  else {
    if (baddie->cooldown <= 0.0) {
      baddie->state = 0;
      baddie->cooldown = 1.5;
    }
  }
}

void az_tick_bad_death_ray(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_DEATH_RAY);
  if (baddie->state == 0 || baddie->state == 1) {
    // Fire a beam.
    const az_vector_t beam_start =
      az_vadd(baddie->position, az_vpolar(15, baddie->angle));
    az_impact_t impact;
    az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                  (az_ship_is_decloaked(&state->ship) ?
                   AZ_IMPF_NONE : AZ_IMPF_SHIP), baddie->uid, &impact);
    if (impact.type == AZ_IMP_SHIP) {
      baddie->state = 1;
      const double beam_damage = 200.0 * time;
      az_damage_ship(state, beam_damage, false);
      // Add particles for the beam.
      const uint8_t alt = 32 * az_clock_zigzag(4, 1, state->clock);
      const az_color_t beam_color = {255, 255, 128 + alt, 192};
      az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                  4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
      az_add_speck(state, AZ_WHITE, 1.0, impact.position,
                   az_vpolar(az_random(20.0, 70.0),
                             az_vtheta(impact.normal) +
                             az_random(-AZ_HALF_PI, AZ_HALF_PI)));
      az_loop_sound(&state->soundboard, AZ_SND_BEAM_PIERCE);
    } else {
      baddie->state = 0;
      if (az_clock_mod(2, 2, state->clock)) {
        const az_color_t beam_color = {255, 128, 128, 128};
        az_add_beam(state, beam_color, beam_start, impact.position, 0.0, 1.0);
      }
    }
  }
}

void az_tick_bad_boss_door(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_BOSS_DOOR);
  const double max_angle = AZ_DEG2RAD(50);
  const double beam_duration = 0.75;
  // Aim eye towards ship:
  if (baddie->state != 2) {
    const double turn_rate =
      (baddie->state == 3 ? AZ_DEG2RAD(40) : AZ_DEG2RAD(120));
    baddie->components[0].angle =
      fmax(-max_angle, fmin(max_angle, az_mod2pi(az_angle_towards(
        baddie->angle + baddie->components[0].angle, turn_rate * time,
        az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                                 baddie->angle)));
  }
  // States 0 and 1: Wait for an opportunity to fire at ship.
  if (baddie->state == 0 || baddie->state == 1) {
    const bool can_see_ship = az_ship_is_decloaked(&state->ship) &&
      (fabs(az_mod2pi(az_vtheta(az_vsub(
           state->ship.position, baddie->position)) -
                      baddie->angle)) <= max_angle) &&
      az_can_see_ship(state, baddie);
    const double eyelid_turn = AZ_DEG2RAD(360) * time;
    // State 0: Wait for ship to get in sight.
    if (baddie->state == 0) {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn, 0);
      baddie->param = fmax(0.0, baddie->param - 2.0 * time);
      if (can_see_ship && az_ship_in_range(state, baddie, 200)) {
        baddie->state = 1;
      }
    }
    // State 1: Open eyelids and charge up beam.
    else {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn,
                         max_angle + AZ_DEG2RAD(10));
      if (baddie->cooldown <= 0.0) {
        baddie->param = fmin(1.0, baddie->param + time);
      }
      if (!(can_see_ship && az_ship_in_range(state, baddie, 300))) {
        baddie->state = 0;
      } else if (baddie->param >= 1.0 &&
                 baddie->components[1].angle >= max_angle) {
        baddie->state = 2;
        baddie->cooldown = 0.35;
        az_play_sound(&state->soundboard, AZ_SND_CHARGED_MISSILE_BEAM);
      }
    }
    baddie->components[2].angle = -AZ_HALF_PI - baddie->components[1].angle;
  }
  // State 2: Pause for a short time before firing.
  else if (baddie->state == 2) {
    if (baddie->cooldown <= 0.0) {
      baddie->state = 3;
      baddie->cooldown = beam_duration;
      az_play_sound(&state->soundboard, AZ_SND_FIRE_MISSILE_BEAM);
    }
  }
  // State 3: Fire beam until cooldown expires.
  else {
    baddie->param = baddie->cooldown / beam_duration;
    // Fire a beam.
    const double beam_damage = 25.0 * time / beam_duration;
    const double beam_angle = baddie->angle + baddie->components[0].angle;
    const az_vector_t beam_start =
      az_vadd(baddie->position, az_vpolar(15, beam_angle));
    az_impact_t impact;
    az_ray_impact(state, beam_start, az_vpolar(5000, beam_angle),
                  AZ_IMPF_NONE, baddie->uid, &impact);
    if (impact.type == AZ_IMP_BADDIE) {
      az_try_damage_baddie(state, impact.target.baddie.baddie,
          impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
    } else if (impact.type == AZ_IMP_SHIP) {
      az_damage_ship(state, beam_damage, false);
    }
    // Add particles for the beam.
    const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
    const az_color_t beam_color = {255, alt, 128, 192};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                (6.0 + 0.5 * az_clock_zigzag(8, 1, state->clock)) *
                baddie->cooldown);
    az_add_speck(state, AZ_WHITE, 1.0, impact.position,
                 az_vpolar(az_random(20.0, 70.0),
                           az_vtheta(impact.normal) +
                           az_random(-AZ_HALF_PI, AZ_HALF_PI)));
    // When the cooldown timer reachers zero, stop firing the beam.
    if (baddie->cooldown <= 0.0) {
      baddie->state = 1;
      baddie->cooldown = 1.0;
    }
  }
}

/*===========================================================================*/
