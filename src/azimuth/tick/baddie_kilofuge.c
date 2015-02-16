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
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/tick/object.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define MOVE_BODY_FORWARD_STATE 0
#define MOVE_EVEN_LEGS_FORWARD_STATE 1
#define MOVE_ODD_LEGS_FORWARD_STATE 2
#define MOVE_BODY_BACKWARD_STATE 3
#define MOVE_EVEN_LEGS_BACKWARD_STATE 4
#define MOVE_ODD_LEGS_BACKWARD_STATE 5
#define MELTBEAM_STATE 6
#define CROSSBEAM_STATE 7
#define WAITING_STATE 8
#define HIDING_STATE 9

#define NUM_EYES 3
#define FIRST_EYE_COMPONENT_INDEX 0
#define LEFT_PINCER_COMPONENT_INDEX 3
#define RIGHT_PINCER_COMPONENT_INDEX 4
#define NUM_LEGS 6
#define FIRST_LEG_COMPONENT_INDEX 5
#define LEG_LENGTH 80.0

#define BASE_EYE_TURN_RATE (AZ_DEG2RAD(15))
#define BASE_BODY_SPEED 60.0
#define BASE_FOOT_SPEED 80.0

#define MELTBEAM_MIN_RANGE 350.0
#define MELTBEAM_MID_RANGE 400.0
#define MELTBEAM_MAX_RANGE 600.0

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

static double speed_mult(const az_baddie_t *baddie) {
  return 2.0 - baddie->health / baddie->data->max_health;
}

static void move_pincers_towards(az_baddie_t *baddie, double time,
                                 double goal_angle) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  az_component_t *left_pincer =
    &baddie->components[LEFT_PINCER_COMPONENT_INDEX];
  az_component_t *right_pincer =
    &baddie->components[RIGHT_PINCER_COMPONENT_INDEX];
  const double new_left_angle =
    az_angle_towards(left_pincer->angle, AZ_DEG2RAD(120) * time, goal_angle);
  left_pincer->angle = new_left_angle;
  right_pincer->angle = -new_left_angle;
}

static void close_pincers(az_baddie_t *baddie, double time) {
  move_pincers_towards(baddie, time, AZ_DEG2RAD(-45));
}

static void open_pincers(az_baddie_t *baddie, double time) {
  move_pincers_towards(baddie, time, AZ_DEG2RAD(10));
}

static void relax_pincers(az_baddie_t *baddie, double time) {
  move_pincers_towards(baddie, time, 0);
}

// Rotate all three eyes towards pointing at the given absolute position.
static void turn_eyes_towards(
    az_baddie_t *baddie, double time, az_vector_t target) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  for (int eye_index = 0; eye_index < NUM_EYES; ++eye_index) {
    const int component_index = FIRST_EYE_COMPONENT_INDEX + eye_index;
    az_component_t *eye = &baddie->components[component_index];
    const az_vector_t abs_eye_position =
      az_vadd(az_vrotate(eye->position, baddie->angle), baddie->position);
    const double goal_angle = az_vtheta(az_vsub(target, abs_eye_position));
    const double old_abs_eye_angle = az_mod2pi(eye->angle + baddie->angle);
    const double new_abs_eye_angle =
      az_angle_towards(old_abs_eye_angle, speed_mult(baddie) *
                       BASE_EYE_TURN_RATE * time, goal_angle);
    eye->angle = az_mod2pi(new_abs_eye_angle - baddie->angle);
  }
}

static void turn_eyes_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const az_vector_t target =
    (az_ship_is_decloaked(&state->ship) ? state->ship.position :
     az_vadd(baddie->position, az_vpolar(5000.0, baddie->angle)));
  turn_eyes_towards(baddie, time, target);
}

// Destroy any ice crystals that are touching the Kilofuge.
static void crush_ice_crystals(az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  AZ_ARRAY_LOOP(other, state->baddies) {
    if (other->kind != AZ_BAD_ICE_CRYSTAL) continue;
    if (az_circle_touches_baddie(baddie, other->data->overall_bounding_radius,
                                 other->position, NULL)) {
      az_kill_baddie(state, other);
    }
  }
}

