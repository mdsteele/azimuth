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

#include "azimuth/tick/baddie_oth_gunship.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/state/sound.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/baddie_oth.h"
#include "azimuth/tick/baddie_util.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// Primary states:
#define INITIAL_STATE 0
#define RETREAT_STATE 1
#define LOCK_ON_TRACTOR_STATE 2
#define SPIN_UP_CPLUS_STATE 3
#define CPLUS_READY_STATE 4
#define CPLUS_ACTIVE_STATE 5
#define DOGFIGHT_STATE 6
#define BARRAGE_STATE 7
#define CRAZY_STATE 8

// Engine constants:
#define TURN_RATE (AZ_DEG2RAD(285))
#define MAX_SPEED 300.0
#define FORWARD_ACCEL 300.0

// C-plus constants:
#define CPLUS_SPEED 1000.0
#define CPLUS_CHARGE_UP_TIME 4.0
#define CPLUS_DECAY_TIME 3.5

/*===========================================================================*/

static int get_primary_state(az_baddie_t *baddie) {
  return baddie->state & 0xff;
}

static int get_secondary_state(az_baddie_t *baddie) {
  return (baddie->state >> 8) & 0xff;
}

static void set_primary_state(az_baddie_t *baddie, int primary) {
  assert(primary >= 0 && primary <= 0xff);
  baddie->state = primary;
}

static void set_secondary_state(az_baddie_t *baddie, int secondary) {
  assert(secondary >= 0 && secondary <= 0xff);
  baddie->state = (baddie->state & 0xff) | (secondary << 8);
}

/*===========================================================================*/

#define TRACTOR_MIN_LENGTH 100.0
#define TRACTOR_MAX_LENGTH 150.0
#define TRACTOR_COMPONENT_INDEX 6

static bool tractor_locked_on(
    const az_baddie_t *baddie, az_vector_t *tractor_pos_out,
    double *tractor_length_out) {
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
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
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

/*===========================================================================*/

// Fly towards the goal position, getting as close as possible.
void fly_towards(az_space_state_t *state, az_baddie_t *baddie, double time,
                 az_vector_t goal) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  az_fly_towards_position(state, baddie, time, goal, TURN_RATE, MAX_SPEED,
                          FORWARD_ACCEL, 200, 100);
}

// Fly towards the ship, but try not to be too close.
void fly_towards_ship(az_space_state_t *state, az_baddie_t *baddie,
                      double time) {
  az_fly_towards_ship(state, baddie, time, TURN_RATE, MAX_SPEED,
                      FORWARD_ACCEL, 200, 200, 100);
}

void begin_retreat(az_baddie_t *baddie, double hurt) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  set_primary_state(baddie, RETREAT_STATE);
  set_secondary_state(baddie, 1 + (int)(hurt * 8));
  baddie->cooldown = 0.5;
}

void begin_dogfight(az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  set_primary_state(baddie, DOGFIGHT_STATE);
  set_secondary_state(baddie, az_randint(20, 40));
  baddie->cooldown = 0.2;
}

/*===========================================================================*/

