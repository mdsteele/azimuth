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

#include "azimuth/tick/baddie_forcefiend.h"

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

// Primary states:
#define CHASE_STATE 0
#define SEEK_LEFT_STATE 1
#define SEEK_RIGHT_STATE 2
#define FORCE_FLURRY_STATE 3

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

void az_tick_bad_forcefiend(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_FORCEFIEND);
  const az_camera_bounds_t *bounds = az_current_camera_bounds(state);
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  // Open/close jaws:
  const double jaw_angle =
    AZ_DEG2RAD(30) * (0.5 + 0.5 * sin(3.0 * state->ship.player.total_time));
  baddie->components[0].angle = az_angle_towards(
      baddie->components[0].angle, AZ_DEG2RAD(90) * time, jaw_angle);
  baddie->components[1].angle = az_angle_towards(
      baddie->components[1].angle, AZ_DEG2RAD(90) * time, -jaw_angle);
  // If the ship is destroyed or we run it over, move away from it.
  if (!az_ship_is_decloaked(&state->ship) ||
      az_vwithin(baddie->position, state->ship.position, 30.0)) {
    if (az_mod2pi(az_vtheta(state->ship.position) -
                  (bounds->min_theta + 0.5 * bounds->theta_span)) < 0) {
      set_primary_state(baddie, SEEK_LEFT_STATE);
    } else set_primary_state(baddie, SEEK_RIGHT_STATE);
  }
  // CHASE_STATE: Chase ship and fire homing torpedoes.
  const int primary = get_primary_state(baddie);
  if (primary == CHASE_STATE) {
    az_snake_towards(state, baddie, time, 8, 130 + 130 * hurt, 150,
                     state->ship.position, true);
    if (baddie->cooldown <= 0.0 && az_can_see_ship(state, baddie) &&
        az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(20))) {
      if (az_random(0, 1) < 0.5) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_TRINE_TORPEDO,
                                  20, 0, 0);
        if (az_random(0, 1) < 0.5) {
          if (az_mod2pi(az_vtheta(state->ship.position) -
                        (bounds->min_theta + 0.5 * bounds->theta_span)) < 0) {
            set_primary_state(baddie, SEEK_LEFT_STATE);
          } else set_primary_state(baddie, SEEK_RIGHT_STATE);
        }
      } else {
        bool any_gravity_torps = false;
        AZ_ARRAY_LOOP(proj, state->projectiles) {
          if (proj->kind == AZ_PROJ_GRAVITY_TORPEDO ||
              proj->kind == AZ_PROJ_GRAVITY_TORPEDO_WELL) {
            any_gravity_torps = true;
            break;
          }
        }
        if (!any_gravity_torps) {
          for (int i = -1; i <= 1; ++i) {
            az_fire_baddie_projectile(state, baddie, AZ_PROJ_GRAVITY_TORPEDO,
                                      20, 0, i * AZ_DEG2RAD(20));
          }
          az_play_sound(&state->soundboard, AZ_SND_FIRE_GRAVITY_TORPEDO);
        }
      }
      int num_eggs = 0;
      AZ_ARRAY_LOOP(other, state->baddies) {
        if (other->kind == AZ_BAD_FORCE_EGG) ++num_eggs;
      }
      if (num_eggs < 8) {
        az_add_baddie(state, AZ_BAD_FORCE_EGG,
                      baddie->position, baddie->angle);
      }
      baddie->cooldown = 2.0 - 0.8 * hurt;
    }
  }
  // SEEK_LEFT/RIGHT_STATE: Seek to left/right side of room.
  else if (primary == SEEK_LEFT_STATE || primary == SEEK_RIGHT_STATE) {
    const double r = bounds->min_r + 0.5 * bounds->r_span;
    const double dt = 150.0 / r;
    const az_vector_t dest = az_vpolar(r, bounds->min_theta +
        (primary == SEEK_LEFT_STATE ? bounds->theta_span + dt : -dt));
    az_snake_towards(state, baddie, time, 8, 200 + 150 * hurt,
                     150 + 150 * hurt, dest, true);
    if (az_ship_is_alive(&state->ship) &&
        az_vwithin(baddie->position, dest, 100.0)) {
      set_primary_state(baddie, FORCE_FLURRY_STATE);
      set_secondary_state(baddie, az_randint(10, 15));
      if (hurt > 0.5) {
        az_fire_baddie_projectile(state, baddie, AZ_PROJ_TRINE_TORPEDO,
                                  20, 0, 0);
      }
    }
  }
  // FORCE_FLURRY_STATE: Shoot a flurry of force waves.
  else if (primary == FORCE_FLURRY_STATE) {
    baddie->angle = az_angle_towards(baddie->angle, AZ_DEG2RAD(180) * time,
        az_vtheta(az_vsub(state->ship.position, baddie->position)));
    if (baddie->cooldown <= 0.0 &&
        az_ship_within_angle(state, baddie, 0, AZ_DEG2RAD(60))) {
      az_fire_baddie_projectile(state, baddie, AZ_PROJ_FORCE_WAVE, 0, 0,
                                az_random(-1.3, 1.3));
      baddie->cooldown = 0.3;
      const int remaining = get_secondary_state(baddie) - 1;
      if (remaining > 0) set_secondary_state(baddie, remaining);
      else set_primary_state(baddie, CHASE_STATE);
    }
    if (!az_ship_is_decloaked(&state->ship) ||
        az_vwithin(state->ship.position, baddie->position, 150.0)) {
      set_primary_state(baddie, CHASE_STATE);
    }
  }
  // Move claws:
  // TODO: When ship gets in front of Forcefiend, swipe with one of the claws.
  const double offset = baddie->components[9].position.y;
  baddie->components[4].position = (az_vector_t){-54 + 0.02 * offset*offset,
      34.64 + (offset < 0 ? 0.5 * offset : offset)};
  baddie->components[7].position = (az_vector_t){-54 + 0.02 * offset*offset,
      -34.64 + (offset > 0 ? 0.5 * offset : offset)};
  baddie->components[4].angle = baddie->components[7].angle =
    az_mod2pi(baddie->components[9].angle + AZ_PI);
  // Move arms to fit claws:
  for (int i = 2; i <= 5; i += 3) {
    const az_vector_t wrist = baddie->components[i+2].position;
    const double dist = az_vnorm(wrist);
    static const double upper = 40, lower = 34;
    const double cosine = (upper*upper + dist*dist - lower*lower) /
      (2.0 * upper * dist);
    const double gamma = acos(fmin(fmax(-1, cosine), 1));
    const double theta = az_vtheta(wrist) +
      ((i % 2) ^ (wrist.x < 0) ? -gamma : gamma);
    const az_vector_t elbow = az_vpolar(upper, theta);
    baddie->components[i].angle = az_mod2pi(theta);
    baddie->components[i+1].position = elbow;
    baddie->components[i+1].angle = az_vtheta(az_vsub(wrist, elbow));
  }
}

