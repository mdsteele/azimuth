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
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static az_vector_t choose_random_marker_position(az_space_state_t *state) {
  int num_markers = 0;
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_MARKER) ++num_markers;
  }
  if (num_markers == 0) return AZ_VZERO;
  int marker_index = az_randint(1, num_markers);
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind != AZ_NODE_MARKER) continue;
    if (--marker_index <= 0) return node->position;
  }
  AZ_ASSERT_UNREACHABLE();
}

void az_tick_bad_magbeest_head(az_space_state_t *state, az_baddie_t *baddie,
                               double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  // TODO: control legs
  // TODO: laser sight
  // Magma bombs:
  if (baddie->cooldown <= 0.0) {
    baddie->cooldown = 3.0;
    const az_vector_t target = choose_random_marker_position(state);
    az_baddie_t *bomb = az_add_baddie(state, AZ_BAD_MAGMA_BOMB,
                                      baddie->position, baddie->angle);
    if (bomb != NULL) {
      const az_vector_t delta = az_vsub(target, baddie->position);
      bomb->velocity = az_vadd(az_vwithlen(delta, 400.0),
                               az_vwithlen(baddie->position,
                                           0.4 * az_vnorm(delta)));
    }
  }
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
    baddie->cooldown = 7.0;
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

void az_tick_bad_magma_bomb(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_MAGMA_BOMB);
  baddie->angle = az_mod2pi(baddie->angle +
      copysign(0.02, az_vcross(baddie->position, baddie->velocity)) *
      time * az_vnorm(baddie->velocity));
  // Check if we're in a lava pool:
  az_gravfield_t *lava = NULL;
  AZ_ARRAY_LOOP(gravfield, state->gravfields) {
    if (gravfield->kind != AZ_GRAV_LAVA) continue;
    if (az_point_within_gravfield(gravfield, baddie->position)) {
      lava = gravfield;
      break;
    }
  }
  // Apply forces:
  const double gravity_accel = 200.0;
  const az_vector_t gravity_force =
    az_vwithlen(baddie->position, -gravity_accel);
  const double max_speed = (lava != NULL ? 80.0 : 1000.0);
  const double drag_coeff = gravity_accel / (max_speed * max_speed);
  const az_vector_t drag_force =
    az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
  az_vpluseq(&baddie->velocity,
             az_vmul(az_vadd(gravity_force, drag_force), time));
  // Countdown:
  if (baddie->state == 0) {
    baddie->state = 1;
    baddie->cooldown = 4.0;
  } else if (baddie->cooldown <= 0.0) {
    az_add_projectile(state, AZ_PROJ_MAGMA_EXPLOSION, baddie->position,
                      az_vtheta(baddie->position), 1.0, AZ_NULL_UID);
    if (lava != NULL) {
      for (int i = -4; i < 4; ++i) {
        az_impact_t impact;
        az_ray_impact(state, baddie->position,
                      az_vpolar(1000.0, az_vtheta(baddie->position) +
                                i * AZ_DEG2RAD(20)),
                      (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
        az_vector_t point, normal;
        if (az_ray_hits_liquid_surface(
                lava, baddie->position,
                az_vsub(impact.position, baddie->position), &point, &normal)) {
          az_add_projectile(state, AZ_PROJ_ERUPTION, point,
                            az_vtheta(az_vneg(normal)), 1.0, AZ_NULL_UID);
        }
      }
    }
    az_kill_baddie(state, baddie);
  }
}

/*===========================================================================*/
