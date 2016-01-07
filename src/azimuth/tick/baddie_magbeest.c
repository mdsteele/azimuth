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
#include <stdbool.h>
#include <stdlib.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/object.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

// Head states:
enum {
  HEAD_INIT_STATE = 0,
  HEAD_DORMANT_STATE,
  HEAD_POPDOWN_STATE,
  HEAD_POPUP_STATE,
  HEAD_LASER_SEEK_STATE,
  HEAD_WAITING_STATE,
  HEAD_QUICK_POPUP_STATE,
  HEAD_QUICK_POPDOWN_STATE,
  HEAD_SPIDER_WAIT_TO_POPUP_STATE,
  HEAD_SPIDER_POPUP_STATE,
  HEAD_SPIDER_PLANT_LEG_0_STATE,
  HEAD_SPIDER_PLANT_LEG_1_STATE,
  HEAD_SPIDER_PLANT_LEG_2_STATE,
  HEAD_SPIDER_PLANT_LEG_3_STATE,
  HEAD_WALL_SPIDER_MOVE_BODY_STATE,
  HEAD_WALL_SPIDER_MOVE_EVEN_LEGS_UP_STATE,
  HEAD_WALL_SPIDER_MOVE_ODD_LEGS_UP_STATE,
  HEAD_CORNER_SPIDER_MOVE_BODY_STATE,
  HEAD_CORNER_SPIDER_MOVE_EVEN_LEGS_UP_STATE,
  HEAD_CORNER_SPIDER_MOVE_ODD_LEGS_UP_STATE,
  HEAD_CEILING_SPIDER_MOVE_BODY_STATE,
  HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_LEFT_STATE,
  HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_LEFT_STATE,
  HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_RIGHT_STATE,
  HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_RIGHT_STATE
};
// Shared legs states:
#define LEGS_DORMANT_STATE 0
#define LEGS_POPDOWN_STATE 1
#define LEGS_POPUP_STATE 2
// Legs-L states:
#define MAGNET_COOLDOWN_STATE 3
#define MAGNET_RECOVER_STATE 4
#define MAGNET_FUSION_BEAM_CHARGE_STATE 5
#define MAGNET_FINISH_ATTRACT_STATE 6
#define MAGNET_MEDIUM_SPAWN_STATE (MAGNET_FINISH_ATTRACT_STATE + 10)
#define MAGNET_START_SPAWN_STATE (MAGNET_FINISH_ATTRACT_STATE + 20)
// Legs-R states:
#define GATLING_COOLDOWN_STATE 3
#define GATLING_STOP_FIRING_STATE 4
#define GATLING_MID_FIRING_STATE (GATLING_STOP_FIRING_STATE + 14)
#define GATLING_START_FIRING_STATE (GATLING_STOP_FIRING_STATE + 28)

// How fast the head pops up/down, in pixels/second:
#define HEAD_POP_SPEED 250
// How far above baseline the head pops up:
#define HEAD_POPUP_DIST 350
// How fast the legs pop up/down, in pixels/second:
#define LEGS_POP_SPEED 200
// How far above baseline the legs pop up:
#define LEGS_POPUP_DIST 320
// How long it takes the fusion beam to fire, in seconds:
#define MAGNET_FUSION_BEAM_CHARGE_TIME 3.0
// How long it takes to shift a leg when beginning spider mode:
#define SPIDER_PLANT_LEG_TIME 0.5
// How long it takes to shift a pair of legs when in spider mode:
#define SPIDER_MOVE_LEGS_TIME 0.5
// How fast a spider foot can move, in pixels/second:
#define SPIDER_FOOT_SPEED 300
// How fast the spider body can move, in pixels/second:
#define SPIDER_BODY_SPEED 75

typedef enum {
  WALL_SPIDER,
  CORNER_SPIDER,
  CEILING_SPIDER
} az_spider_mode_t;

/*===========================================================================*/

// Comparison function for use with qsort.  Sorts az_vector_t structs by their
// theta values.
static int compare_thetas(const void *v1, const void *v2) {
  const double t1 = az_vtheta(*(az_vector_t*)v1);
  const double t2 = az_vtheta(*(az_vector_t*)v2);
  return (t1 < t2 ? 1 : t1 > t2 ? -1 : 0);
}

static void get_marker_positions(
    az_space_state_t *state, az_vector_t positions_out[], int num_positions) {
  int i = 0;
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind != AZ_NODE_MARKER) continue;
    positions_out[i++] = node->position;
    if (i >= num_positions) break;
  }
  while (i < num_positions) positions_out[i++] = AZ_VZERO;
}

static void sort_marker_positions(
    az_space_state_t *state, az_vector_t positions_out[], int num_positions) {
  get_marker_positions(state, positions_out, num_positions);
  qsort(positions_out, num_positions, sizeof(az_vector_t), compare_thetas);
}

static void shuffle_marker_positions(
    az_space_state_t *state, az_vector_t positions_out[], int num_positions) {
  get_marker_positions(state, positions_out, num_positions);
  for (int i = num_positions - 1; i > 0; --i) {
    const int j = az_randint(0, i);
    const az_vector_t tmp = positions_out[i];
    positions_out[i] = positions_out[j];
    positions_out[j] = tmp;
  }
}

static az_vector_t raycast(az_space_state_t *state, az_vector_t start,
                           double theta) {
  az_impact_t impact;
  az_ray_impact(state, start, az_vpolar(10000, theta),
                (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), AZ_NULL_UID, &impact);
  return impact.position;
}

/*===========================================================================*/

static void on_leg_stomp(az_space_state_t *state) {
  az_play_sound(&state->soundboard, AZ_SND_MAGBEEST_LEG_STOMP);
  az_shake_camera(&state->camera, 0.0, 2.0);
}

