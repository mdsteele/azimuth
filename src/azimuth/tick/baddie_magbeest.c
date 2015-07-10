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

/*===========================================================================*/

// Head states:
#define HEAD_POPDOWN_STATE 0
#define HEAD_POPUP_STATE 1
#define HEAD_LASER_SEEK_STATE 2
#define HEAD_WAITING_STATE 3
#define HEAD_QUICK_POPUP_STATE 4
#define HEAD_QUICK_POPDOWN_STATE 5
// Shared legs states:
#define LEGS_POPDOWN_STATE 0
#define LEGS_POPUP_STATE 1
// Legs-L states:
#define MAGNET_FUSION_BEAM_RECOVER_STATE 2
#define MAGNET_FUSION_BEAM_CHARGE_STATE 3
#define MAGNET_FINISH_ATTRACT_STATE 4
#define MAGNET_START_SPAWN_STATE (MAGNET_FINISH_ATTRACT_STATE + 20)
// Legs-R states:
#define GATLING_STOP_FIRING_STATE 2
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

static az_vector_t choose_random_marker_position(az_space_state_t *state) {
  az_vector_t positions[4];
  shuffle_marker_positions(state, positions, AZ_ARRAY_SIZE(positions));
  return positions[0];
}

/*===========================================================================*/

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
                        az_vector_t abs_foot_pos) {
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
                 az_vpolar(1, baddie->angle));
  shin->position = rel_foot_pos;
  shin->angle = az_vtheta(az_vsub(rel_knee_pos, rel_foot_pos));
  thigh->angle = az_vtheta(az_vsub(rel_knee_pos, rel_hip_pos));
}

static void legs_pop_up_down(az_space_state_t *state, az_baddie_t *baddie,
                             double delta) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  az_shake_camera(&state->camera, 0.0, 1.0);
  const az_vector_t abs_foot_0_pos =
    az_vadd(az_vrotate(baddie->components[2].position, baddie->angle),
            baddie->position);
  const az_vector_t abs_foot_1_pos =
    az_vadd(az_vrotate(baddie->components[4].position, baddie->angle),
            baddie->position);
  az_vpluseq(&baddie->position, az_vwithlen(baddie->position, delta));
  arrange_leg(baddie, 0, abs_foot_0_pos);
  arrange_leg(baddie, 1, abs_foot_1_pos);
}

static void init_legs_popup(az_baddie_t *baddie, az_vector_t marker_position) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  baddie->position = marker_position;
  baddie->angle = az_vtheta(baddie->position);
  arrange_leg(baddie, 0, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle - AZ_DEG2RAD(135))));
  arrange_leg(baddie, 1, az_vadd(baddie->position,
      az_vpolar(50, baddie->angle + AZ_DEG2RAD(135))));
  baddie->state = LEGS_POPUP_STATE;
  baddie->cooldown = 2.0;
}

/*===========================================================================*/

