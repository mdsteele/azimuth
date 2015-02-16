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
#include "azimuth/state/object.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define HEAD_POPDOWN_STATE 0
#define HEAD_POPUP_STATE 1

// How fast the head pops up/down, in pixels/second:
#define HEAD_POP_SPEED 250
// How far above baseline the head pops up:
#define HEAD_POPUP_DIST 350

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

static az_baddie_t *lookup_legs(az_space_state_t *state, az_baddie_t *baddie,
                                int cargo_index) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  az_object_t object;
  if (az_lookup_object(state, baddie->cargo_uuids[cargo_index], &object) &&
      object.type == AZ_OBJ_BADDIE) {
    return object.obj.baddie;
  } else return NULL;
}

static void arrange_head_piston(az_baddie_t *baddie, int piston_index,
                                az_vector_t abs_foot_pos) {
  const az_vector_t rel_foot_pos =
    az_vrotate(az_vsub(abs_foot_pos, baddie->position), -baddie->angle);
  const double rel_angle = az_vtheta(rel_foot_pos);
  const az_vector_t rel_base_pos = az_vwithlen(rel_foot_pos, 60);
  const az_vector_t delta = az_vsub(rel_base_pos, rel_foot_pos);
  for (int i = 0; i < 5; ++i) {
    az_component_t *component = &baddie->components[5 * piston_index + i];
    component->position = az_vadd(rel_foot_pos, az_vmul(delta, 0.25 * i));
    component->angle = rel_angle;
  }
}

static void head_pop_up_down(az_space_state_t *state, az_baddie_t *baddie,
                             double delta) {
  az_shake_camera(&state->camera, 0.0, 1.0);
  const az_vector_t abs_foot_0_pos =
    az_vadd(az_vrotate(baddie->components[0].position, baddie->angle),
            baddie->position);
  const az_vector_t abs_foot_1_pos =
    az_vadd(az_vrotate(baddie->components[5].position, baddie->angle),
            baddie->position);
  az_vpluseq(&baddie->position, az_vwithlen(baddie->position, delta));
  arrange_head_piston(baddie, 0, abs_foot_0_pos);
  arrange_head_piston(baddie, 1, abs_foot_1_pos);
}

/*===========================================================================*/

void az_tick_bad_magbeest_head(az_space_state_t *state, az_baddie_t *baddie,
                               double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  az_baddie_t *legs_l = lookup_legs(state, baddie, 0);
  az_baddie_t *legs_r = lookup_legs(state, baddie, 1);
  if (legs_l == NULL || legs_l->kind != AZ_BAD_MAGBEEST_LEGS_L ||
      legs_r == NULL || legs_r->kind != AZ_BAD_MAGBEEST_LEGS_R) return;
  if (az_ship_is_decloaked(&state->ship)) {
    baddie->components[10].angle =
      az_mod2pi(az_vtheta(az_vsub(state->ship.position, baddie->position)) -
                baddie->angle);
  }
  const double baseline =
    az_current_camera_bounds(state)->min_r - AZ_SCREEN_HEIGHT/2;
  const double head_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case HEAD_POPDOWN_STATE:
      if (head_top > baseline) {
        head_pop_up_down(state, baddie, -HEAD_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      if (baddie->cooldown <= 0.0) {
        baddie->position = choose_random_marker_position(state);
        baddie->angle = az_vtheta(baddie->position);
        arrange_head_piston(baddie, 0, az_vadd(baddie->position,
            az_vpolar(50, baddie->angle + AZ_DEG2RAD(110))));
        arrange_head_piston(baddie, 1, az_vadd(baddie->position,
            az_vpolar(50, baddie->angle - AZ_DEG2RAD(110))));
        baddie->state = HEAD_POPUP_STATE;
        baddie->cooldown = 3.0;
      }
      break;
    case HEAD_POPUP_STATE:
      if (head_top < baseline + HEAD_POPUP_DIST) {
        head_pop_up_down(state, baddie, HEAD_POP_SPEED * time);
      }
      if (baddie->cooldown <= 0.0) {
        baddie->state = HEAD_POPDOWN_STATE;
        baddie->cooldown = 5.0;
      }
      break;
    default:
      baddie->state = HEAD_POPDOWN_STATE;
      break;
  }
  // TODO: control legs
  // TODO: laser sight
  // Magma bombs:
  if (false && baddie->cooldown <= 0.0) {
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
