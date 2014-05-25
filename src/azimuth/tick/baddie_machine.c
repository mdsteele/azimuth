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
      az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                           impact.target.wall->data->color1 :
                           AZ_WHITE), 1.0, impact.position,
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
                  AZ_IMPF_NONE, baddie->uid, &impact);
    if (impact.type == AZ_IMP_SHIP) {
      baddie->state = 1;
      const double beam_damage = 200.0 * time;
      az_damage_ship(state, beam_damage, false);
      // Add particles for the beam.
      const uint8_t alt = 32 * az_clock_zigzag(4, 1, state->clock);
      const az_color_t beam_color = {255, 255, 128 + alt, 192};
      az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                  4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
      az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                           impact.target.wall->data->color1 :
                           AZ_WHITE), 1.0, impact.position,
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

/*===========================================================================*/