static void on_tunnelling(az_space_state_t *state) {
  az_loop_sound(&state->soundboard, AZ_SND_MAGBEEST_TUNNELLING);
  az_shake_camera(&state->camera, 0.0, 1.0);
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

static void turn_eye_towards(az_space_state_t *state, az_baddie_t *baddie,
                             az_vector_t goal, double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  const double turn_rate = AZ_DEG2RAD(45);
  baddie->components[10].angle =
    az_mod2pi(az_angle_towards(
        baddie->angle + baddie->components[10].angle, turn_rate * time,
        az_vtheta(az_vsub(goal, baddie->position))) - baddie->angle);
}

static void turn_eye_towards_ship(az_space_state_t *state, az_baddie_t *baddie,
                                  double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  if (az_ship_is_decloaked(&state->ship)) {
    turn_eye_towards(state, baddie, state->ship.position, time);
  }
}

static void arrange_head_piston(az_baddie_t *baddie, int piston_index,
                                az_vector_t abs_foot_pos) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
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
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  on_tunnelling(state);
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

static void init_head_popup(az_baddie_t *baddie, az_vector_t marker_position) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  baddie->position = marker_position;
  baddie->angle = az_vtheta(baddie->position);
  arrange_head_piston(baddie, 0, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle + AZ_DEG2RAD(110))));
  arrange_head_piston(baddie, 1, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle - AZ_DEG2RAD(110))));
}

static bool head_laser_sight(az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  const double beam_theta = baddie->angle + baddie->components[10].angle;
  const az_vector_t beam_start =
    az_vadd(baddie->position, az_vpolar(15, beam_theta));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(10000, beam_theta),
                (az_ship_is_decloaked(&state->ship) ?
                 AZ_IMPF_NONE : AZ_IMPF_SHIP), baddie->uid, &impact);
  if (az_clock_mod(2, 2, state->clock)) {
    const az_color_t beam_color = {255, 128, 128, 48};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0, 1.0);
  }
  return (impact.type == AZ_IMP_SHIP);
}

static void arrange_leg(az_baddie_t *baddie, int leg_index,
                        az_vector_t abs_foot_pos, double abs_knee_theta) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  assert(leg_index == 0 || leg_index == 1);
  az_component_t *thigh = &baddie->components[1 + 2 * leg_index];
  az_component_t *shin = &baddie->components[2 + 2 * leg_index];
  const double thigh_length =
    (baddie->kind == AZ_BAD_MAGBEEST_LEGS_L) ^ (leg_index == 0) ? 100 : 180;
  const double shin_length =
    (baddie->kind == AZ_BAD_MAGBEEST_LEGS_L) ^ (leg_index == 0) ? 180 : 100;
  const az_vector_t rel_foot_pos =
    az_vrotate(az_vsub(abs_foot_pos, baddie->position), -baddie->angle);
  const az_vector_t rel_hip_pos = thigh->position;
  const az_vector_t rel_knee_pos =
    az_find_knee(rel_hip_pos, rel_foot_pos, thigh_length, shin_length,
                 az_vpolar(1, abs_knee_theta - baddie->angle));
  shin->position = rel_foot_pos;
  shin->angle = az_vtheta(az_vsub(rel_knee_pos, rel_foot_pos));
  thigh->angle = az_vtheta(az_vsub(rel_knee_pos, rel_hip_pos));
}

static void legs_pop_up_down(az_space_state_t *state, az_baddie_t *baddie,
                             double baseline, double delta) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  on_tunnelling(state);
  az_vpluseq(&baddie->position, az_vwithlen(baddie->position, delta));
  const double feet_rho = 0.5 * (az_vnorm(baddie->position) - baseline) +
    baseline - 0.505 * LEGS_POPUP_DIST;
  const az_vector_t unit_left = az_vunit(az_vrot90ccw(baddie->position));
  arrange_leg(baddie, 0, az_vwithlen(az_vadd(baddie->position,
                                             az_vmul(unit_left, -40)),
                                     feet_rho), baddie->angle);
  arrange_leg(baddie, 1, az_vwithlen(az_vadd(baddie->position,
                                             az_vmul(unit_left, 40)),
                                     feet_rho), baddie->angle);
}

static void init_legs_popup(az_baddie_t *baddie, az_vector_t marker_position) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  baddie->position = marker_position;
  baddie->angle = az_vtheta(baddie->position);
  arrange_leg(baddie, 0, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle - AZ_DEG2RAD(135))), baddie->angle);
  arrange_leg(baddie, 1, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle + AZ_DEG2RAD(135))), baddie->angle);
  baddie->state = LEGS_POPUP_STATE;
  baddie->cooldown = 2.0;
}

static az_vector_t get_abs_foot_position(az_baddie_t *legs_l,
                                         az_baddie_t *legs_r, int leg_index) {
  assert(legs_l->kind == AZ_BAD_MAGBEEST_LEGS_L);
  assert(legs_r->kind == AZ_BAD_MAGBEEST_LEGS_R);
  assert(leg_index >= 0 && leg_index < 4);
  az_baddie_t *legs = (leg_index < 2 ? legs_r : legs_l);
  const int component_index = (leg_index % 2 ? 2 : 4);
  return az_vadd(az_vrotate(legs->components[component_index].position,
                            legs->angle), legs->position);
}