void az_tick_bad_oth_gunship(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  const double old_angle = baddie->angle;
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;

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
  }

  // When very low on health, go crazy:
  if (hurt >= 0.95) set_primary_state(baddie, CRAZY_STATE);

  switch (get_primary_state(baddie)) {
    case INITIAL_STATE:
      baddie->cooldown = CPLUS_DECAY_TIME;
      set_primary_state(baddie, CPLUS_READY_STATE);
      break;
    case RETREAT_STATE: {
      const int secondary = get_secondary_state(baddie);
      // Drop Oth Razors behind us as we flee.
      if (secondary > 0 && baddie->cooldown <= 0.0) {
        for (int i = -1; i <= 1; i += 2) {
          az_add_baddie(state, AZ_BAD_OTH_RAZOR_2, baddie->position,
                        baddie->angle + AZ_PI + i * AZ_DEG2RAD(30));
        }
        az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
        set_secondary_state(baddie, secondary - 1);
        baddie->cooldown = (secondary > 1 ? 0.5 : 5.0);
      }
      // Out of all marker nodes the Oth Gunship can see, pick the one farthest
      // from the ship.
      double best_dist = 0.0;
      az_vector_t target = baddie->position;
      AZ_ARRAY_LOOP(node, state->nodes) {
        if (node->kind != AZ_NODE_MARKER) continue;
        if (az_baddie_has_clear_path_to_position(state, baddie,
                                                 node->position)) {
          const double dist = az_vdist(node->position, state->ship.position);
          if (dist > best_dist) {
            best_dist = dist;
            target = node->position;
          }
        }
      }
      // If we've reached the target position (or if the cooldown expires),
      // change states.  Otherwise, fly towards the target position.
      if ((secondary == 0 && (baddie->cooldown <= 0.0 ||
                              !az_ship_in_range(state, baddie, 800))) ||
          az_vwithin(baddie->position, target, 50.0)) {
        baddie->cooldown = 0.0;
        if (!az_ship_in_range(state, baddie, 500)) {
          set_primary_state(baddie, LOCK_ON_TRACTOR_STATE);
        } else {
          for (int i = 0; i < 360; i += 15) {
            az_fire_baddie_projectile(
                state, baddie, AZ_PROJ_OTH_SPRAY,
                baddie->data->main_body.bounding_radius, AZ_DEG2RAD(i), 0);
          }
          az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
          begin_dogfight(baddie);
        }
      } else {
        fly_towards(state, baddie, time, target);
      }
    } break;
    case LOCK_ON_TRACTOR_STATE: {
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
      if (nearest == NULL) {
        begin_dogfight(baddie);
        break;
      }
      if (best_dist <= TRACTOR_MAX_LENGTH) {
        set_tractor_node(baddie, nearest);
        set_primary_state(baddie, SPIN_UP_CPLUS_STATE);
        baddie->cooldown = CPLUS_CHARGE_UP_TIME;
      } else fly_towards(state, baddie, time, nearest->position);
    } break;
    case SPIN_UP_CPLUS_STATE: {
      az_vector_t tractor_pos;
      if (tractor_locked_on(baddie, &tractor_pos, NULL)) {
        const az_vector_t forward = az_vpolar(1, baddie->angle);
        const az_vector_t axis = az_vsub(baddie->position, tractor_pos);
        const az_vector_t projected = az_vflatten(forward, axis);
        const double goal_theta = az_vtheta(projected);
        baddie->angle =
          az_angle_towards(baddie->angle, TURN_RATE * time, goal_theta);
        baddie->velocity = az_vflatten(az_vcaplen(az_vadd(
            baddie->velocity, az_vpolar(FORWARD_ACCEL * time, baddie->angle)),
                                                  500.0), axis);
        if (baddie->cooldown <= 2.0) {
          az_particle_t *particle;
          for (int offset = -30; offset <= 30; offset += 60) {
            if (az_insert_particle(state, &particle)) {
              particle->kind = AZ_PAR_OTH_FRAGMENT;
              particle->color = (az_color_t){192, 192, 192,
                  255 * (0.25 + 0.25 * (2.0 - baddie->cooldown))};
              particle->position =
                az_vadd(baddie->position,
                        az_vpolar(-17.0, baddie->angle + AZ_DEG2RAD(offset)));
              particle->velocity = AZ_VZERO;
              particle->lifetime = 0.5;
              particle->param1 = 8.0;
            }
          }
        }
        if (baddie->cooldown <= 0.0) {
          az_play_sound(&state->soundboard, AZ_SND_CPLUS_CHARGED);
          set_tractor_node(baddie, NULL);
          set_primary_state(baddie, CPLUS_READY_STATE);
          baddie->cooldown = CPLUS_DECAY_TIME;
        }
      } else begin_dogfight(baddie);
    } break;
    case CPLUS_READY_STATE: {
      if (baddie->cooldown <= 0.0) {
        begin_dogfight(baddie);
        break;
      }
      az_loop_sound(&state->soundboard, AZ_SND_CPLUS_READY);
      az_vector_t rel_impact;
      if (az_lead_target(az_vsub(state->ship.position, baddie->position),
                         state->ship.velocity, CPLUS_SPEED, &rel_impact)) {
        const double goal_theta = az_vtheta(rel_impact);
        baddie->angle =
          az_angle_towards(baddie->angle, TURN_RATE * time, goal_theta);
        const az_vector_t goal_pos = az_vadd(baddie->position, rel_impact);
        if (fabs(az_mod2pi(baddie->angle - goal_theta)) <= AZ_DEG2RAD(1) &&
            az_baddie_has_clear_path_to_position(state, baddie, goal_pos)) {
          baddie->velocity = az_vpolar(CPLUS_SPEED, baddie->angle);
          set_primary_state(baddie, CPLUS_ACTIVE_STATE);
          // TODO: Fix this sound; either use persisted sounds so that we can
          // cut it short, or use an alternate, shorter sound.
          az_play_sound(&state->soundboard, AZ_SND_CPLUS_ACTIVE);
        } else {
          fly_towards(state, baddie, time, goal_pos);
        }
      } else {
        fly_towards(state, baddie, time, state->ship.position);
      }
    } break;
    case CPLUS_ACTIVE_STATE: {
      baddie->temp_properties |= AZ_BADF_QUAD_IMPACT;
      if (fabs(az_mod2pi(az_vtheta(baddie->velocity) -
                         baddie->angle)) > AZ_DEG2RAD(5)) {
        az_play_sound(&state->soundboard, AZ_SND_CPLUS_IMPACT);
        begin_dogfight(baddie);
      } else {
        az_particle_t *particle;
        if (az_insert_particle(state, &particle)) {
          particle->kind = AZ_PAR_OTH_FRAGMENT;
          particle->color = (az_color_t){192, 255, 192, 255};
          particle->position =
            az_vadd(baddie->position, az_vpolar(-15.0, baddie->angle));
          particle->velocity = AZ_VZERO;
          particle->lifetime = 0.3;
          particle->param1 = 16;
        }
      }
    } break;
    case DOGFIGHT_STATE: {
      set_tractor_node(baddie, NULL);
      fly_towards_ship(state, baddie, time);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(3)) &&
          az_can_see_ship(state, baddie)) {
        int secondary = get_secondary_state(baddie);
        if (hurt >= 0.75 && secondary < 5) {
          az_play_sound(&state->soundboard, AZ_SND_MAGBEEST_MAGNET_CHARGE);
          set_primary_state(baddie, BARRAGE_STATE);
          baddie->cooldown = 2.0;
          break;
        } else if (hurt >= 0.45 && secondary < 5 &&
                   !az_ship_in_range(state, baddie, 150)) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_ROCKET,
                                    20.0, 0.0, 0.0);
          az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
          secondary -= 5;
        } else if (hurt >= 0.25 && secondary < 15 * hurt &&
                   !az_ship_in_range(state, baddie, 150)) {
          az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_MINIROCKET,
                                    20.0, 0.0, 0.0);
          az_play_sound(&state->soundboard, AZ_SND_FIRE_ROCKET);
          secondary -= 2;
        } else {
          if (hurt >= 0.60) {
            for (int i = -1; i <= 1; ++i) {
              az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                        20.0, 0.0, AZ_DEG2RAD(10) * i);
            }
          } else if (hurt >= 0.3) {
            for (int i = -1; i <= 1; i += 2) {
              az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                        20.0, 0.0, AZ_DEG2RAD(5) * i);
            }
          } else {
            az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BULLET,
                                      20.0, 0.0, 0.0);
          }
          az_play_sound(&state->soundboard, AZ_SND_FIRE_GUN_NORMAL);
          secondary -= 1;
        }
        baddie->cooldown = 0.2;
        if (secondary > 0) {
          set_secondary_state(baddie, secondary);
        } else {
          begin_retreat(baddie, hurt);
        }
      }
    } break;
    case BARRAGE_STATE: {
      fly_towards_ship(state, baddie, time);
      if (baddie->cooldown <= 0.0 &&
          az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(7))) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_OTH_BARRAGE,
                                  20.0, 0.0, 0.0);
        begin_retreat(baddie, hurt);
      }
    } break;
    case CRAZY_STATE: {
      set_tractor_node(baddie, NULL);
      fly_towards(state, baddie, time, state->ship.position);
      if (baddie->cooldown <= 0.0) {
        az_projectile_t *proj = az_fire_baddie_projectile(
            state, baddie, AZ_PROJ_OTH_ROCKET, 20.0, 0.0, 0.0);
        if (proj != NULL) {
          proj->power = 0.5;
          az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
        }
        baddie->cooldown = 0.75;
      }
    } break;
    default:
      begin_dogfight(baddie);
      break;
  }
  az_tick_oth_tendrils(state, baddie, &AZ_OTH_GUNSHIP_TENDRILS, old_angle,
                       time);
}

