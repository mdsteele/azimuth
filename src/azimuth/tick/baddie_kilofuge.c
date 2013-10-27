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

#include "azimuth/tick/baddie_kilofuge.h"

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

#define MOVE_BODY_STATE 0
#define MOVE_EVEN_LEGS_STATE 1
#define MOVE_ODD_LEGS_STATE 2

#define NUM_EYES 3
#define FIRST_EYE_COMPONENT_INDEX 0
#define NUM_LEGS 6
#define FIRST_LEG_COMPONENT_INDEX 5
#define LEG_LENGTH 80.0

#define EYE_TURN_RATE (AZ_DEG2RAD(60))
#define BODY_SPEED 45.0
#define FOOT_SPEED 60.0

/*===========================================================================*/

static az_vector_t leg_anchors[NUM_LEGS];

static void init_leg_anchors(const az_baddie_data_t *data) {
  static bool leg_anchors_initialized = false;
  if (leg_anchors_initialized) return;
  for (int i = 0; i < NUM_LEGS; ++i) {
    const az_component_data_t *leg =
      &data->components[i + FIRST_LEG_COMPONENT_INDEX];
    leg_anchors[i] =
      az_vadd(leg->init_position, az_vpolar(-LEG_LENGTH, leg->init_angle));
  }
  leg_anchors_initialized = true;
}

/*===========================================================================*/

static void turn_eyes_towards(
    az_baddie_t *baddie, double time, az_vector_t target) {
  for (int eye_index = 0; eye_index < NUM_EYES; ++eye_index) {
    const int component_index = FIRST_EYE_COMPONENT_INDEX + eye_index;
    az_component_t *eye = &baddie->components[component_index];
    const az_vector_t abs_eye_position =
      az_vadd(az_vrotate(eye->position, baddie->angle), baddie->position);
    const double goal_angle = az_vtheta(az_vsub(target, abs_eye_position));
    const double old_abs_eye_angle = az_mod2pi(eye->angle + baddie->angle);
    const double new_abs_eye_angle =
      az_angle_towards(old_abs_eye_angle, EYE_TURN_RATE * time, goal_angle);
    eye->angle = az_mod2pi(new_abs_eye_angle - baddie->angle);
  }
}

static void move_body_forward(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const az_camera_bounds_t *bounds =
    &state->planet->rooms[state->ship.player.current_room].camera_bounds;
  const double mid_r = bounds->min_r + 0.5 * bounds->r_span;
  const az_vector_t old_position = baddie->position;
  const double old_angle = baddie->angle;
  const az_vector_t end_position = az_vpolar(mid_r, bounds->min_theta);
  if (az_vwithin(old_position, end_position, 20.0)) {
    // TODO: change state
    return;
  }
  const az_vector_t new_position = az_vwithlen(
      az_vadd(old_position, az_vpolar(BODY_SPEED * time, old_angle)), mid_r);
  const double new_angle = az_mod2pi(az_vtheta(old_position) - AZ_HALF_PI);
  int stop_index = -1;
  for (int leg_index = 0; leg_index < NUM_LEGS; ++leg_index) {
    const int component_index = FIRST_LEG_COMPONENT_INDEX + leg_index;
    az_component_t *leg = &baddie->components[component_index];
    const az_vector_t abs_foot_position =
      az_vadd(az_vrotate(leg->position, old_angle), old_position);
    leg->position = az_vrotate(az_vsub(abs_foot_position, new_position),
                               -new_angle);
    leg->angle = az_vtheta(az_vsub(leg->position, leg_anchors[leg_index]));
    if (fabs(leg->angle) >= 2.0) stop_index = leg_index;
  }
  baddie->position = new_position;
  baddie->angle = new_angle;
  if (stop_index >= 0) {
    baddie->state = (stop_index % 2 == 0 ? MOVE_EVEN_LEGS_STATE :
                     MOVE_ODD_LEGS_STATE);
  }
}

static void move_legs_forward(az_baddie_t *baddie, double time, int parity) {
  int num_legs_okay = 0;
  for (int leg_index = parity; leg_index < NUM_LEGS; leg_index += 2) {
    const int component_index = FIRST_LEG_COMPONENT_INDEX + leg_index;
    az_component_t *leg = &baddie->components[component_index];
    const az_vector_t anchor = leg_anchors[leg_index];
    const az_vector_t goal =
      az_vadd(anchor, (az_vector_t){40, copysign(80, anchor.y)});
    if (az_vwithin(leg->position, goal, FOOT_SPEED * time)) {
      leg->position = goal;
      ++num_legs_okay;
    } else {
      az_vpluseq(&leg->position, az_vcaplen(az_vsub(goal, leg->position),
                                            FOOT_SPEED * time));
    }
    leg->angle = az_vtheta(az_vsub(leg->position, anchor));
  }
  if (num_legs_okay >= NUM_LEGS / 2) baddie->state = MOVE_BODY_STATE;
}

/*===========================================================================*/

void az_tick_bad_kilofuge(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  init_leg_anchors(baddie->data);
  switch (baddie->state) {
    case MOVE_BODY_STATE:
      move_body_forward(state, baddie, time);
      break;
    case MOVE_EVEN_LEGS_STATE:
      move_legs_forward(baddie, time, 0);
      break;
    case MOVE_ODD_LEGS_STATE:
      move_legs_forward(baddie, time, 1);
      break;
    default:
      baddie->state = MOVE_BODY_STATE;
      break;
  }
  if (az_ship_is_present(&state->ship)) {
    turn_eyes_towards(baddie, time, state->ship.position);
  }
}

/*===========================================================================*/