void az_arrange_spider_leg(az_baddie_t *legs_l, az_baddie_t *legs_r,
                           int leg_index, az_vector_t abs_foot_pos) {
  assert(legs_l->kind == AZ_BAD_MAGBEEST_LEGS_L);
  assert(legs_r->kind == AZ_BAD_MAGBEEST_LEGS_R);
  assert(leg_index >= 0 && leg_index < 4);
  switch (leg_index) {
    case 0: arrange_leg(legs_r, 1, abs_foot_pos, legs_l->angle); break;
    case 1: arrange_leg(legs_r, 0, abs_foot_pos, legs_l->angle); break;
    case 2: arrange_leg(legs_l, 1, abs_foot_pos, legs_r->angle); break;
    case 3: arrange_leg(legs_l, 0, abs_foot_pos, legs_r->angle); break;
  }
}

static void move_spider_foot_towards(
    az_baddie_t *head, az_baddie_t *legs_l, az_baddie_t *legs_r, int leg_index,
    az_vector_t goal, double time) {
  assert(head->kind == AZ_BAD_MAGBEEST_HEAD);
  assert(legs_l->kind == AZ_BAD_MAGBEEST_LEGS_L);
  assert(legs_r->kind == AZ_BAD_MAGBEEST_LEGS_R);
  const az_vector_t old_position =
    get_abs_foot_position(legs_l, legs_r, leg_index);
  const az_vector_t new_position = az_vadd(old_position, az_vcaplen(
      az_vsub(goal, old_position),
      SPIDER_FOOT_SPEED * time * (head->param > 0.5 ? 2 : 1)));
  az_arrange_spider_leg(legs_l, legs_r, leg_index, new_position);
}

static void shift_spider_body(az_baddie_t *head, az_baddie_t *legs_l,
                              az_baddie_t *legs_r, az_vector_t delta) {
  assert(head->kind == AZ_BAD_MAGBEEST_HEAD);
  assert(legs_l->kind == AZ_BAD_MAGBEEST_LEGS_L);
  assert(legs_r->kind == AZ_BAD_MAGBEEST_LEGS_R);
  az_vector_t abs_foot_positions[4];
  for (int i = 0; i < 4; ++i) {
    abs_foot_positions[i] = get_abs_foot_position(legs_l, legs_r, i);
  }
  const double towards_wall =
    az_mod2pi(az_vtheta(az_vsub(abs_foot_positions[3],
                                abs_foot_positions[0])) + AZ_HALF_PI);
  az_vpluseq(&head->position, delta);
  head->angle = az_mod2pi(towards_wall + AZ_DEG2RAD(45));
  legs_r->position = az_vadd(head->position,
                             az_vpolar(50, towards_wall + AZ_DEG2RAD(45)));
  legs_r->angle = az_mod2pi(towards_wall - AZ_DEG2RAD(135));
  legs_l->position = az_vadd(head->position,
                             az_vpolar(50, towards_wall - AZ_DEG2RAD(45)));
  legs_l->angle = az_mod2pi(towards_wall + AZ_DEG2RAD(135));
  for (int i = 0; i < 4; ++i) {
    az_arrange_spider_leg(legs_l, legs_r, i, abs_foot_positions[i]);
  }
}

static az_vector_t spider_foot_goal(
    az_space_state_t *state, double up_theta, az_vector_t rough_goal) {
  return raycast(state, az_vsub(rough_goal, az_vpolar(150, up_theta)),
                 up_theta);
}

static void move_spider_legs(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r, int parity, double goal_offset, int horz_sign,
    az_spider_mode_t mode, double time) {
  assert(head->kind == AZ_BAD_MAGBEEST_HEAD);
  assert(legs_l->kind == AZ_BAD_MAGBEEST_LEGS_L);
  assert(legs_r->kind == AZ_BAD_MAGBEEST_LEGS_R);
  assert(parity == 0 || parity == 1);
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  const double up_theta =
    (mode == WALL_SPIDER ? bounds->min_theta - AZ_DEG2RAD(90) :
     mode == CORNER_SPIDER ? bounds->min_theta - AZ_DEG2RAD(45) :
     az_vtheta(head->position));
  const az_vector_t unit_down = az_vpolar(1, up_theta + AZ_PI);
  const az_vector_t unit_left = az_vneg(az_vrot90ccw(unit_down));
  const double horz = copysign(50.0, horz_sign) * (1.0 - head->param);
  const double vert = (60.0 / AZ_PI) * atan(0.15 * fabs(horz));
  for (int leg_index = parity; leg_index < 4; leg_index += 2) {
    const az_vector_t other_foot_pos =
      get_abs_foot_position(legs_l, legs_r, leg_index ^ 1);
    const az_vector_t rough_goal =
      az_vadd(other_foot_pos, az_vmul(unit_left, goal_offset));
    const az_vector_t final_goal =
      spider_foot_goal(state, (mode != CEILING_SPIDER ? up_theta :
                               az_vtheta(rough_goal)),
                       rough_goal);
    const az_vector_t current_goal =
      az_vadd(final_goal, az_vadd(az_vmul(unit_left, horz),
                                  az_vmul(unit_down, vert)));
    move_spider_foot_towards(head, legs_l, legs_r, leg_index, current_goal,
                             time);
  }
}

static void move_wall_spider_legs(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r, int parity, double goal_offset, int horz_sign,
    double time) {
  move_spider_legs(state, head, legs_l, legs_r, parity, goal_offset, horz_sign,
                   WALL_SPIDER, time);
}

static void move_corn_spider_legs(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r, int parity, double goal_offset, int horz_sign,
    double time) {
  move_spider_legs(state, head, legs_l, legs_r, parity, goal_offset, horz_sign,
                   CORNER_SPIDER, time);
}

static void move_ceil_spider_legs(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r, int parity, double goal_offset, int horz_sign,
    double time) {
  move_spider_legs(state, head, legs_l, legs_r, parity, goal_offset, horz_sign,
                   CEILING_SPIDER, time);
}