static void release_gnat_swarm(az_space_state_t *state, az_baddie_t *baddie) {
  const az_component_t *main_eye =
    &baddie->components[FIRST_EYE_COMPONENT_INDEX];
  const az_vector_t start_position =
    az_vadd(az_vrotate(main_eye->position, baddie->angle), baddie->position);
  for (int i = -50; i <= 50; i += 20) {
    const double angle = baddie->angle + AZ_DEG2RAD(i);
    az_baddie_t *gnat =
      az_add_baddie(state, AZ_BAD_GNAT,
                    az_vadd(start_position, az_vpolar(10.0, angle)), angle);
    if (gnat == NULL) break;
  }
}

static void choose_next_state(
    az_space_state_t *state, az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);

  if (baddie->param > 0.0 || !az_ship_is_alive(&state->ship)) {
    baddie->state = MOVE_BODY_BACKWARD_STATE;
    baddie->param = fmax(0.0, baddie->param - 1.0);
    return;
  }

  if (baddie->state != HIDING_STATE) {
    bool any_crystals_in_range = false;
    AZ_ARRAY_LOOP(other, state->baddies) {
      if (other->kind != AZ_BAD_ICE_CRYSTAL) continue;
      if (!az_vwithin(baddie->position, other->position, MELTBEAM_MIN_RANGE) &&
          az_vwithin(baddie->position, other->position, MELTBEAM_MID_RANGE)) {
        any_crystals_in_range = true;
        break;
      }
    }
    if (any_crystals_in_range) {
      baddie->state = MELTBEAM_STATE;
      baddie->cooldown = 3.0 / speed_mult(baddie);
      return;
    }
  }

  if (az_randint(1, (baddie->state == HIDING_STATE ? 3 : 6)) == 1) {
    release_gnat_swarm(state, baddie);
    baddie->state = WAITING_STATE;
    baddie->cooldown = 2.0 / speed_mult(baddie);
  } else if (baddie->state != HIDING_STATE && az_randint(1, 7) == 1) {
    baddie->state = CROSSBEAM_STATE;
    baddie->cooldown = 2.5 / speed_mult(baddie);
    baddie->components[FIRST_EYE_COMPONENT_INDEX + 1].angle = AZ_DEG2RAD(-45);
    baddie->components[FIRST_EYE_COMPONENT_INDEX + 2].angle = AZ_DEG2RAD(45);
  } else {
    baddie->state = MOVE_BODY_FORWARD_STATE;
    az_vector_t rel_impact;
    if (az_ship_is_decloaked(&state->ship) &&
        az_lead_target(az_vsub(state->ship.position,
                               az_vadd(baddie->position,
                                       az_vpolar(150.0, baddie->angle))),
                       state->ship.velocity, 400.0, &rel_impact)) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_ICE_TORPEDO, 150.0, 0.0,
                                az_vtheta(rel_impact) - baddie->angle);
    }
  }
}

static void move_body(
    az_space_state_t *state, az_baddie_t *baddie, double time, int direction) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  assert(direction == 1 || direction == -1);
  if (direction < 0) close_pincers(baddie, time);
  else relax_pincers(baddie, time);
  turn_eyes_towards_ship(state, baddie, time);
  // Determine our new position, moving along the center of the room.
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  const double mid_r = bounds->min_r + 0.5 * bounds->r_span;
  const az_vector_t old_position = baddie->position;
  const double old_angle = baddie->angle;
  const az_vector_t end_position =
    az_vpolar(mid_r, bounds->min_theta +
              (direction < 0 ? bounds->theta_span : 0.0));
  if (az_vwithin(old_position, end_position, 20.0)) {
    baddie->state = (direction < 0 ? HIDING_STATE : WAITING_STATE);
    baddie->cooldown = 1.0;
    return;
  }
  const az_vector_t new_position =
    az_vwithlen(az_vadd(old_position,
                        az_vpolar(direction * speed_mult(baddie) *
                                  BASE_BODY_SPEED * time, old_angle)), mid_r);
  const double new_angle = az_mod2pi(az_vtheta(old_position) - AZ_HALF_PI);
  // Move legs so that all the feet stay in the same absolute position.  Stop
  // moving when any of the legs are bent too far.
  int stop_index = -1;
  for (int leg_index = 0; leg_index < NUM_LEGS; ++leg_index) {
    const int component_index = FIRST_LEG_COMPONENT_INDEX + leg_index;
    az_component_t *leg = &baddie->components[component_index];
    const az_vector_t abs_foot_position =
      az_vadd(az_vrotate(leg->position, old_angle), old_position);
    leg->position = az_vrotate(az_vsub(abs_foot_position, new_position),
                               -new_angle);
    leg->angle = az_vtheta(az_vsub(leg->position, leg_anchors[leg_index]));
    if (direction < 0 ? fabs(leg->angle) <= 1.0 : fabs(leg->angle) >= 2.0) {
      stop_index = leg_index;
    }
  }
  // Update the baddie position and destroy any ice crystals we run into.
  baddie->position = new_position;
  baddie->angle = new_angle;
  crush_ice_crystals(state, baddie);
  // Whichever legs are bent back, we'll move those legs next.
  if (stop_index >= 0) {
    baddie->state = (direction < 0 ?
                     (stop_index % 2 == 0 ? MOVE_EVEN_LEGS_BACKWARD_STATE :
                      MOVE_ODD_LEGS_BACKWARD_STATE) :
                     (stop_index % 2 == 0 ? MOVE_EVEN_LEGS_FORWARD_STATE :
                      MOVE_ODD_LEGS_FORWARD_STATE));
  }
}

