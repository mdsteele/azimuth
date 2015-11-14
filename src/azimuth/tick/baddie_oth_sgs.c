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

#include "azimuth/tick/baddie_oth_sgs.h"

#include <assert.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"

/*===========================================================================*/

#define TURN_RATE (AZ_DEG2RAD(285))
#define MAX_SPEED 300.0
#define FORWARD_ACCEL 300.0

/*===========================================================================*/

void az_tick_bad_oth_supergunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time, TURN_RATE, MAX_SPEED,
                      FORWARD_ACCEL, 200, 200, 100);
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
      az_can_see_ship(state, baddie)) {
    for (int i = -1; i <= 1; ++i) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                20.0, 0.0, AZ_DEG2RAD(10) * i);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
    baddie->cooldown = 0.2;
  }
  // TODO: implement Oth Supergunship
  az_tick_oth_tendrils(state, baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                       time);
}

void az_on_oth_supergunship_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (damage_kind & (AZ_DMGF_ROCKET | AZ_DMGF_BOMB)) {
    AZ_ARRAY_LOOP(decoy, state->baddies) {
      if (decoy->kind != AZ_BAD_OTH_DECOY) continue;
      az_kill_baddie(state, decoy);
    }
    double theta =
      az_vtheta(az_vrot90ccw(az_vsub(baddie->position, state->ship.position)));
    if (az_randint(0, 1)) theta = -theta;
    baddie->velocity = az_vpolar(250, theta);
    az_baddie_t *decoy = az_add_baddie(state, AZ_BAD_OTH_DECOY,
                                       baddie->position, baddie->angle);
    if (decoy != NULL) {
      decoy->velocity = az_vneg(baddie->velocity);
      decoy->cooldown = 0.5;
    }
  }
}

/*===========================================================================*/

void az_tick_bad_oth_decoy(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_DECOY);
  const double old_angle = baddie->angle;
  az_fly_towards_ship(state, baddie, time, TURN_RATE, MAX_SPEED,
                      FORWARD_ACCEL, 200, 200, 100);
  if (baddie->cooldown <= 0.0 &&
      az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
      az_can_see_ship(state, baddie)) {
    for (int i = -1; i <= 1; ++i) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                20.0, 0.0, AZ_DEG2RAD(10) * i);
    }
    az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
    baddie->cooldown = 0.2;
  }
  az_tick_oth_tendrils(state, baddie, &AZ_OTH_SUPERGUNSHIP_TENDRILS, old_angle,
                       time);
}

/*===========================================================================*/