/*===========================================================================*/

static void prepare_to_spider_popup(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r) {
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  az_vector_t marker_positions[4];
  sort_marker_positions(state, marker_positions,
                        AZ_ARRAY_SIZE(marker_positions));

  head->position = az_vsub(marker_positions[3],
                           az_vwithlen(marker_positions[3], 135));
  head->angle = az_mod2pi(bounds->min_theta - AZ_DEG2RAD(45));
  head->state = HEAD_SPIDER_WAIT_TO_POPUP_STATE;
  arrange_head_piston(head, 0, az_vadd(head->position,
      az_vpolar(70, bounds->min_theta - AZ_DEG2RAD(45))));
  arrange_head_piston(head, 1, az_vadd(head->position,
      az_vpolar(70, bounds->min_theta - AZ_DEG2RAD(135))));

  legs_l->angle = az_mod2pi(bounds->min_theta + AZ_DEG2RAD(45));
  legs_l->position = az_vsub(head->position, az_vpolar(50, legs_l->angle));
  legs_l->state = LEGS_DORMANT_STATE;
  legs_l->param2 = 1;

  legs_r->angle = az_mod2pi(bounds->min_theta + AZ_DEG2RAD(135));
  legs_r->position = az_vsub(head->position, az_vpolar(50, legs_r->angle));
  legs_r->state = LEGS_DORMANT_STATE;
  legs_r->param2 = 1;

  // Arrange legs_l legs, ready to pop up.
  arrange_leg(legs_l, 0, az_vadd(legs_l->position,
      az_vpolar(50, legs_l->angle - AZ_DEG2RAD(135))), legs_l->angle);
  arrange_leg(legs_l, 1, az_vadd(legs_l->position,
      az_vpolar(50, legs_l->angle + AZ_DEG2RAD(135))), legs_l->angle);
  // Arrange legs_r legs, folded up.  Note intentional use of legs_l->angle
  // here for knee direction, to be consistent with spider mode.
  arrange_leg(legs_r, 0, az_vadd(legs_r->position,
      az_vpolar(100, legs_r->angle - AZ_DEG2RAD(180))), legs_l->angle);
  arrange_leg(legs_r, 1, az_vadd(legs_r->position,
      az_vpolar(90, legs_r->angle - AZ_DEG2RAD(110))), legs_l->angle);
}

static void plant_spider_leg(
    az_space_state_t *state, az_baddie_t *head, az_baddie_t *legs_l,
    az_baddie_t *legs_r, int leg_index, double vert_offset, int next_state,
    double time) {
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  head->param = fmin(1.0, head->param + time / SPIDER_PLANT_LEG_TIME);
  const az_vector_t unit_up = az_vpolar(1, bounds->min_theta);
  const double theta_right = bounds->min_theta - AZ_HALF_PI;
  const az_vector_t final_goal =
    raycast(state, az_vadd(head->position,
                           az_vmul(unit_up, vert_offset)), theta_right);
  const az_vector_t current_goal =
    az_vsub(final_goal, az_vpolar(50 * (1 - head->param), theta_right));
  move_spider_foot_towards(head, legs_l, legs_r, leg_index,
                           current_goal, time);
  if (head->param >= 1.0) {
    on_leg_stomp(state);
    head->state = next_state;
    head->param = 0.0;
  }
}

/*===========================================================================*/