static void move_legs(az_space_state_t *state, az_baddie_t *baddie,
                      double time, int parity, int direction) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  assert(parity == 0 || parity == 1);
  assert(direction == 1 || direction == -1);
  if (direction < 0) close_pincers(baddie, time);
  else relax_pincers(baddie, time);
  turn_eyes_towards_ship(state, baddie, time);
  // Move half of the legs forward; either the even or the odd numbered ones.
  int num_legs_okay = 0;
  for (int leg_index = parity; leg_index < NUM_LEGS; leg_index += 2) {
    const int component_index = FIRST_LEG_COMPONENT_INDEX + leg_index;
    az_component_t *leg = &baddie->components[component_index];
    const az_vector_t anchor = leg_anchors[leg_index];
    const double delta = speed_mult(baddie) * BASE_FOOT_SPEED * time;
    const az_vector_t goal =
      az_vadd(anchor, (az_vector_t){40 * direction, copysign(80, anchor.y)});
    if (az_vwithin(leg->position, goal, delta)) {
      leg->position = goal;
      ++num_legs_okay;
    } else {
      az_vpluseq(&leg->position,
                 az_vcaplen(az_vsub(goal, leg->position), delta));
    }
    // Angle the leg so that it still passes through the anchor.
    leg->angle = az_vtheta(az_vsub(leg->position, anchor));
  }
  crush_ice_crystals(state, baddie);
  if (num_legs_okay >= NUM_LEGS / 2) {
    if (direction < 0) baddie->state = HIDING_STATE;
    choose_next_state(state, baddie);
  }
}

static void fire_beam_from_eye(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    int eye_index, double power) {
  // Fire a beam from the eye.
  const double eye_radius =
    baddie->data->components[eye_index].bounding_radius;
  const az_component_t *eye =
    &baddie->components[FIRST_EYE_COMPONENT_INDEX + eye_index];
  const double beam_angle = baddie->angle + eye->angle;
  const az_vector_t beam_start =
    az_vadd(az_vadd(az_vrotate(eye->position, baddie->angle),
                    baddie->position),
            az_vpolar(0.7 * eye_radius, beam_angle));
  az_impact_t impact;
  az_ray_impact(state, beam_start, az_vpolar(5000, beam_angle),
                AZ_IMPF_NONE, baddie->uid, &impact);

  // The beam does far more damage to other baddies than to the ship.
  if (impact.type == AZ_IMP_BADDIE) {
    az_try_damage_baddie(
        state, impact.target.baddie.baddie, impact.target.baddie.component,
        AZ_DMGF_BEAM, 75.0 * power * time);
  } else if (impact.type == AZ_IMP_SHIP) {
    az_damage_ship(state, 15.0 * power * time, false);
  }

  // Add particles/sound for the beam.
  const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
  const az_color_t beam_color = {255, 128, alt, 192};
  az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
              power * (4.0 + 0.75 * az_clock_zigzag(8, 1, state->clock)));
  az_add_speck(state, AZ_WHITE, 1.0, impact.position,
               az_vpolar(az_random(20.0, 70.0),
                         az_vtheta(impact.normal) +
                         az_random(-AZ_HALF_PI, AZ_HALF_PI)));
}

