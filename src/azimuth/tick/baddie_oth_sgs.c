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
#include <math.h>

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

#define TRACTOR_MIN_LENGTH 100.0
#define TRACTOR_MAX_LENGTH 300.0
#define TRACTOR_COMPONENT_INDEX 6
#define TRACTOR_CLOAK_CHARGE_TIME 2.5
#define TRACTOR_CLOAK_DECAY_TIME 5.0

static bool tractor_locked_on(
    const az_baddie_t *baddie, az_vector_t *tractor_pos_out,
    double *tractor_length_out) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const az_component_t *component =
    &baddie->components[TRACTOR_COMPONENT_INDEX];
  const double tractor_length = component->angle;
  if (tractor_length == 0) return false;
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  if (tractor_pos_out != NULL) *tractor_pos_out = component->position;
  if (tractor_length_out != NULL) *tractor_length_out = tractor_length;
  return true;
}

static void set_tractor_node(az_baddie_t *baddie, const az_node_t *node) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (node == NULL) {
    baddie->components[TRACTOR_COMPONENT_INDEX].angle = 0;
    return;
  }
  assert(node->kind == AZ_NODE_TRACTOR);
  const double tractor_length = az_vdist(baddie->position, node->position);
  assert(tractor_length >= TRACTOR_MIN_LENGTH &&
         tractor_length <= TRACTOR_MAX_LENGTH);
  baddie->components[TRACTOR_COMPONENT_INDEX].position = node->position;
  baddie->components[TRACTOR_COMPONENT_INDEX].angle = tractor_length;
}

static void decloak_immediately(az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  if (baddie->param > 1.0) {
    baddie->param = 1.0;
    az_play_sound(&state->soundboard, AZ_SND_CLOAK_END);
  }
}

/*===========================================================================*/

void az_tick_bad_oth_supergunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_SUPERGUNSHIP);
  const double old_angle = baddie->angle;

  // Handle tractor beam:
  az_vector_t tractor_position;
  double tractor_length;
  if (tractor_locked_on(baddie, &tractor_position, &tractor_length)) {
    baddie->position = az_vadd(az_vwithlen(az_vsub(baddie->position,
                                                   tractor_position),
                                           tractor_length),
                               tractor_position);
    baddie->velocity = az_vflatten(baddie->velocity,
        az_vsub(baddie->position, tractor_position));
    az_loop_sound(&state->soundboard, AZ_SND_TRACTOR_BEAM);
    baddie->param += time / TRACTOR_CLOAK_CHARGE_TIME;
    if (baddie->param >= 1.0) {
      az_play_sound(&state->soundboard, AZ_SND_CLOAK_BEGIN);
      baddie->param = 1.0 + TRACTOR_CLOAK_DECAY_TIME;
      set_tractor_node(baddie, NULL);
    }
  } else if (baddie->param == 0.0) {
    az_node_t *nearest = NULL;
    double best_dist = INFINITY;
    AZ_ARRAY_LOOP(node, state->nodes) {
      if (node->kind != AZ_NODE_TRACTOR) continue;
      const double dist = az_vdist(baddie->position, node->position);
      if (dist >= TRACTOR_MIN_LENGTH && dist < best_dist) {
        nearest = node;
        best_dist = dist;
      }
    }
    if (nearest != NULL && best_dist <= TRACTOR_MAX_LENGTH) {
      set_tractor_node(baddie, nearest);
    }
  } else {
    const double old_param = baddie->param;
    baddie->param = fmax(0.0, baddie->param - time);
    if (old_param > 1.0 && baddie->param <= 1.0) {
      az_play_sound(&state->soundboard, AZ_SND_CLOAK_END);
    }
  }
  if (baddie->param >= 1.0) {
    baddie->temp_properties |= AZ_BADF_NO_HOMING;
  }

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
    decloak_immediately(state, baddie);
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
    decloak_immediately(state, baddie);
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