void az_tick_bad_magbeest_head(az_space_state_t *state, az_baddie_t *baddie,
                               double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  az_baddie_t *legs_l = lookup_legs(state, baddie, 0);
  az_baddie_t *legs_r = lookup_legs(state, baddie, 1);
  if (legs_l == NULL || legs_l->kind != AZ_BAD_MAGBEEST_LEGS_L ||
      legs_r == NULL || legs_r->kind != AZ_BAD_MAGBEEST_LEGS_R) {
    AZ_WARNING_ONCE("Magbeest head can't find legs.\n");
    return;
  }
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  const double baseline = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  const double head_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case HEAD_INIT_STATE:
      baddie->state = HEAD_DORMANT_STATE;
      baddie->cooldown = 1.0;
      break;
    case HEAD_DORMANT_STATE:
      if (baddie->cooldown <= 0.0) {
        az_vector_t marker_positions[4];
        sort_marker_positions(state, marker_positions,
                              AZ_ARRAY_SIZE(marker_positions));
        baddie->state = HEAD_POPUP_STATE;
        baddie->cooldown = 3.5;
        init_head_popup(baddie, marker_positions[1]);
      }
      break;
    case HEAD_POPDOWN_STATE:
      turn_eye_towards(state, baddie, az_vmul(baddie->position, 2), time);
      if (head_top > baseline) {
        head_pop_up_down(state, baddie, -HEAD_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      if (baddie->cooldown <= 0.0) {
        az_vector_t marker_positions[4];
        if (hurt >= 1/3.) {
          sort_marker_positions(state, marker_positions,
                                AZ_ARRAY_SIZE(marker_positions));
          baddie->state = HEAD_QUICK_POPUP_STATE;
        } else {
          shuffle_marker_positions(state, marker_positions,
                                   AZ_ARRAY_SIZE(marker_positions));
          baddie->state = HEAD_POPUP_STATE;
          baddie->cooldown = 3.5;
          // Position legs:
          if (hurt > 0.0) {
            const bool both = hurt >= 1/6.;
            const bool left = both || az_randint(0, 1) != 0;
            const bool right = both || !left;
            if (left) init_legs_popup(legs_l, marker_positions[1]);
            if (right) init_legs_popup(legs_r, marker_positions[2]);
          }
        }
        init_head_popup(baddie, marker_positions[0]);
      }
      break;
    case HEAD_POPUP_STATE:
      turn_eye_towards(state, baddie, az_vmul(baddie->position, 2), time);
      if (head_top < baseline + HEAD_POPUP_DIST) {
        head_pop_up_down(state, baddie, HEAD_POP_SPEED * time);
      } else {
        baddie->state = HEAD_LASER_SEEK_STATE;
      }
      break;
    case HEAD_LASER_SEEK_STATE:
    case HEAD_WAITING_STATE:
      turn_eye_towards_ship(state, baddie, time);
      if (head_laser_sight(state, baddie) &&
          baddie->state == HEAD_LASER_SEEK_STATE) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_ROCKET, 15.0,
                                  baddie->components[10].angle, 0.0);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
        baddie->state = HEAD_WAITING_STATE;
      } else if (baddie->cooldown <= 0.0) {
        baddie->state = HEAD_POPDOWN_STATE;
        baddie->cooldown = 5.0;
      }
      break;
    case HEAD_QUICK_POPUP_STATE:
      turn_eye_towards_ship(state, baddie, time);
      if (head_top < baseline + HEAD_POPUP_DIST) {
        head_pop_up_down(state, baddie, 1.5 * HEAD_POP_SPEED * time);
      } else {
        baddie->state = HEAD_QUICK_POPDOWN_STATE;
        baddie->cooldown = 1.5;
      }
      break;
    case HEAD_QUICK_POPDOWN_STATE:
      turn_eye_towards_ship(state, baddie, time);
      if (head_top > baseline) {
        head_pop_up_down(state, baddie, -1.5 * HEAD_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      if (baddie->cooldown <= 0.0) {
        az_vector_t marker_positions[4];
        sort_marker_positions(state, marker_positions,
                              AZ_ARRAY_SIZE(marker_positions));
        int new_index = 3;
        for (int i = 0; i < 3; ++i) {
          if (az_vwithin(baddie->position, marker_positions[i], 50)) {
            new_index = i + 1;
            break;
          }
        }
        if (new_index < 3) {
          init_head_popup(baddie, marker_positions[new_index]);
          baddie->state = HEAD_QUICK_POPUP_STATE;
        } else {
          prepare_to_spider_popup(state, baddie, legs_l, legs_r);
        }
      }
      break;
    case HEAD_SPIDER_WAIT_TO_POPUP_STATE: {
      const az_vector_t target =
        az_vwithlen(baddie->position, bounds->min_r + 0.5 * bounds->r_span);
      if (az_vwithin(state->ship.position, target, 320)) {
        baddie->state = HEAD_SPIDER_POPUP_STATE;
        legs_l->state = MAGNET_RECOVER_STATE;
        legs_r->state = GATLING_STOP_FIRING_STATE;
      }
    } break;
    case HEAD_SPIDER_POPUP_STATE: {
      const az_vector_t old_legs_l_pos = legs_l->position;
      const double base_top =
        az_vnorm(legs_l->position) + legs_l->data->main_body.bounding_radius;
      if (base_top < baseline + LEGS_POPUP_DIST - 40) {
        legs_pop_up_down(state, legs_l, baseline, LEGS_POP_SPEED * time);
        const az_vector_t legs_l_delta =
          az_vsub(legs_l->position, old_legs_l_pos);
        az_vpluseq(&baddie->position, legs_l_delta);
        az_vpluseq(&legs_r->position, legs_l_delta);
      } else {
        baddie->state = HEAD_SPIDER_PLANT_LEG_0_STATE;
        baddie->param = 0.0;
      }
    } break;
    case HEAD_SPIDER_PLANT_LEG_0_STATE:
      plant_spider_leg(state, baddie, legs_l, legs_r, 0, 200,
                       HEAD_SPIDER_PLANT_LEG_1_STATE, time);
      break;
    case HEAD_SPIDER_PLANT_LEG_1_STATE:
      plant_spider_leg(state, baddie, legs_l, legs_r, 1, 100,
                       HEAD_SPIDER_PLANT_LEG_2_STATE, time);
      break;
    case HEAD_SPIDER_PLANT_LEG_2_STATE:
      plant_spider_leg(state, baddie, legs_l, legs_r, 2, -100,
                       HEAD_SPIDER_PLANT_LEG_3_STATE, time);
      break;
    case HEAD_SPIDER_PLANT_LEG_3_STATE:
      plant_spider_leg(state, baddie, legs_l, legs_r, 3, -200,
                       HEAD_WALL_SPIDER_MOVE_BODY_STATE, time);
      break;
    case HEAD_WALL_SPIDER_MOVE_BODY_STATE: {
      const az_vector_t foot_0_pos = get_abs_foot_position(legs_l, legs_r, 0);
      const az_vector_t foot_2_pos = get_abs_foot_position(legs_l, legs_r, 2);
      const az_vector_t foot_3_pos = get_abs_foot_position(legs_l, legs_r, 3);
      const az_vector_t goal =
        az_vadd(az_vmul(az_vadd(foot_0_pos, foot_3_pos), 0.5),
                az_vwithlen(az_vrot90ccw(az_vsub(foot_0_pos, foot_3_pos)),
                            140));
      shift_spider_body(baddie, legs_l, legs_r,
                        az_vcaplen(az_vsub(goal, baddie->position),
                                   SPIDER_BODY_SPEED * time));
      if (az_vwithin(baddie->position, goal, 1)) {
        baddie->param = 0.0;
        if (az_vnorm(get_abs_foot_position(legs_l, legs_r, 1)) >
            bounds->min_r + bounds->r_span + 25) {
          baddie->state = HEAD_CORNER_SPIDER_MOVE_EVEN_LEGS_UP_STATE;
        } else if (az_vwithin(foot_2_pos, foot_3_pos, 120)) {
          baddie->state = HEAD_WALL_SPIDER_MOVE_EVEN_LEGS_UP_STATE;
        } else {
          baddie->state = HEAD_WALL_SPIDER_MOVE_ODD_LEGS_UP_STATE;
        }
      }
    } break;
    case HEAD_WALL_SPIDER_MOVE_EVEN_LEGS_UP_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_wall_spider_legs(state, baddie, legs_l, legs_r, 0, 150, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_WALL_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_WALL_SPIDER_MOVE_ODD_LEGS_UP_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_wall_spider_legs(state, baddie, legs_l, legs_r, 1, -100, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_WALL_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CORNER_SPIDER_MOVE_BODY_STATE: {
      const az_vector_t foot_0_pos = get_abs_foot_position(legs_l, legs_r, 0);
      const az_vector_t foot_2_pos = get_abs_foot_position(legs_l, legs_r, 2);
      const az_vector_t foot_3_pos = get_abs_foot_position(legs_l, legs_r, 3);
      const az_vector_t goal =
        az_vadd(az_vmul(az_vadd(foot_0_pos, foot_3_pos), 0.5),
                az_vwithlen(az_vrot90ccw(az_vsub(foot_0_pos, foot_3_pos)),
                            100));
      shift_spider_body(baddie, legs_l, legs_r,
                        az_vcaplen(az_vsub(goal, baddie->position),
                                   SPIDER_BODY_SPEED * time));
      if (az_vwithin(baddie->position, goal, 1)) {
        baddie->param = 0.0;
        if (az_vnorm(foot_3_pos) > bounds->min_r + bounds->r_span + 120) {
          baddie->state = HEAD_CEILING_SPIDER_MOVE_BODY_STATE;
        } else if (az_vwithin(foot_2_pos, foot_3_pos, 120)) {
          baddie->state = HEAD_CORNER_SPIDER_MOVE_EVEN_LEGS_UP_STATE;
        } else {
          baddie->state = HEAD_CORNER_SPIDER_MOVE_ODD_LEGS_UP_STATE;
        }
      }
    } break;
    case HEAD_CORNER_SPIDER_MOVE_EVEN_LEGS_UP_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_corn_spider_legs(state, baddie, legs_l, legs_r, 0, 157, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CORNER_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CORNER_SPIDER_MOVE_ODD_LEGS_UP_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_corn_spider_legs(state, baddie, legs_l, legs_r, 1, -71, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CORNER_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CEILING_SPIDER_MOVE_BODY_STATE: {
      const az_vector_t foot_0_pos = get_abs_foot_position(legs_l, legs_r, 0);
      const az_vector_t foot_2_pos = get_abs_foot_position(legs_l, legs_r, 2);
      const az_vector_t foot_3_pos = get_abs_foot_position(legs_l, legs_r, 3);
      const az_vector_t goal =
        az_vadd(az_vmul(az_vadd(foot_0_pos, foot_3_pos), 0.5),
                az_vwithlen(az_vrot90ccw(az_vsub(foot_0_pos, foot_3_pos)),
                            140));
      shift_spider_body(baddie, legs_l, legs_r,
                        az_vcaplen(az_vsub(goal, baddie->position),
                                   SPIDER_BODY_SPEED * time));
      if (az_vwithin(baddie->position, goal, 1)) {
        baddie->param = 0.0;
        if (az_mod2pi(az_vtheta(state->ship.position) -
                      az_vtheta(baddie->position)) < 0) {
          if (az_vtheta(baddie->position) >= bounds->min_theta) {
            if (az_vwithin(foot_2_pos, foot_3_pos, 120)) {
              baddie->state = HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_RIGHT_STATE;
            } else {
              baddie->state = HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_RIGHT_STATE;
            }
          }
        } else {
          if (az_vtheta(baddie->position) <=
              bounds->min_theta + bounds->theta_span) {
            if (az_vwithin(foot_2_pos, foot_3_pos, 120)) {
              baddie->state = HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_LEFT_STATE;
            } else {
              baddie->state = HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_LEFT_STATE;
            }
          }
        }
      }
    } break;
    case HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_LEFT_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_ceil_spider_legs(state, baddie, legs_l, legs_r, 0, 150, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CEILING_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_LEFT_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_ceil_spider_legs(state, baddie, legs_l, legs_r, 1, -100, -1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CEILING_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CEILING_SPIDER_MOVE_EVEN_LEGS_RIGHT_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_ceil_spider_legs(state, baddie, legs_l, legs_r, 0, 100, 1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CEILING_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    case HEAD_CEILING_SPIDER_MOVE_ODD_LEGS_RIGHT_STATE: {
      baddie->param = fmin(1.0, baddie->param + time / SPIDER_MOVE_LEGS_TIME);
      move_ceil_spider_legs(state, baddie, legs_l, legs_r, 1, -150, 1, time);
      if (baddie->param >= 1.0) {
        on_leg_stomp(state);
        baddie->state = HEAD_CEILING_SPIDER_MOVE_BODY_STATE;
      }
    } break;
    default:
      baddie->state = HEAD_POPDOWN_STATE;
      break;
  }
  if (baddie->state >= HEAD_CEILING_SPIDER_MOVE_BODY_STATE) {
    turn_eye_towards_ship(state, baddie, time);
    // Magma bombs:
    if (baddie->cooldown <= 0.0) {
      baddie->cooldown = 5.0;
      az_vector_t marker_positions[4];
      get_marker_positions(state, marker_positions,
                           AZ_ARRAY_SIZE(marker_positions));
      az_vector_t target = AZ_VZERO;
      double best_dist = INFINITY;
      AZ_ARRAY_LOOP(position, marker_positions) {
        const double dist = az_vdist(*position, state->ship.position);
        if (dist < best_dist) {
          target = *position;
          best_dist = dist;
        }
      }
      az_baddie_t *bomb = az_add_baddie(state, AZ_BAD_MAGMA_BOMB,
                                        baddie->position, baddie->angle);
      if (bomb != NULL) {
        const az_vector_t delta = az_vsub(target, baddie->position);
        bomb->velocity = az_vadd(az_vwithlen(delta, 400.0),
                                 az_vwithlen(baddie->position,
                                             0.4 * az_vnorm(delta)));
        az_play_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGMA_BOMB);
      }
    }
  }
  // TODO: occasionally cause scrap metal to fall from ceiling?
}

void az_tick_bad_magbeest_legs_l(az_space_state_t *state, az_baddie_t *baddie,
                                 double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L);
  az_component_t *magnet = &baddie->components[0];
  const az_vector_t magnet_abs_hinge_pos =
    az_vadd(az_vrotate(magnet->position, baddie->angle), baddie->position);
  const az_vector_t magnet_abs_emitter_pos =
    az_vadd(az_vrotate(az_vadd(az_vpolar(25.0, magnet->angle),
                               magnet->position), baddie->angle),
            baddie->position);
  const az_vector_t magnet_abs_collect_pos =
    az_vadd(az_vrotate(az_vadd(az_vpolar(35.0, magnet->angle),
                               magnet->position), baddie->angle),
            baddie->position);
  // Fusion beam:
  if (baddie->state == MAGNET_FUSION_BEAM_CHARGE_STATE) {
    const double threshold = 0.75 * MAGNET_FUSION_BEAM_CHARGE_TIME;
    if (baddie->cooldown > threshold) {
      az_loop_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGNET);
    } else if (baddie->cooldown + time > threshold) {
      az_play_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGNET_CHARGE);
    }
    const double factor = baddie->cooldown / MAGNET_FUSION_BEAM_CHARGE_TIME;
    // Aim the magnet at the ship:
    if (az_ship_is_decloaked(&state->ship)) {
      const double turn_rate = AZ_DEG2RAD(40) + AZ_DEG2RAD(60) * factor;
      magnet->angle = az_mod2pi(az_angle_towards(
          magnet->angle + baddie->angle, turn_rate * time,
          az_vtheta(az_vsub(state->ship.position, magnet_abs_hinge_pos))) -
                                baddie->angle);
    }
    const double magnet_abs_angle = az_mod2pi(magnet->angle + baddie->angle);
    // Apply force to ship:
    if (az_ship_is_alive(&state->ship)) {
      const az_vector_t delta =
        az_vsub(state->ship.position, magnet_abs_hinge_pos);
      const double arc = AZ_DEG2RAD(10) * factor;
      if (az_mod2pi(az_vtheta(delta) - magnet_abs_angle) <= arc) {
        az_vpluseq(&state->ship.velocity, az_vwithlen(delta, -300.0 * time));
      }
    }
    // Fire:
    if (baddie->cooldown <= 0.0) {
      az_add_projectile(state, AZ_PROJ_MAGNET_FUSION_BEAM,
                        magnet_abs_emitter_pos, magnet_abs_angle, 1.0,
                        baddie->uid);
      baddie->state = MAGNET_RECOVER_STATE;
      baddie->cooldown = 1.0;
    }
    return;
  }
  // Use magnet:
  if (baddie->state >= MAGNET_FINISH_ATTRACT_STATE &&
      baddie->state <= MAGNET_START_SPAWN_STATE) {
    // Aim the magnet at the ship:
    if (az_ship_is_decloaked(&state->ship)) {
      const double old_angle = magnet->angle;
      magnet->angle = az_mod2pi(az_angle_towards(
          magnet->angle + baddie->angle, AZ_DEG2RAD(100) * time,
          az_vtheta(az_vsub(state->ship.position, magnet_abs_hinge_pos))) -
                                baddie->angle);
      // Pull scrap metal along with swinging magnetetic field:
      const double dtheta = az_mod2pi(magnet->angle - old_angle);
      AZ_ARRAY_LOOP(scrap, state->baddies) {
        if (scrap->kind != AZ_BAD_SCRAP_METAL) continue;
        scrap->position =
          az_vadd(az_vrotate(az_vsub(scrap->position, magnet_abs_hinge_pos),
                             0.7 * dtheta), magnet_abs_hinge_pos);
      }
    }
    const double magnet_abs_angle = az_mod2pi(magnet->angle + baddie->angle);
    // Spawn scrap metal:
    if (baddie->state != MAGNET_FINISH_ATTRACT_STATE &&
        baddie->cooldown <= 0.0) {
      az_add_baddie(state, AZ_BAD_SCRAP_METAL,
                    az_vadd(magnet_abs_hinge_pos,
                            az_vpolar(1500.0, magnet_abs_angle +
                                      az_random(-AZ_DEG2RAD(10),
                                                AZ_DEG2RAD(10)))),
                    az_random(-AZ_PI, AZ_PI));
      --baddie->state;
      baddie->cooldown = az_random(0.06, 0.12);
    }
    // Apply force to scrap metal:
    double max_scrap_dist = 0.0;
    AZ_ARRAY_LOOP(scrap, state->baddies) {
      if (scrap->kind != AZ_BAD_SCRAP_METAL) continue;
      az_vpluseq(&scrap->position, az_vwithlen(
          az_vsub(scrap->position, magnet_abs_collect_pos), -500.0 * time));
      scrap->angle = az_mod2pi(scrap->angle + AZ_DEG2RAD(300) * time);
      max_scrap_dist =
        fmax(max_scrap_dist, az_vdist(scrap->position,
                                      magnet_abs_collect_pos));
    }
    az_loop_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGNET);
    // Apply force to ship:
    if (az_ship_is_alive(&state->ship)) {
      const az_vector_t delta =
        az_vsub(state->ship.position, magnet_abs_hinge_pos);
      if (az_mod2pi(az_vtheta(delta) - magnet_abs_angle) <= AZ_DEG2RAD(10)) {
        az_vpluseq(&state->ship.velocity, az_vwithlen(delta, -300.0 * time));
      }
    }
    // Fire:
    if (baddie->state == MAGNET_FINISH_ATTRACT_STATE &&
        max_scrap_dist < 10.0) {
      int num_scraps = 0;
      AZ_ARRAY_LOOP(scrap, state->baddies) {
        if (scrap->kind != AZ_BAD_SCRAP_METAL) continue;
        az_projectile_t *proj =
          az_add_projectile(state, AZ_PROJ_SCRAP_METAL, scrap->position,
                            magnet_abs_angle +
                            az_random(-AZ_DEG2RAD(15), AZ_DEG2RAD(15)),
                            1.0, baddie->uid);
        if (proj != NULL) {
          proj->angle = az_random(-AZ_PI, AZ_PI);
          proj->velocity = az_vmul(proj->velocity, az_random(0.75, 1.25));
          ++num_scraps;
        }
        scrap->kind = AZ_BAD_NOTHING;
      }
      if (num_scraps > 0) {
        az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
      }
      baddie->state = MAGNET_RECOVER_STATE;
      baddie->cooldown = 0.5;
    }
    return;
  }
  // Pop up/down:
  const double baseline =
    az_current_camera_bounds(state)->min_r - AZ_SCREEN_HEIGHT/2;
  const double base_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case LEGS_DORMANT_STATE:
      break;
    case LEGS_POPDOWN_STATE:
      if (base_top > baseline) {
        legs_pop_up_down(state, baddie, baseline, -LEGS_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      break;
    case LEGS_POPUP_STATE:
      if (base_top < baseline + LEGS_POPUP_DIST) {
        legs_pop_up_down(state, baddie, baseline, LEGS_POP_SPEED * time);
      } else {
        baddie->state = MAGNET_COOLDOWN_STATE;
      }
      break;
    case MAGNET_COOLDOWN_STATE:
      if (az_ship_is_decloaked(&state->ship)) {
        magnet->angle = az_mod2pi(az_angle_towards(
            magnet->angle + baddie->angle, AZ_DEG2RAD(100) * time,
            az_vtheta(az_vsub(state->ship.position, magnet_abs_hinge_pos))) -
                                  baddie->angle);
      }
      if (baddie->cooldown <= 0.0) {
        if (az_random(0.0, 1.0) < 1.0 - pow(0.75, baddie->param)) {
          baddie->state = MAGNET_FUSION_BEAM_CHARGE_STATE;
          baddie->cooldown = MAGNET_FUSION_BEAM_CHARGE_TIME;
          baddie->param = 0.0;
        } else {
          baddie->state = (baddie->param2 > 0 ? MAGNET_MEDIUM_SPAWN_STATE :
                           MAGNET_START_SPAWN_STATE);
          baddie->param += 1.0;
        }
      }
      break;
    case MAGNET_RECOVER_STATE:
      if (baddie->cooldown <= 0.0) {
        if (baddie->param2 > 0) {
          baddie->state = MAGNET_COOLDOWN_STATE;
          baddie->cooldown = 6.9;
        } else {
          baddie->state = LEGS_POPDOWN_STATE;
        }
      }
      break;
    default:
      baddie->state = LEGS_POPDOWN_STATE;
      break;
  }
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
  // Fire gatling gun:
  if (baddie->state > GATLING_STOP_FIRING_STATE &&
      baddie->state <= GATLING_START_FIRING_STATE) {
    if (baddie->cooldown <= 0.0) {
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
    baddie->param = az_mod2pi(baddie->param + 2 * AZ_TWO_PI * time);
    return;
  }
  const double baseline =
    az_current_camera_bounds(state)->min_r - AZ_SCREEN_HEIGHT/2;
  const double base_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case LEGS_DORMANT_STATE:
      break;
    case LEGS_POPDOWN_STATE:
      if (base_top > baseline) {
        legs_pop_up_down(state, baddie, baseline, -LEGS_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      break;
    case LEGS_POPUP_STATE:
      if (base_top < baseline + LEGS_POPUP_DIST) {
        legs_pop_up_down(state, baddie, baseline, LEGS_POP_SPEED * time);
      }
      if (baddie->cooldown <= 0.0) {
        baddie->state = GATLING_START_FIRING_STATE;
      }
      break;
    case GATLING_COOLDOWN_STATE:
      if (baddie->cooldown <= 0.0) {
        baddie->state = GATLING_MID_FIRING_STATE;
      }
      break;
    case GATLING_STOP_FIRING_STATE:
      if (baddie->param2 > 0) {
        baddie->state = GATLING_COOLDOWN_STATE;
        baddie->cooldown = 4.2;
      } else {
        baddie->state = LEGS_POPDOWN_STATE;
      }
      break;
    default:
      baddie->state = LEGS_POPDOWN_STATE;
      break;
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
  } else if (baddie->cooldown <= 1.5 &&
             ceil(2 * baddie->cooldown) <
             ceil(2 * (baddie->cooldown + time))) {
    az_play_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGMA_BOMB);
  }
}

/*===========================================================================*/
