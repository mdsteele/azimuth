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

#include "azimuth/tick/baddie_spiner.h"

#include <assert.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_bad_spine_mine(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SPINE_MINE);
  az_drift_towards_ship(state, baddie, time, 20, 20, 20);
  baddie->angle = az_mod2pi(baddie->angle - 0.5 * time);
  if (az_ship_in_range(state, baddie, 150) &&
      az_can_see_ship(state, baddie)) {
    for (int i = 0; i < 360; i += 20) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_SPINE,
          baddie->data->main_body.bounding_radius,
          AZ_DEG2RAD(i) + az_random(AZ_DEG2RAD(-10), AZ_DEG2RAD(10)), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_KILL_BOUNCER);
    baddie->kind = AZ_BAD_NOTHING;
  }
}

void az_tick_bad_spiner(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SPINER);
  az_drift_towards_ship(state, baddie, time, 40, 10, 100);
  baddie->angle = az_mod2pi(baddie->angle + 0.4 * time);
  if (baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 200) &&
      az_can_see_ship(state, baddie)) {
    for (int i = 0; i < 360; i += 45) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_SPINE,
          baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_SHRAPNEL_BURST);
    baddie->cooldown = 2.0;
  }
}

void az_tick_bad_super_spiner(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SUPER_SPINER);
  az_drift_towards_ship(state, baddie, time, 50, 20, 100);
  baddie->angle = az_mod2pi(baddie->angle - 0.5 * time);
  if (baddie->cooldown <= 0.0 &&
      az_ship_in_range(state, baddie, 200) &&
      az_can_see_ship(state, baddie)) {
    for (int i = 0; i < 360; i += 45) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_SPINE,
          baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_SHRAPNEL_BURST);
    baddie->cooldown = 1.0;
    baddie->state = 1;
  }
  if (baddie->state == 1 && baddie->cooldown <= 0.75) {
    for (int i = 0; i < 360; i += 45) {
      az_fire_baddie_projectile(
          state, baddie, AZ_PROJ_SPINE,
          baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i + 22.5), 0.0);
    }
    az_play_sound(&state->soundboard, AZ_SND_SHRAPNEL_BURST);
    baddie->cooldown = 1.0;
    baddie->state = 0;
  }
}

void az_on_super_spiner_killed(az_space_state_t *state, az_vector_t position,
                               double angle) {
  for (int i = 0; i < 3; ++i) {
    const double theta = angle + i * AZ_DEG2RAD(120);
    az_baddie_t *urchin =
      az_add_baddie(state, AZ_BAD_URCHIN,
                    az_vadd(position, az_vpolar(10, theta)), theta);
    if (urchin != NULL) urchin->velocity = az_vpolar(175, theta);
  }
}

void az_tick_bad_urchin(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_URCHIN);
  az_drift_towards_ship(state, baddie, time, 250, 300, 500);
  baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(50) * time);
}

/*===========================================================================*/
