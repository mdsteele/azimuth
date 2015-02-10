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

#include "azimuth/tick/baddie_magbeest.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_magbeest_head(az_space_state_t *state, az_baddie_t *baddie,
                               double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  // TODO: control legs
  // TODO: laser sight
  // TODO: magma bombs
}

void az_tick_bad_magbeest_legs_l(az_space_state_t *state, az_baddie_t *baddie,
                                 double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L);
  // TODO: pull in scrap with magnet, then fling it back out
  // TODO: fusion beam
}

void az_tick_bad_magbeest_legs_r(az_space_state_t *state, az_baddie_t *baddie,
                                 double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  az_component_t *gun = &baddie->components[0];
  // Aim the gatling gun at the ship (leading the target if possible).
  if (az_ship_is_decloaked(&state->ship)) {
    const az_vector_t gun_abs_position =
      az_vadd(az_vrotate(gun->position, baddie->angle), baddie->position);
    az_vector_t goal = state->ship.position;
    if (az_lead_target(az_vsub(state->ship.position, gun_abs_position),
                        state->ship.velocity, 600.0, &goal)) {
      az_vpluseq(&goal, gun_abs_position);
    }
    gun->angle = az_mod2pi(
        az_angle_towards(gun->angle + baddie->angle, AZ_DEG2RAD(60) * time,
                         az_vtheta(az_vsub(goal, gun_abs_position))) -
        baddie->angle);
  }
  // Fire gatling salvos periodicly.
  if (baddie->state == 0) {
    baddie->state = 28;
    baddie->cooldown = 2.0;
  } else if (baddie->cooldown <= 0) {
    const az_vector_t fire_from =
      {55, 2 * (az_clock_zigzag(6, 1, baddie->state) - 3)};
    az_add_projectile(
      state, AZ_PROJ_LASER_PULSE,
      az_vadd(az_vrotate(az_vadd(az_vrotate(fire_from, gun->angle),
                                 gun->position),
                         baddie->angle),
              baddie->position),
      baddie->angle + gun->angle, 1.0, baddie->uid);
    az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
    --baddie->state;
    baddie->cooldown = 0.1;
  }
  // Spin the gatling gun when we're firing.
  if (baddie->cooldown <= 0.1) {
    baddie->param = az_mod2pi(baddie->param + 2 * AZ_TWO_PI * time);
  }
}

/*===========================================================================*/