/*===========================================================================*/

void az_tick_bad_force_egg(az_space_state_t *state, az_baddie_t *baddie,
                           bool bounced, double time) {
  assert(baddie->kind == AZ_BAD_FORCE_EGG);
  baddie->angle = az_mod2pi(baddie->angle +
                            time * 0.006 * az_vnorm(baddie->velocity));
  // If the egg hits a wall, hatch it.
  if (bounced) {
    const az_vector_t position = baddie->position;
    az_kill_baddie(state, baddie);
    for (int i = -1; i <= 1; i += 2) {
      az_baddie_t *forceling = az_add_baddie(
          state, AZ_BAD_FORCELING, position,
          az_mod2pi(baddie->angle + i * AZ_DEG2RAD(90)));
      if (forceling != NULL) forceling->state = i;
    }
    return;
  }
  // Apply drag.
  const double drag_coeff = 1.0 / 200.0;
  const az_vector_t drag_force =
    az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
  baddie->velocity = az_vadd(baddie->velocity, az_vmul(drag_force, time));
  // Accelerate away from walls/doors and from other Force Eggs.
  az_vector_t closest_pos = AZ_VZERO;
  double closest_dist = INFINITY;
  AZ_ARRAY_LOOP(other, state->baddies) {
    if (other->kind != AZ_BAD_FORCE_EGG) continue;
    if (other == baddie) continue;
    const double dist = az_vdist(other->position, baddie->position);
    if (dist < closest_dist) {
      closest_pos = other->position;
      closest_dist = dist;
    }
  }
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    const double dist = az_vdist(door->position, baddie->position);
    if (dist < closest_dist) {
      closest_pos = door->position;
      closest_dist = dist;
    }
  }
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    const double dist = az_vdist(wall->position, baddie->position);
    if (dist < closest_dist) {
      closest_pos = wall->position;
      closest_dist = dist;
    }
  }
  az_vpluseq(&baddie->velocity,
             az_vwithlen(az_vsub(baddie->position, closest_pos), 50.0 * time));
}

void az_tick_bad_forceling(az_space_state_t *state, az_baddie_t *baddie,
                           double time) {
  assert(baddie->kind == AZ_BAD_FORCELING);
  az_snake_towards(state, baddie, time, 0, 100.0, 150.0,
                   az_vadd(az_vpolar(150, state->ship.angle +
                                     baddie->state * AZ_DEG2RAD(90)),
                           state->ship.position), true);
}

static void tick_fish(az_space_state_t *state, az_baddie_t *baddie,
                      double chase_range, double chase_speed,
                      double roam_speed, double wiggle, double time) {
  // If the ship is nearby and visible, chase it.
  if (az_ship_in_range(state, baddie, chase_range) &&
      az_can_see_ship(state, baddie)) {
    az_snake_towards(state, baddie, time, 0, chase_speed, wiggle,
                     state->ship.position, false);
  } else {
    // Otherwise, we'll head in one of eight directions, determined by the
    // baddie's current state.
    const double abs_angle = az_vtheta(baddie->position) +
      AZ_DEG2RAD(45) * az_modulo(baddie->state, 8);
    const double rel_angle = az_mod2pi(abs_angle - baddie->angle);
    const az_vector_t dest =
      az_vadd(baddie->position, az_vpolar(300.0, abs_angle));
    // Check if we're about to hit a wall.
    const double dist =
      az_baddie_dist_to_wall(state, baddie, 100.0, rel_angle);
    // Head towards our current destination.
    az_snake_towards(state, baddie, time, 0, roam_speed, wiggle, dest, false);
    // If we're getting near a wall ahead, turn back.
    if (dist <= 90.0) {
      const int shift = (az_randint(0, 1) ? 3 : -3);
      baddie->state = az_modulo(baddie->state + shift, 8);
    }
  }
}

void az_tick_bad_small_fish(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_SMALL_FISH);
  tick_fish(state, baddie, 180.0, 100.0, 70.0, 150.0, time);
}

void az_tick_bad_large_fish(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_LARGE_FISH);
  tick_fish(state, baddie, 270.0, 150.0, 70.0, 30.0, time);
}

/*===========================================================================*/