void az_on_oth_gunship_damaged(
    az_space_state_t *state, az_baddie_t *baddie, double amount,
    az_damage_flags_t damage_kind) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  if (damage_kind & (AZ_DMGF_ROCKET | AZ_DMGF_BOMB)) {
    set_tractor_node(baddie, NULL);
    const int primary = get_primary_state(baddie);
    if (primary == SPIN_UP_CPLUS_STATE) {
      begin_dogfight(baddie);
    } else if (primary == DOGFIGHT_STATE) {
      const int secondary = get_secondary_state(baddie);
      if (secondary >= 5) {
        set_secondary_state(baddie, secondary - 2);
      }
    }
    double theta =
      az_vtheta(az_vrot90ccw(az_vsub(baddie->position, state->ship.position)));
    if (az_randint(0, 1)) theta = -theta;
    az_vpluseq(&baddie->velocity, az_vpolar(250, theta));
  }
}

/*===========================================================================*/

void az_tick_bad_reflection(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_REFLECTION);
  if (baddie->state == 0 && !az_ship_is_alive(&state->ship)) {
    baddie->param = -1.0;
    return;
  }
  const az_vector_t rel_pos =
    az_vrotate(az_vsub(state->ship.position, baddie->position),
               -baddie->angle);
  const double rel_angle = az_mod2pi(state->ship.angle - baddie->angle);
  baddie->components[0].position = (az_vector_t){rel_pos.x, -rel_pos.y};
  baddie->components[0].angle = -rel_angle;
  if (baddie->state == 0) {
    baddie->param = 0.0;
  } else {
    const double nps_lifetime = 3.0;
    if (baddie->param == 0.0) {
      state->camera.wobble_goal = 0.5 * nps_lifetime;
      az_play_sound(&state->soundboard, AZ_SND_NPS_PORTAL);
    }
    baddie->param = fmin(1.0, baddie->param + time / (0.5 * nps_lifetime));
    if (baddie->param >= 1.0) {
      const az_vector_t abs_position =
        az_vadd(az_vrotate(baddie->components[0].position, baddie->angle),
                baddie->position);
      const double abs_angle =
        az_vtheta(az_vsub(state->ship.position, abs_position));
      az_particle_t *particle;
      if (az_insert_particle(state, &particle)) {
        particle->kind = AZ_PAR_NPS_PORTAL;
        particle->color = (az_color_t){128, 64, 255, 255};
        particle->position = abs_position;
        particle->velocity = AZ_VZERO;
        particle->angle = 0.0;
        particle->age = 0.5 * nps_lifetime;
        particle->lifetime = nps_lifetime;
        particle->param1 = 50.0 * sqrt(nps_lifetime);
      }
      const az_script_t *script = baddie->on_kill;
      az_init_baddie(baddie, AZ_BAD_OTH_GUNSHIP, abs_position, abs_angle);
      baddie->on_kill = script;
    }
  }
}

/*===========================================================================*/