static void fire_meltbeam(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  open_pincers(baddie, time);

  // Find the lowest ice crystal within range.
  az_baddie_t *lowest_crystal = NULL;
  double lowest_height = INFINITY;
  AZ_ARRAY_LOOP(other, state->baddies) {
    if (other->kind != AZ_BAD_ICE_CRYSTAL) continue;
    if (az_vwithin(baddie->position, other->position, MELTBEAM_MIN_RANGE)) {
      continue;
    }
    if (az_vwithin(baddie->position, other->position, MELTBEAM_MAX_RANGE)) {
      const double height = az_vnorm(other->position);
      if (height < lowest_height) {
        lowest_height = height;
        lowest_crystal = other;
      }
    }
  }

  // Aim the beam towards the lowest ice crystal, or towards the ship if there
  // are no ice crystals in range.
  if (lowest_crystal != NULL) {
    turn_eyes_towards(baddie, time, lowest_crystal->position);
  } else turn_eyes_towards_ship(state, baddie, time);
  fire_beam_from_eye(state, baddie, time, 0, 1.0);
  az_loop_sound(&state->soundboard, AZ_SND_BEAM_PHASE);

  // When the cooldown timer reachers zero, stop firing the beam.
  if (baddie->cooldown <= 0.0) {
    baddie->state = HIDING_STATE;
    baddie->cooldown = 0.5;
  }
}

static void fire_crossbeam(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  open_pincers(baddie, time);

  turn_eyes_towards_ship(state, baddie, time);
  fire_beam_from_eye(state, baddie, time, 1, 0.67);
  fire_beam_from_eye(state, baddie, time, 2, 0.67);
  az_loop_sound(&state->soundboard, AZ_SND_BEAM_NORMAL);

  // When the cooldown timer reachers zero, stop firing the beams.
  if (baddie->cooldown <= 0.0) {
    baddie->state = HIDING_STATE;
    baddie->cooldown = 0.25;
  }
}

/*===========================================================================*/

void az_tick_bad_kilofuge(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  init_leg_anchors(baddie->data);
  switch (baddie->state) {
    case MOVE_BODY_FORWARD_STATE:
      move_body(state, baddie, time, 1);
      break;
    case MOVE_EVEN_LEGS_FORWARD_STATE:
      move_legs(state, baddie, time, 0, 1);
      break;
    case MOVE_ODD_LEGS_FORWARD_STATE:
      move_legs(state, baddie, time, 1, 1);
      break;
    case MOVE_BODY_BACKWARD_STATE:
      move_body(state, baddie, time, -1);
      break;
    case MOVE_EVEN_LEGS_BACKWARD_STATE:
      move_legs(state, baddie, time, 0, -1);
      break;
    case MOVE_ODD_LEGS_BACKWARD_STATE:
      move_legs(state, baddie, time, 1, -1);
      break;
    case MELTBEAM_STATE:
      fire_meltbeam(state, baddie, time);
      break;
    case CROSSBEAM_STATE:
      fire_crossbeam(state, baddie, time);
      break;
    case WAITING_STATE:
      relax_pincers(baddie, time);
      if (baddie->cooldown <= 0.0) choose_next_state(state, baddie);
      break;
    case HIDING_STATE:
      close_pincers(baddie, time);
      if (baddie->cooldown <= 0.0) choose_next_state(state, baddie);
      break;
    default:
      baddie->state = WAITING_STATE;
      break;
  }
}

void az_on_kilofuge_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  const int knockback =
    ((damage_kind & AZ_DMGF_HYPER_ROCKET) ? 3 :
     (damage_kind & (AZ_DMGF_ROCKET | AZ_DMGF_BOMB)) ? 2 : 0);
  if (knockback > 0) {
    baddie->param = fmax(baddie->param, knockback);
    if (baddie->state == MELTBEAM_STATE || baddie->state == CROSSBEAM_STATE) {
      baddie->state = WAITING_STATE;
      baddie->cooldown = 0.0;
    }
  }
}

/*===========================================================================*/