void az_tick_bad_magbeest_head(az_space_state_t *state, az_baddie_t *baddie,
                               double time) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  az_baddie_t *legs_l = lookup_legs(state, baddie, 0);
  az_baddie_t *legs_r = lookup_legs(state, baddie, 1);
  if (legs_l == NULL || legs_l->kind != AZ_BAD_MAGBEEST_LEGS_L ||
      legs_r == NULL || legs_r->kind != AZ_BAD_MAGBEEST_LEGS_R) return;
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  const double baseline =
    az_current_camera_bounds(state)->min_r - AZ_SCREEN_HEIGHT/2;
  const double head_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case HEAD_POPDOWN_STATE:
      turn_eye_towards(state, baddie, az_vmul(baddie->position, 2), time);
      if (head_top > baseline) {
        head_pop_up_down(state, baddie, -HEAD_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      if (baddie->cooldown <= 0.0) {
        az_vector_t marker_positions[4];
        if (hurt >= 0.5) {
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
            const bool both = hurt >= 0.25;
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
        init_head_popup(baddie, marker_positions[new_index]);
        baddie->state = HEAD_QUICK_POPUP_STATE;
      }
      break;
    default:
      baddie->state = HEAD_POPDOWN_STATE;
      break;
  }
  // TODO: control legs
  // TODO: occasionally cause scrap metal to fall from ceiling?
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
  az_component_t *magnet = &baddie->components[0];
  const az_vector_t magnet_abs_position =
    az_vadd(az_vrotate(az_vadd(az_vpolar(20.0, magnet->angle),
                               magnet->position), baddie->angle),
            baddie->position);
  // Fusion beam:
  if (baddie->state == MAGNET_FUSION_BEAM_CHARGE_STATE) {
    const double factor = baddie->cooldown / MAGNET_FUSION_BEAM_CHARGE_TIME;
    // Aim the magnet at the ship:
    if (az_ship_is_decloaked(&state->ship)) {
      const double turn_rate = AZ_DEG2RAD(40) + AZ_DEG2RAD(60) * factor;
      magnet->angle = az_mod2pi(az_angle_towards(
          magnet->angle + baddie->angle, turn_rate * time,
          az_vtheta(az_vsub(state->ship.position, magnet_abs_position))) -
                                baddie->angle);
    }
    const double magnet_abs_angle = az_mod2pi(magnet->angle + baddie->angle);
    // Apply force to ship:
    if (az_ship_is_alive(&state->ship)) {
      const az_vector_t delta =
        az_vsub(state->ship.position, magnet_abs_position);
      const double arc = AZ_DEG2RAD(10) * factor;
      if (az_mod2pi(az_vtheta(delta) - magnet_abs_angle) <= arc) {
        az_vpluseq(&state->ship.velocity, az_vwithlen(delta, -300.0 * time));
      }
    }
    // Fire:
    if (baddie->cooldown <= 0.0) {
      az_add_projectile(state, AZ_PROJ_MAGNET_FUSION_BEAM, magnet_abs_position,
                        magnet_abs_angle, 1.0, baddie->uid);
      baddie->state = MAGNET_FUSION_BEAM_RECOVER_STATE;
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
          az_vtheta(az_vsub(state->ship.position, magnet_abs_position))) -
                                baddie->angle);
      // Pull scrap metal along with swinging magnetetic field:
      const double dtheta = az_mod2pi(magnet->angle - old_angle);
      AZ_ARRAY_LOOP(scrap, state->baddies) {
        if (scrap->kind != AZ_BAD_SCRAP_METAL) continue;
        scrap->position =
          az_vadd(az_vrotate(az_vsub(scrap->position, magnet_abs_position),
                             0.7 * dtheta), magnet_abs_position);
      }
    }
    const double magnet_abs_angle = az_mod2pi(magnet->angle + baddie->angle);
    // Spawn scrap metal:
    if (baddie->state != MAGNET_FINISH_ATTRACT_STATE &&
        baddie->cooldown <= 0.0) {
      az_add_baddie(state, AZ_BAD_SCRAP_METAL,
                    az_vadd(magnet_abs_position,
                            az_vpolar(1500.0, magnet_abs_angle +
                                      az_random(-AZ_DEG2RAD(10),
                                                AZ_DEG2RAD(10)))),
                    az_random(-AZ_PI, AZ_PI));
      --baddie->state;
      baddie->cooldown = 0.1;
    }
    // Apply force to scrap metal:
    double max_scrap_dist = 0.0;
    AZ_ARRAY_LOOP(scrap, state->baddies) {
      if (scrap->kind != AZ_BAD_SCRAP_METAL) continue;
      az_vpluseq(&scrap->position, az_vwithlen(
          az_vsub(scrap->position, magnet_abs_position), -500.0 * time));
      scrap->angle = az_mod2pi(scrap->angle + AZ_DEG2RAD(300) * time);
      max_scrap_dist =
        fmax(max_scrap_dist, az_vdist(scrap->position, magnet_abs_position));
    }
    az_loop_sound(&state->soundboard, AZ_SND_TRACTOR_BEAM);
    // Apply force to ship:
    if (az_ship_is_alive(&state->ship)) {
      const az_vector_t delta =
        az_vsub(state->ship.position, magnet_abs_position);
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
      baddie->state = LEGS_POPDOWN_STATE;
    }
    return;
  }
  // Pop up/down:
  const double baseline =
    az_current_camera_bounds(state)->min_r - AZ_SCREEN_HEIGHT/2;
  const double base_top =
    az_vnorm(baddie->position) + baddie->data->main_body.bounding_radius;
  switch (baddie->state) {
    case LEGS_POPDOWN_STATE:
      if (base_top > baseline) {
        legs_pop_up_down(state, baddie, -LEGS_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      break;
    case LEGS_POPUP_STATE:
      if (base_top < baseline + LEGS_POPUP_DIST) {
        legs_pop_up_down(state, baddie, LEGS_POP_SPEED * time);
      }
      if (baddie->cooldown <= 0.0) {
        if (az_random(0.0, 1.0) < 1.0 - pow(0.75, baddie->param)) {
          baddie->state = MAGNET_FUSION_BEAM_CHARGE_STATE;
          baddie->cooldown = MAGNET_FUSION_BEAM_CHARGE_TIME;
          baddie->param = 0.0;
        } else {
          baddie->state = MAGNET_START_SPAWN_STATE;
          baddie->param += 1.0;
        }
      }
      break;
    case MAGNET_FUSION_BEAM_RECOVER_STATE:
      if (baddie->cooldown <= 0.0) {
        baddie->state = LEGS_POPDOWN_STATE;
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
    case LEGS_POPDOWN_STATE:
      if (base_top > baseline) {
        legs_pop_up_down(state, baddie, -LEGS_POP_SPEED * time);
      } else {
        baddie->temp_properties |= AZ_BADF_INCORPOREAL | AZ_BADF_NO_HOMING;
      }
      break;
    case LEGS_POPUP_STATE:
      if (base_top < baseline + LEGS_POPUP_DIST) {
        legs_pop_up_down(state, baddie, LEGS_POP_SPEED * time);
      }
      if (baddie->cooldown <= 0.0) {
        baddie->state = GATLING_START_FIRING_STATE;
      }
      break;
    case GATLING_STOP_FIRING_STATE:
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
  }
}

/*===========================================================================*/
