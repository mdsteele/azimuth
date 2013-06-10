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

#include "azimuth/tick/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/constants.h"
#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/tick/object.h"
#include "azimuth/tick/script.h"
#include "azimuth/tick/ship.h" // for az_on_baddie_hit_ship
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static bool angle_to_ship_within(az_space_state_t *state, az_baddie_t *baddie,
                                 double offset, double angle) {
  return (az_ship_is_present(&state->ship) &&
          fabs(az_mod2pi(baddie->angle + offset -
                         az_vtheta(az_vsub(state->ship.position,
                                           baddie->position)))) <= angle);
}

static bool has_line_of_sight_to_ship(az_space_state_t *state,
                                      az_baddie_t *baddie) {
  if (!az_ship_is_present(&state->ship)) return false;
  az_impact_t impact;
  az_ray_impact(state, baddie->position,
                az_vsub(state->ship.position, baddie->position),
                AZ_IMPF_BADDIE, baddie->uid, &impact);
  return (impact.type == AZ_IMP_SHIP);
}

static double dist_until_hit_wall(az_space_state_t *state, az_baddie_t *baddie,
                                  double max_dist, double rel_angle) {
  az_impact_t impact;
  az_circle_impact(
      state, baddie->data->overall_bounding_radius, baddie->position,
      az_vpolar(max_dist, baddie->angle + rel_angle),
      (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  return az_vdist(baddie->position, impact.position);
}

static az_projectile_t *fire_projectile(
    az_space_state_t *state, az_baddie_t *baddie, az_proj_kind_t kind,
    double forward, double firing_angle, double proj_angle_offset) {
  const double theta = firing_angle + baddie->angle;
  return az_add_projectile(
      state, kind, true, az_vadd(baddie->position, az_vpolar(forward, theta)),
      theta + proj_angle_offset, 1.0);
}

static az_vector_t force_field(
    az_space_state_t *state, az_baddie_t *baddie,
    double ship_coeff, double ship_min_range, double ship_max_range,
    double wall_far_coeff, double wall_near_coeff) {
  const az_vector_t pos = baddie->position;
  az_vector_t drift = AZ_VZERO;
  if (az_ship_is_present(&state->ship) &&
      az_vwithin(state->ship.position, pos, ship_max_range)) {
    if (az_vwithin(state->ship.position, pos, ship_min_range)) {
      drift = az_vwithlen(az_vsub(state->ship.position, pos), -ship_coeff);
    } else {
      drift = az_vwithlen(az_vsub(state->ship.position, pos), ship_coeff);
    }
  }
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    const az_vector_t delta = az_vsub(pos, door->position);
    const double dist = az_vnorm(delta) - AZ_DOOR_BOUNDING_RADIUS -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      az_vpluseq(&drift, az_vwithlen(delta, wall_near_coeff));
    } else {
      az_vpluseq(&drift, az_vwithlen(delta, wall_far_coeff * exp(-dist)));
    }
  }
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    const az_vector_t delta = az_vsub(pos, wall->position);
    const double dist = az_vnorm(delta) - wall->data->bounding_radius -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      az_vpluseq(&drift, az_vwithlen(delta, wall_near_coeff));
    } else {
      az_vpluseq(&drift, az_vwithlen(delta, wall_far_coeff * exp(-dist)));
    }
  }
  return drift;
}

static void drift_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_speed, double ship_force, double wall_force) {
  const az_vector_t drift = force_field(state, baddie, ship_force, 0.0, 1000.0,
                                        wall_force, 2.0 * wall_force);
  baddie->velocity =
    az_vcaplen(az_vadd(baddie->velocity, az_vmul(drift, time)), max_speed);
}

static void fly_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double turn_rate, double max_speed, double forward_accel,
    double lateral_decel_rate, double attack_range, double ship_coeff) {
  const double backward_accel = 80.0;
  const az_vector_t drift =
    (baddie->cooldown > 1.0 ?
     force_field(state, baddie, -100.0, 0.0, 500.0, 100.0, 200.0) :
     force_field(state, baddie, ship_coeff, attack_range, 1000.0, 100.0,
                 200.0));
  const double goal_theta =
    az_vtheta(baddie->cooldown <= 0.0 &&
              az_vwithin(baddie->position, state->ship.position, 120.0) ?
              az_vsub(state->ship.position, baddie->position) : drift);
  baddie->angle =
    az_angle_towards(baddie->angle, turn_rate * time, goal_theta);
  const az_vector_t unit = az_vpolar(1, baddie->angle);
  const az_vector_t lateral = az_vflatten(baddie->velocity, unit);
  const az_vector_t dvel =
    az_vadd(az_vcaplen(az_vneg(lateral), lateral_decel_rate * time),
            (az_vdot(unit, drift) >= 0.0 ?
             az_vmul(unit, forward_accel * time) :
             az_vmul(unit, -backward_accel * time)));
  baddie->velocity = az_vcaplen(az_vadd(baddie->velocity, dvel), max_speed);
}

static bool perch_on_ceiling(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  const double turn_rate = 4.0;
  const double max_speed = 500.0;
  const double accel = 200.0;
  const double lateral_decel_rate = 250.0;
  // Pick a ceiling to perch on.
  const double baddie_norm = az_vnorm(baddie->position);
  const double baddie_theta = az_vtheta(baddie->position);
  az_vector_t goal = AZ_VZERO;
  double best_dist = INFINITY;
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    if (az_vnorm(wall->position) < baddie_norm) continue;
    const az_vector_t delta = az_vsub(wall->position, baddie->position);
    const double dist = az_vnorm(delta) +
      150.0 * fabs(az_mod2pi(az_vtheta(delta) - baddie_theta));
    if (dist < best_dist) {
      goal = wall->position;
      best_dist = dist;
    }
  }
  // Turn towards that wall.
  const double goal_theta = az_vtheta(az_vsub(goal, baddie->position));
  baddie->angle =
    az_angle_towards(baddie->angle, turn_rate * time, goal_theta);
  // If we kept going straight, what's the distance until we hit a wall?
  az_impact_t impact;
  az_circle_impact(state, baddie->data->main_body.bounding_radius,
                   baddie->position, az_vsub(goal, baddie->position),
                   (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  const double dist = az_vdist(baddie->position, impact.position);
  // If we're there, stop.
  if (dist <= 2.0) {
    baddie->velocity = AZ_VZERO;
    baddie->angle = az_vtheta(impact.normal);
    return true;
  }
  // Otherwise, steady ourselves sideways, and accel/decel to an appropriate
  // speed depending on how far we are from the perch.
  const az_vector_t unit = az_vpolar(1, baddie->angle);
  const az_vector_t lateral = az_vflatten(baddie->velocity, unit);
  const double forward_speed = az_vdot(baddie->velocity, unit);
  const double desired_speed = 0.8 * dist + 90.0;
  const az_vector_t dvel =
    az_vadd(az_vcaplen(az_vneg(lateral), lateral_decel_rate * time),
            (forward_speed < desired_speed ?
             az_vmul(unit, accel * time) :
             az_vmul(unit, -accel * time)));
  baddie->velocity = az_vcaplen(az_vadd(baddie->velocity, dvel), max_speed);
  return false;
}

static void crawl_around(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    bool rightwards, double turn_rate, double max_speed, double accel) {
  // Adjust velocity in crawling direction.
  const az_vector_t unit =
    az_vpolar(1.0, baddie->angle + AZ_DEG2RAD(rightwards ? -115 : 115));
  az_vpluseq(&baddie->velocity, az_vmul(unit, accel * time));
  const double drag_coeff = accel / (max_speed * max_speed);
  const az_vector_t drag_force =
    az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
  az_vpluseq(&baddie->velocity, az_vmul(drag_force, time));
  // Rotate to point away from wall.
  az_impact_t impact;
  az_circle_impact(
      state, baddie->data->main_body.bounding_radius, baddie->position,
      az_vmul(unit, 100000.0), (AZ_IMPF_BADDIE | AZ_IMPF_SHIP),
      baddie->uid, &impact);
  if (impact.type != AZ_IMP_NOTHING) {
    baddie->angle = az_angle_towards(baddie->angle, turn_rate * time,
                                     az_vtheta(impact.normal));
  }
}

static void snake_along(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    int first_component, double spacing, double speed, double wiggle,
    az_vector_t destination) {
  const double dest_theta = az_vtheta(az_vsub(destination, baddie->position));
  const az_vector_t goal =
    az_vadd(destination,
            az_vpolar(wiggle * sin(baddie->param), dest_theta + AZ_HALF_PI));
  const double new_angle = az_angle_towards(
      baddie->angle, AZ_PI * time,
      az_vtheta(az_vsub(goal, baddie->position)));
  const double dtheta = az_mod2pi(new_angle - baddie->angle);
  const az_vector_t reldelta = az_vpolar(speed * time, dtheta);
  az_vector_t last_pos = AZ_VZERO;
  for (int i = first_component; i < baddie->data->num_components; ++i) {
    az_component_t *component = &baddie->components[i];
    component->position = az_vrotate(component->position, -dtheta);
    component->position = az_vsub(component->position, reldelta);
    component->position =
      az_vadd(last_pos, az_vwithlen(az_vsub(component->position, last_pos),
                                    spacing));
    component->angle = az_vtheta(az_vsub(last_pos, component->position));
    last_pos = component->position;
  }
  baddie->position =
    az_vadd(baddie->position, az_vpolar(speed * time, new_angle));
  baddie->angle = new_angle;
  baddie->param = az_mod2pi(baddie->param + AZ_TWO_PI * time);
}

/*===========================================================================*/

static void tick_boss_door(az_space_state_t *state, az_baddie_t *baddie,
                           double time) {
  static const double max_angle = AZ_DEG2RAD(50);
  static const double beam_duration = 0.75;
  // Aim eye towards ship:
  if (baddie->state != 2) {
    const double turn_rate =
      (baddie->state == 3 ? AZ_DEG2RAD(40) : AZ_DEG2RAD(120));
    baddie->components[0].angle =
      fmax(-max_angle, fmin(max_angle, az_mod2pi(az_angle_towards(
        baddie->angle + baddie->components[0].angle, turn_rate * time,
        az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                                 baddie->angle)));
  }
  // States 0 and 1: Wait for an opportunity to fire at ship.
  if (baddie->state == 0 || baddie->state == 1) {
    const bool can_see_ship = az_ship_is_present(&state->ship) &&
      (fabs(az_mod2pi(az_vtheta(az_vsub(
           state->ship.position, baddie->position)) -
                      baddie->angle)) <= max_angle) &&
      has_line_of_sight_to_ship(state, baddie);
    const double eyelid_turn = AZ_DEG2RAD(360) * time;
    // State 0: Wait for ship to get in sight.
    if (baddie->state == 0) {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn, 0);
      baddie->param = fmax(0.0, baddie->param - 2.0 * time);
      if (can_see_ship &&
          az_vwithin(state->ship.position, baddie->position, 200.0)) {
        baddie->state = 1;
      }
    }
    // State 1: Open eyelids and charge up beam.
    else {
      baddie->components[1].angle =
        az_angle_towards(baddie->components[1].angle, eyelid_turn,
                         max_angle + AZ_DEG2RAD(10));
      if (baddie->cooldown <= 0.0) {
        baddie->param = fmin(1.0, baddie->param + time);
      }
      if (!(can_see_ship &&
            az_vwithin(state->ship.position, baddie->position, 300.0))) {
        baddie->state = 0;
      } else if (baddie->param >= 1.0 &&
                 baddie->components[1].angle >= max_angle) {
        baddie->state = 2;
        baddie->cooldown = 0.35;
        az_play_sound(&state->soundboard, AZ_SND_CHARGED_MISSILE_BEAM);
      }
    }
    baddie->components[2].angle = -AZ_HALF_PI - baddie->components[1].angle;
  }
  // State 2: Pause for a short time before firing.
  else if (baddie->state == 2) {
    if (baddie->cooldown <= 0.0) {
      baddie->state = 3;
      baddie->cooldown = beam_duration;
      az_play_sound(&state->soundboard, AZ_SND_FIRE_MISSILE_BEAM);
    }
  }
  // State 3: Fire beam until cooldown expires.
  else {
    baddie->param = baddie->cooldown / beam_duration;
    // Fire a beam.
    const double beam_damage = 25.0 * time / beam_duration;
    const double beam_angle = baddie->angle + baddie->components[0].angle;
    const az_vector_t beam_start =
      az_vadd(baddie->position, az_vpolar(15, beam_angle));
    az_impact_t impact;
    az_ray_impact(state, beam_start, az_vpolar(5000, beam_angle),
                  AZ_IMPF_NOTHING, baddie->uid, &impact);
    if (impact.type == AZ_IMP_BADDIE) {
      az_try_damage_baddie(state, impact.target.baddie.baddie,
          impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
    } else if (impact.type == AZ_IMP_SHIP) {
      az_damage_ship(state, beam_damage, false);
    }
    // Add particles for the beam.
    const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
    const az_color_t beam_color = {255, alt, 128, 192};
    az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                (6.0 + 0.5 * az_clock_zigzag(8, 1, state->clock)) *
                baddie->cooldown);
    az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                         impact.target.wall->data->color1 :
                         AZ_WHITE), 1.0, impact.position,
                 az_vpolar(az_random(20.0, 70.0),
                           az_vtheta(impact.normal) +
                           az_random(-AZ_HALF_PI, AZ_HALF_PI)));
    // When the cooldown timer reachers zero, stop firing the beam.
    if (baddie->cooldown <= 0.0) {
      baddie->state = 1;
      baddie->cooldown = 1.0;
    }
  }
}

static void tick_rockwyrm(az_space_state_t *state, az_baddie_t *baddie,
                          double time) {
  assert(baddie->kind == AZ_BAD_ROCKWYRM);
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  // Open/close jaws:
  if (baddie->cooldown < 0.5 && baddie->state != 1) {
    const double limit = AZ_DEG2RAD(45);
    const double delta = AZ_DEG2RAD(180) * time;
    baddie->components[0].angle =
      fmin(limit, baddie->components[0].angle + delta);
    baddie->components[1].angle =
      fmax(-limit, baddie->components[1].angle - delta);
  } else {
    const double delta = AZ_DEG2RAD(45) * time;
    baddie->components[0].angle =
      fmax(0, baddie->components[0].angle - delta);
    baddie->components[1].angle =
      fmin(0, baddie->components[1].angle + delta);
  }
  // State 0: Shoot a spread of bullets:
  if (baddie->state == 0) {
    if (baddie->cooldown <= 0.0 &&
        angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(20)) &&
        has_line_of_sight_to_ship(state, baddie)) {
      // Fire bullets.  The lower our health, the more bullets we fire.
      const int num_projectiles = 3 + 2 * (int)floor(4 * hurt);
      double theta = 0.0;
      for (int i = 0; i < num_projectiles; ++i) {
        fire_projectile(state, baddie, AZ_PROJ_STINGER,
                        baddie->data->main_body.bounding_radius, 0.0,
                        theta);
        theta = -theta;
        if (i % 2 == 0) theta += AZ_DEG2RAD(10);
      }
      // Below 35% health, we have a 20% chance to go to state 20 (firing a
      // long spray of bullets).
      if (hurt >= 0.65 && az_random(0, 1) < 0.2) {
        baddie->state = 20;
        baddie->cooldown = 3.0;
      }
      // Otherwise, we have a 50/50 chance to stay in state 0, or to go to
      // state 1 (drop eggs).
      else if (az_random(0, 1) < 0.5) {
        baddie->state = 1;
        baddie->cooldown = 1.0;
      } else baddie->cooldown = az_random(2.0, 4.0);
    }
  }
  // State 1: Drop eggs:
  else if (baddie->state == 1) {
    if (baddie->cooldown <= 0.0) {
      // The lower our health, the more eggs we drop at once.
      const int num_eggs = 1 + (int)floor(4 * hurt);
      const double spread = AZ_DEG2RAD(num_eggs * 10);
      const az_component_t *tail =
        &baddie->components[baddie->data->num_components - 1];
      for (int i = 0; i < num_eggs; ++i) {
        az_baddie_t *egg = az_add_baddie(
            state, AZ_BAD_WYRM_EGG,
            az_vadd(baddie->position,
                    az_vrotate(tail->position, baddie->angle)),
            baddie->angle + tail->angle + AZ_PI + az_random(-spread, spread));
        if (egg != NULL) {
          egg->velocity = az_vpolar(az_random(50, 150), egg->angle);
          // Set the egg to hatch (without waiting for the ship to get
          // close first) after a random amount of time.
          egg->state = 1;
          egg->cooldown = az_random(2.0, 2.5);
        } else break;
      }
      baddie->state = 0;
      baddie->cooldown = az_random(1.0, 3.0);
    }
  }
  // State 20: Wait until we have line of sight:
  else if (baddie->state == 20) {
    if (baddie->cooldown <= 0.0 &&
        angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(8)) &&
        has_line_of_sight_to_ship(state, baddie)) {
      --baddie->state;
    }
  }
  // States 2-19: Fire a steady spray of bullets:
  else if (2 <= baddie->state && baddie->state <= 19) {
    if (baddie->cooldown <= 0.0) {
      for (int i = -1; i <= 1; ++i) {
        fire_projectile(state, baddie, AZ_PROJ_STINGER,
                        baddie->data->main_body.bounding_radius, 0.0,
                        AZ_DEG2RAD(i * az_random(0, 10)));
      }
      baddie->cooldown = 0.1;
      --baddie->state;
    }
  } else baddie->state = 0;
  // Chase ship; get slightly slower as we get hurt.
  snake_along(state, baddie, time, 2, 40.0, 130.0 - 10.0 * hurt, 50.0,
              state->ship.position);
}

static void tick_forcefiend(az_space_state_t *state, az_baddie_t *baddie,
                            double time) {
  assert(baddie->kind == AZ_BAD_FORCEFIEND);
  const az_camera_bounds_t *bounds =
    &state->planet->rooms[state->ship.player.current_room].camera_bounds;
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  // Open/close jaws:
  const double jaw_angle =
    AZ_DEG2RAD(30) * (0.5 + 0.5 * sin(3.0 * state->ship.player.total_time));
  baddie->components[0].angle = az_angle_towards(
      baddie->components[0].angle, AZ_DEG2RAD(90) * time, jaw_angle);
  baddie->components[1].angle = az_angle_towards(
      baddie->components[1].angle, AZ_DEG2RAD(90) * time, -jaw_angle);
  // If the ship is destroyed or we run it over, move away from it.
  if (!az_ship_is_present(&state->ship) ||
      az_vwithin(baddie->position, state->ship.position, 30.0)) {
    if (az_mod2pi(az_vtheta(state->ship.position) -
                  (bounds->min_theta + 0.5 * bounds->theta_span)) < 0) {
      baddie->state = 1;
    } else baddie->state = 2;
  }
  // State 0: Chase ship and fire homing torpedoes.
  if (baddie->state == 0) {
    snake_along(state, baddie, time, 8, 30, 130 + 130 * hurt, 150,
                state->ship.position);
    if (baddie->cooldown <= 0.0 && has_line_of_sight_to_ship(state, baddie)) {
      if (az_random(0, 1) < 0.5) {
        fire_projectile(state, baddie, AZ_PROJ_TRINE_TORPEDO, 20, 0, 0);
        if (az_random(0, 1) < 0.5) {
          if (az_mod2pi(az_vtheta(state->ship.position) -
                        (bounds->min_theta + 0.5 * bounds->theta_span)) < 0) {
            baddie->state = 1;
          } else baddie->state = 2;
        }
      } else {
        fire_projectile(state, baddie, AZ_PROJ_GRAVITY_TORPEDO, 20, 0, 0);
      }
      baddie->cooldown = 2.0 - 0.8 * hurt;
    }
  }
  // States 1 and 2: Seek to left/right side of room.
  else if (baddie->state == 1 || baddie->state == 2) {
    const double r = bounds->min_r + 0.5 * bounds->r_span;
    const double dt = 150.0 / r;
    const az_vector_t dest = az_vpolar(r, bounds->min_theta +
        (baddie->state == 1 ? bounds->theta_span + dt : -dt));
    snake_along(state, baddie, time, 8, 30, 200 + 150 * hurt,
                150 + 150 * hurt, dest);
    if (az_ship_is_present(&state->ship) &&
        az_vwithin(baddie->position, dest, 100.0)) {
      baddie->state = 3;
      if (hurt > 0.5) {
        fire_projectile(state, baddie, AZ_PROJ_TRINE_TORPEDO, 20, 0, 0);
      }
    }
  }
  // State 3: Shoot a flurry of force waves.
  else if (baddie->state == 3) {
    baddie->angle = az_angle_towards(baddie->angle, AZ_DEG2RAD(180) * time,
        az_vtheta(az_vsub(state->ship.position, baddie->position)));
    if (baddie->cooldown <= 0.0 &&
        angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(60))) {
      fire_projectile(state, baddie, AZ_PROJ_FORCE_WAVE, 0, 0,
                      az_random(-1.3, 1.3));
      baddie->cooldown = 0.3;
      if (az_random(0, 1) < 0.05) baddie->state = 0;
    }
    if (!az_ship_is_present(&state->ship) ||
        az_vwithin(state->ship.position, baddie->position, 150.0)) {
      baddie->state = 0;
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

// How long it takes a baddie's armor flare to die down, in seconds:
#define AZ_BADDIE_ARMOR_FLARE_TIME 0.3
// How long it takes a baddie to unfreeze, in seconds.
#define AZ_BADDIE_THAW_TIME 8.0

static void tick_baddie(az_space_state_t *state, az_baddie_t *baddie,
                        double time) {
  // Allow the armor flare to die down a bit.
  assert(baddie->armor_flare >= 0.0);
  assert(baddie->armor_flare <= 1.0);
  baddie->armor_flare =
    fmax(0.0, baddie->armor_flare - time / AZ_BADDIE_ARMOR_FLARE_TIME);

  // Allow the baddie to thaw a bit.  If we're in a superheated room, thaw
  // faster than usual.
  assert(baddie->frozen >= 0.0);
  assert(baddie->frozen <= 1.0);
  const double thaw_rate =
    (state->planet->rooms[state->ship.player.current_room].properties &
     AZ_ROOMF_HEATED) ? 3.0 / AZ_BADDIE_THAW_TIME : 1.0 / AZ_BADDIE_THAW_TIME;
  baddie->frozen = fmax(0.0, baddie->frozen - thaw_rate * time);
  if (baddie->frozen > 0.0) {
    baddie->velocity = AZ_VZERO;
    return;
  }

  // Cool down the baddie's weapon.
  baddie->cooldown = fmax(0.0, baddie->cooldown - time);

  // Apply velocity.
  const az_vector_t old_baddie_position = baddie->position;
  const double old_baddie_angle = baddie->angle;
  if (az_vnonzero(baddie->velocity)) {
    az_impact_t impact;
    az_circle_impact(state, baddie->data->main_body.bounding_radius,
                     baddie->position, az_vmul(baddie->velocity, time),
                     (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
    baddie->position = impact.position;
    if (impact.type != AZ_IMP_NOTHING) {
      // Baddies with the BOUNCE_PERP flag always bounce perfectly backwards
      // (as if they hit the wall dead-on), even if they hit a wall at an
      // oblique angle.
      if (baddie->data->properties & AZ_BADF_BOUNCE_PERP) {
        impact.normal = az_vproj(impact.normal, baddie->velocity);
      }
      // Push the baddie slightly away from the impact point (so that we're
      // hopefully no longer in contact with the object we hit).
      az_vpluseq(&baddie->position, az_vwithlen(impact.normal, 0.5));
      // Bounce the baddie off the object.
      baddie->velocity =
        az_vsub(baddie->velocity,
                az_vmul(az_vproj(baddie->velocity, impact.normal), 1.5));
    }
  }

  // Perform kind-specific logic.
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_MARKER: break; // Do nothing.
    case AZ_BAD_TURRET:
    case AZ_BAD_ARMORED_TURRET:
      // Aim gun:
      baddie->components[0].angle =
        fmax(-1.0, fmin(1.0, az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, 2.0 * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                       baddie->angle)));
      // Fire:
      if (baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, baddie->components[0].angle,
                               AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        fire_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                        baddie->components[0].angle, 0.0);
        baddie->cooldown = 1.5;
        az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
      }
      break;
    case AZ_BAD_ZIPPER:
    case AZ_BAD_ARMORED_ZIPPER:
      if (az_vdot(baddie->velocity, az_vpolar(1, baddie->angle)) < 0.0) {
        baddie->angle = az_mod2pi(baddie->angle + AZ_PI);
      }
      baddie->velocity = az_vpolar(200.0, baddie->angle);
      break;
    case AZ_BAD_BOUNCER:
      if (az_vnonzero(baddie->velocity)) {
        baddie->angle = az_vtheta(baddie->velocity);
      }
      baddie->velocity = az_vpolar(150.0, baddie->angle);
      break;
    case AZ_BAD_ATOM:
      drift_towards_ship(state, baddie, time, 70, 100, 100);
      for (int i = 0; i < baddie->data->num_components; ++i) {
        az_component_t *component = &baddie->components[i];
        component->angle = az_mod2pi(component->angle + 3.5 * time);
        component->position = az_vpolar(4.0, component->angle);
        component->position.x *= 5.0;
        component->position =
          az_vrotate(component->position, i * AZ_DEG2RAD(120));
      }
      break;
    case AZ_BAD_SPINER:
      drift_towards_ship(state, baddie, time, 40, 10, 100);
      baddie->angle = az_mod2pi(baddie->angle + 0.4 * time);
      if (baddie->cooldown <= 0.0 &&
          az_vwithin(baddie->position, state->ship.position, 200)) {
        for (int i = 0; i < 360; i += 45) {
          fire_projectile(state, baddie, AZ_PROJ_SPINE,
                          baddie->data->main_body.bounding_radius,
                          AZ_DEG2RAD(i), 0.0);
        }
        baddie->cooldown = 2.0;
      }
      break;
    case AZ_BAD_BOX:
    case AZ_BAD_ARMORED_BOX:
      // Do nothing.
      break;
    case AZ_BAD_CLAM:
      {
        const double ship_theta =
          az_vtheta(az_vsub(state->ship.position, baddie->position));
        baddie->angle =
          az_angle_towards(baddie->angle, 0.2 * time, ship_theta);
        const double max_angle = AZ_DEG2RAD(40);
        const double old_angle = baddie->components[0].angle;
        const double new_angle =
          (baddie->cooldown <= 0.0 &&
           fabs(az_mod2pi(baddie->angle - ship_theta)) < AZ_DEG2RAD(10) &&
           has_line_of_sight_to_ship(state, baddie) ?
           fmin(max_angle, max_angle -
                (max_angle - old_angle) * pow(0.00003, time)) :
           fmax(0.0, old_angle - 1.0 * time));
        baddie->components[0].angle = new_angle;
        baddie->components[1].angle = -new_angle;
        if (baddie->cooldown <= 0.0 && new_angle >= 0.95 * max_angle) {
          for (int i = -1; i <= 1; ++i) {
            fire_projectile(state, baddie,
                (i == 0 ? AZ_PROJ_FIREBALL_SLOW : AZ_PROJ_FIREBALL_FAST),
                0.5 * baddie->data->main_body.bounding_radius,
                AZ_DEG2RAD(i * 5), 0.0);
          }
          baddie->cooldown = 3.0;
        }
      }
      break;
    case AZ_BAD_NIGHTBUG:
      fly_towards_ship(state, baddie, time,
                       3.0, 40.0, 100.0, 20.0, 100.0, 100.0);
      if (baddie->state == 0) {
        baddie->param = fmax(0.0, baddie->param - time / 3.5);
        if (baddie->cooldown < 0.5 &&
            az_vwithin(baddie->position, state->ship.position, 250.0) &&
            angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(3)) &&
            has_line_of_sight_to_ship(state, baddie)) {
          baddie->state = 1;
        }
      } else if (baddie->state == 1) {
        baddie->param = fmin(1.0, baddie->param + time / 0.5);
        if (baddie->cooldown <= 0.0 && baddie->param == 1.0) {
          for (int i = 0; i < 2; ++i) {
            fire_projectile(state, baddie, AZ_PROJ_FIREBALL_FAST, 20.0, 0.0,
                            az_random(-AZ_DEG2RAD(10), AZ_DEG2RAD(10)));
          }
          baddie->cooldown = 5.0;
          baddie->state = 0;
        }
      } else baddie->state = 0;
      break;
    case AZ_BAD_SPINE_MINE:
      drift_towards_ship(state, baddie, time, 20, 20, 20);
      baddie->angle = az_mod2pi(baddie->angle - 0.5 * time);
      if (az_ship_is_present(&state->ship) &&
          az_vwithin(baddie->position, state->ship.position, 150.0) &&
          has_line_of_sight_to_ship(state, baddie)) {
        for (int i = 0; i < 360; i += 20) {
          fire_projectile(state, baddie, AZ_PROJ_SPINE,
              baddie->data->main_body.bounding_radius,
              AZ_DEG2RAD(i) + az_random(AZ_DEG2RAD(-10), AZ_DEG2RAD(10)), 0.0);
        }
        baddie->kind = AZ_BAD_NOTHING;
      }
      break;
    case AZ_BAD_BROKEN_TURRET:
      if (baddie->state == 2) {
        baddie->components[0].angle -= 5.0 * time;
        if (baddie->components[0].angle <= -1.0) {
          baddie->components[0].angle = -1.0;
          baddie->state = 0;
        }
      } else if (baddie->state == 1) {
        baddie->components[0].angle += 5.0 * time;
        if (baddie->components[0].angle >= 1.0) {
          baddie->components[0].angle = 1.0;
          baddie->state = 0;
        }
      } else if (baddie->state == 0) {
        // Try to aim gun (but sometimes twitch randomly):
        const int aim = az_randint(-1, 1);
        baddie->components[0].angle = fmax(-1.0, fmin(1.0, az_mod2pi(
            (aim == 0 ?
             az_angle_towards(
                baddie->angle + baddie->components[0].angle, 2 * time,
                az_vtheta(az_vsub(state->ship.position, baddie->position))) -
               baddie->angle :
             baddie->components[0].angle + aim * 2 * time))));
        // Fire:
        if (baddie->cooldown <= 0.0 &&
            angle_to_ship_within(state, baddie, baddie->components[0].angle,
                                 AZ_DEG2RAD(6)) &&
            has_line_of_sight_to_ship(state, baddie)) {
          fire_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                          baddie->components[0].angle, 0.0);
          az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
          baddie->cooldown = 1.0;
        }
        // Randomly go crazy:
        if (az_random(0.0, 2.5) < time) {
          baddie->state = az_randint(1, 2);
          const az_vector_t spark_start =
            az_vadd(baddie->position,
                    az_vpolar(20, baddie->angle +
                              baddie->components[0].angle));
          const double spark_angle =
            baddie->angle + baddie->components[0].angle +
            (baddie->state == 1 ? -AZ_DEG2RAD(65) : AZ_DEG2RAD(65));
          for (int i = 0; i < 8; ++i) {
            const double theta =
              spark_angle + az_random(-AZ_DEG2RAD(25), AZ_DEG2RAD(25));
            az_add_speck(state, (az_color_t){255, 200, 100, 255}, 4.0,
                         spark_start, az_vpolar(az_random(10, 70), theta));
          }
        }
      } else baddie->state = 0;
      break;
    case AZ_BAD_ZENITH_CORE:
      // Lie dormant for 31 seconds.
      if (baddie->state == 0) {
        baddie->state = 1;
        baddie->cooldown = 31.0;
      }
      if (baddie->cooldown <= 0.0) {
        // Turn towards the ship.
        baddie->angle = az_angle_towards(
            baddie->angle, AZ_DEG2RAD(30) * time,
            az_vtheta(az_vsub(state->ship.position, baddie->position)));
        // Fire a beam, piercing through the ship and other baddies.
        const az_vector_t beam_start =
          az_vadd(baddie->position, az_vpolar(120, baddie->angle));
        az_impact_t impact;
        az_ray_impact(state, beam_start, az_vpolar(1000, baddie->angle),
                      (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
        const az_vector_t beam_delta = az_vsub(impact.position, beam_start);
        const double beam_damage = 300.0 * time;
        // Damage the ship and any baddies within the beam.
        if (az_ship_is_present(&state->ship) &&
            az_ray_hits_ship(&state->ship, beam_start, beam_delta,
                             NULL, NULL)) {
          az_damage_ship(state, beam_damage, false);
          az_loop_sound(&state->soundboard, AZ_SND_BEAM_NORMAL);
          az_loop_sound(&state->soundboard, AZ_SND_BEAM_PHASE);
        }
        AZ_ARRAY_LOOP(other, state->baddies) {
          if (other->kind == AZ_BAD_NOTHING || other == baddie) continue;
          const az_component_data_t *component;
          if (az_ray_hits_baddie(other, beam_start, beam_delta,
                                 NULL, NULL, &component)) {
            az_try_damage_baddie(state, other, component,
                                 (AZ_DMGF_PIERCE | AZ_DMGF_BEAM), beam_damage);
          }
        }
        // Add particles for the beam.
        const az_color_t beam_color = {
          (az_clock_mod(6, 1, state->clock) < 3 ? 255 : 64),
          (az_clock_mod(6, 1, state->clock + 2) < 3 ? 255 : 64),
          (az_clock_mod(6, 1, state->clock + 4) < 3 ? 255 : 64), 192};
        az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                    6 + az_clock_zigzag(6, 1, state->clock));
        for (int i = 0; i < 5; ++i) {
          az_add_speck(state, beam_color, 1.0, impact.position,
                       az_vpolar(az_random(20.0, 70.0),
                                 az_vtheta(impact.normal) +
                                 az_random(-AZ_HALF_PI, AZ_HALF_PI)));
        }
      }
      break;
    case AZ_BAD_DRAGONFLY:
      fly_towards_ship(state, baddie, time,
                       5.0, 300.0, 300.0, 200.0, 0.0, 100.0);
      if (az_ship_is_present(&state->ship) &&
          state->ship.temp_invincibility > 0.0) {
        baddie->cooldown = 2.0;
      }
      break;
    case AZ_BAD_CRAWLER:
      crawl_around(state, baddie, time, true, 3.0, 40.0, 100.0);
      break;
    case AZ_BAD_CRAWLING_TURRET:
      crawl_around(state, baddie, time, az_ship_is_present(&state->ship) &&
                   az_vcross(az_vsub(state->ship.position, baddie->position),
                             az_vpolar(1.0, baddie->angle)) > 0.0,
                   1.0, 20.0, 100.0);
      // Aim gun:
      baddie->components[0].angle =
        fmax(AZ_DEG2RAD(-85), fmin(AZ_DEG2RAD(85), az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, AZ_DEG2RAD(100) * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                       baddie->angle)));
      // Fire:
      if (baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, baddie->components[0].angle,
                               AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        fire_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 20.0,
                        baddie->components[0].angle, 0.0);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
        baddie->cooldown = 1.5;
      }
      break;
    case AZ_BAD_STINGER:
      // Fire (when the ship is behind us):
      if (baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, AZ_PI, AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        fire_projectile(state, baddie, AZ_PROJ_STINGER, 15.0, AZ_PI, 0.0);
        baddie->cooldown = 0.5;
      }
      // Chase ship if state 0, flee in state 1:
      fly_towards_ship(state, baddie, time, 8.0, 200.0, 300.0, 200.0,
                       0.0, (baddie->state == 0 ? 100.0 : -100.0));
      // Switch from state 0 to state 1 if we're close to the ship; switch from
      // state 1 to state 0 if we're far from the ship.
      if (baddie->state == 0) {
        if (az_ship_is_present(&state->ship) &&
            az_vwithin(state->ship.position, baddie->position, 200.0)) {
          baddie->state = 1;
        }
      } else if (baddie->state == 1) {
        if (!az_ship_is_present(&state->ship) ||
            !az_vwithin(state->ship.position, baddie->position, 400.0)) {
          baddie->state = 0;
        }
      } else baddie->state = 0;
      break;
    case AZ_BAD_BEAM_SENSOR:
    case AZ_BAD_GUN_SENSOR:
      if (baddie->state == 0) {
        if (baddie->health < baddie->data->max_health) {
          baddie->state = 1;
          az_run_script(state, baddie->on_kill);
        }
      }
      baddie->health = baddie->data->max_health;
      break;
    case AZ_BAD_ROCKWYRM:
      tick_rockwyrm(state, baddie, time);
      break;
    case AZ_BAD_WYRM_EGG:
      // Apply drag to the egg (if it's moving).
      if (az_vnonzero(baddie->velocity)) {
        const double drag_coeff = 1.0 / 400.0;
        const az_vector_t drag_force =
          az_vmul(baddie->velocity, -drag_coeff * az_vnorm(baddie->velocity));
        baddie->velocity =
          az_vadd(baddie->velocity, az_vmul(drag_force, time));
      }
      // State 0: Wait for ship to draw near.
      if (baddie->state == 0) {
        if (az_ship_is_present(&state->ship) &&
            az_vwithin(baddie->position, state->ship.position, 200.0) &&
            has_line_of_sight_to_ship(state, baddie)) {
          baddie->state = 1;
          baddie->cooldown = 2.0;
        }
      }
      // State 1: Hatch as soon as cooldown reaches zero.
      else {
        if (baddie->cooldown <= 0.0) {
          const az_vector_t position = baddie->position;
          const double angle = baddie->angle;
          az_kill_baddie(state, baddie);
          az_init_baddie(baddie, AZ_BAD_WYRMLING, position, angle);
        }
      }
      break;
    case AZ_BAD_WYRMLING:
      snake_along(state, baddie, time, 0, 5.0, 180.0, 120.0,
                  state->ship.position);
      break;
    case AZ_BAD_TRAPDOOR:
      if (!(az_ship_is_present(&state->ship) &&
            az_vwithin(state->ship.position, baddie->position,
                       baddie->data->overall_bounding_radius +
                       AZ_SHIP_DEFLECTOR_RADIUS))) {
        baddie->components[0].angle =
          az_angle_towards(baddie->components[0].angle, AZ_DEG2RAD(600) * time,
                           (az_ship_is_present(&state->ship) &&
                            az_vwithin(state->ship.position, baddie->position,
                                       210.0) ? AZ_DEG2RAD(90) : 0));
      }
      break;
    case AZ_BAD_SWOOPER:
      // State 0: Perch on the nearest wall, then go to state 1.
      if (baddie->state == 0) {
        if (perch_on_ceiling(state, baddie, time)) {
          baddie->cooldown = 2.0;
          baddie->state = 1;
        }
      }
      // State 1: Sit and wait until the ship is nearby, then go to state 2.
      else if (baddie->state == 1) {
        if (baddie->cooldown <= 0.0 && az_ship_is_present(&state->ship) &&
            az_vwithin(state->ship.position, baddie->position, 250.0) &&
            has_line_of_sight_to_ship(state, baddie)) {
          baddie->param = 3.5;
          baddie->state = 2;
        }
      }
      // State 2: Chase the ship for up to a few seconds, then go to state 0.
      else if (baddie->state == 2) {
        if (az_ship_is_present(&state->ship)) {
          fly_towards_ship(state, baddie, time,
                           5.0, 500.0, 300.0, 250.0, 0.0, 100.0);
          baddie->param = fmax(0.0, baddie->param - time);
        } else baddie->param = 0.0;
        if (baddie->param <= 0.0) baddie->state = 0;
      } else baddie->state = 0;
      break;
    case AZ_BAD_ICE_CRAWLER:
      crawl_around(state, baddie, time, false, 3.0, 30.0, 100.0);
      break;
    case AZ_BAD_BEAM_TURRET:
      // Aim gun:
      baddie->components[0].angle =
        fmax(-1.0, fmin(1.0, az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, AZ_DEG2RAD(30) * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                       baddie->angle)));
      // If we can see the ship, turn on the beam:
      if (angle_to_ship_within(state, baddie, baddie->components[0].angle,
                               AZ_DEG2RAD(20)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        baddie->cooldown = 0.5;
      }
      // If beam is still turned on, fire:
      if (baddie->cooldown > 0.0) {
        // Fire a beam.
        const double beam_damage = 60.0 * time;
        const double beam_theta = baddie->angle + baddie->components[0].angle;
        const az_vector_t beam_start =
          az_vadd(baddie->position, az_vpolar(30, beam_theta));
        az_impact_t impact;
        az_ray_impact(state, beam_start, az_vpolar(1000, beam_theta),
                      AZ_IMPF_NOTHING, baddie->uid, &impact);
        if (impact.type == AZ_IMP_BADDIE) {
          az_try_damage_baddie(state, impact.target.baddie.baddie,
              impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
        } else if (impact.type == AZ_IMP_SHIP) {
          az_damage_ship(state, beam_damage, false);
        }
        // Add particles for the beam.
        const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
        const az_color_t beam_color = {alt/2, 255, alt, 192};
        az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                    2.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
        az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                             impact.target.wall->data->color1 :
                             AZ_WHITE), 1.0, impact.position,
                     az_vpolar(az_random(20.0, 70.0),
                               az_vtheta(impact.normal) +
                               az_random(-AZ_HALF_PI, AZ_HALF_PI)));
        az_loop_sound(&state->soundboard, AZ_SND_BEAM_NORMAL);
      }
      break;
    case AZ_BAD_OTH_CRAB:
      fly_towards_ship(state, baddie, time,
                       2.0, 40.0, 100.0, 20.0, 100.0, 100.0);
      if (baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        if (fire_projectile(state, baddie, AZ_PROJ_OTH_ROCKET,
                            15.0, 0.0, 0.0) != NULL) {
          az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
          baddie->cooldown = 2.0;
        }
      }
      break;
    case AZ_BAD_OTH_ORB:
      drift_towards_ship(state, baddie, time, 80, 20, 100);
      if (baddie->cooldown <= 0.0 && az_ship_is_present(&state->ship) &&
          az_vwithin(baddie->position, state->ship.position, 500) &&
          has_line_of_sight_to_ship(state, baddie)) {
        fire_projectile(state, baddie, AZ_PROJ_OTH_HOMING,
                        baddie->data->main_body.bounding_radius,
                        az_random(-AZ_PI, AZ_PI), 0.0);
        baddie->cooldown = 0.1;
      }
      break;
    case AZ_BAD_OTH_SNAPDRAGON:
      fly_towards_ship(state, baddie, time,
                       5.0, 300.0, 300.0, 200.0, 0.0, 100.0);
      if (baddie->cooldown <= 0.0) {
        switch (baddie->state) {
          case 0:
          case 1:
          case 2:
          case 4:
          case 6:
            if (angle_to_ship_within(state, baddie, 0, AZ_DEG2RAD(6))) {
              az_projectile_t *proj = fire_projectile(
                  state, baddie, AZ_PROJ_OTH_ROCKET, 30.0, 0.0, 0.0);
              if (proj != NULL) {
                proj->power = 0.7;
                az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_ROCKET);
                baddie->cooldown = 2.0;
                ++baddie->state;
              }
            }
            break;
          case 3:
          case 7:
            for (int i = 0; i < 360; i += 15) {
              fire_projectile(state, baddie, AZ_PROJ_OTH_SPRAY,
                              baddie->data->main_body.bounding_radius,
                              AZ_DEG2RAD(i), 0.0);
            }
            az_play_sound(&state->soundboard, AZ_SND_FIRE_OTH_SPRAY);
            baddie->cooldown = 2.0;
            ++baddie->state;
            break;
          case 5:
            for (int i = -1; i <= 1; ++i) {
              az_baddie_t *razor = az_add_baddie(
                  state, AZ_BAD_OTH_RAZOR, baddie->position,
                  baddie->angle + AZ_PI + i * AZ_DEG2RAD(45));
              if (razor != NULL) {
                razor->velocity =
                  az_vpolar(az_random(300, 500), razor->angle);
              } else break;
            }
            az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
            baddie->cooldown = az_random(2.0, 4.0);
            ++baddie->state;
            break;
          case 8:
            for (int i = 0; i < 4; ++i) {
              az_baddie_t *razor = az_add_baddie(
                  state, AZ_BAD_OTH_RAZOR, baddie->position,
                  baddie->angle + AZ_DEG2RAD(45) + i * AZ_DEG2RAD(90));
              if (razor != NULL) {
                razor->state = 1;
                razor->velocity = az_vpolar(300, razor->angle);
              } else break;
            }
            az_play_sound(&state->soundboard, AZ_SND_LAUNCH_OTH_RAZORS);
            baddie->cooldown = az_random(2.0, 4.0);
            baddie->state = 0;
            break;
        }
      }
      break;
    case AZ_BAD_OTH_RAZOR:
      if (baddie->state == 0) {
        drift_towards_ship(state, baddie, time, 400, 500, 100);
        baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(180) * time);
      } else {
        baddie->velocity = az_vwithlen(baddie->velocity, 300.0);
        baddie->angle = az_mod2pi(baddie->angle - AZ_DEG2RAD(180) * time);
      }
      break;
    case AZ_BAD_SECURITY_DRONE:
      // Chase ship:
      drift_towards_ship(state, baddie, time, 300,
                         (az_vwithin(state->ship.position, baddie->position,
                                     200) ? -300 : 300), 100);
      // Orient body:
      baddie->angle = az_vtheta(baddie->position);
      // Aim gun:
      baddie->components[0].angle = az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, AZ_DEG2RAD(120) * time,
          az_vtheta(az_vsub(state->ship.position,
                            baddie->position))) - baddie->angle);
      // State 0: cooling off for next salvo:
      if (baddie->state == 0 && baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, baddie->components[0].angle,
                               AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        baddie->state = 10;
      }
      // State N: firing salvo, N shots left until next cooldown.
      if (baddie->state > 0 && baddie->cooldown <= 0.0) {
        const double offset =
          (baddie->state % 2 ? AZ_DEG2RAD(-12) : AZ_DEG2RAD(12));
        fire_projectile(state, baddie, AZ_PROJ_LASER_PULSE, 28.6,
                        baddie->components[0].angle - offset, offset);
        az_play_sound(&state->soundboard, AZ_SND_FIRE_LASER_PULSE);
        --baddie->state;
        baddie->cooldown = (baddie->state > 0 ? 0.1 : 1.5);
      }
      break;
    case AZ_BAD_SMALL_TRUCK:
      // States 0 and 1: fly forward.
      if (baddie->state == 0 || baddie->state == 1) {
        const double max_speed = 100.0, accel = 50.0;
        const double min_dist = 100.0, max_dist = 200.0;
        const double dist =
          dist_until_hit_wall(state, baddie, max_dist + 1.0, 0.0);
        const double fraction =
          fmin(1.0, fmax(0.0, dist - min_dist) / (max_dist - min_dist));
        const double speed_limit = max_speed * sqrt(fraction);
        const double speed =
          fmin(speed_limit, az_vnorm(baddie->velocity) + accel * time);
        baddie->state = (speed >= max_speed || speed < speed_limit ? 1 : 0);
        baddie->velocity = az_vpolar(speed, baddie->angle);
        if (fraction <= 0.0) {
          baddie->param = az_mod2pi(baddie->angle + AZ_HALF_PI);
          baddie->state = 2;
        }
      }
      // State 2: Turn.
      else if (baddie->state == 2) {
        baddie->velocity = AZ_VZERO;
        const double turn_rate = AZ_DEG2RAD(50);
        const double goal_angle = baddie->param;
        baddie->angle =
          az_angle_towards(baddie->angle, turn_rate * time, goal_angle);
        if (baddie->angle == goal_angle) baddie->state = 0;
      } else baddie->state = 0;
      break;
    case AZ_BAD_HEAT_RAY:
      // State 0: Fire beam until cooldown expires.
      if (baddie->state == 0) {
        if (baddie->cooldown > 0.0) {
          // Fire a beam.
          const double beam_damage = 38.0 * time;
          const az_vector_t beam_start =
            az_vadd(baddie->position, az_vpolar(15, baddie->angle));
          az_impact_t impact;
          az_ray_impact(state, beam_start, az_vpolar(5000, baddie->angle),
                        AZ_IMPF_NOTHING, baddie->uid, &impact);
          if (impact.type == AZ_IMP_BADDIE) {
            az_try_damage_baddie(state, impact.target.baddie.baddie,
                impact.target.baddie.component, AZ_DMGF_BEAM, beam_damage);
          } else if (impact.type == AZ_IMP_SHIP) {
            az_damage_ship(state, beam_damage, false);
          }
          // Add particles for the beam.
          const uint8_t alt = 32 * az_clock_zigzag(6, 1, state->clock);
          const az_color_t beam_color = {255, 128, alt, 192};
          az_add_beam(state, beam_color, beam_start, impact.position, 0.0,
                      4.0 + 0.5 * az_clock_zigzag(8, 1, state->clock));
          az_add_speck(state, (impact.type == AZ_IMP_WALL ?
                               impact.target.wall->data->color1 :
                               AZ_WHITE), 1.0, impact.position,
                       az_vpolar(az_random(20.0, 70.0),
                                 az_vtheta(impact.normal) +
                                 az_random(-AZ_HALF_PI, AZ_HALF_PI)));
          az_loop_sound(&state->soundboard, AZ_SND_BEAM_FREEZE);
        } else {
          baddie->state = 1;
          baddie->cooldown = az_random(0.5, 3.0);
        }
      }
      // State 1: Recharge until cooldown expires.
      else {
        if (baddie->cooldown <= 0.0) {
          baddie->state = 0;
          baddie->cooldown = 1.5;
        }
      }
      break;
    case AZ_BAD_NUCLEAR_MINE:
      baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(90) * time);
      // State 0: Wait for ship.
      if (baddie->state == 0) {
        if (az_ship_is_present(&state->ship) &&
            az_vwithin(baddie->position, state->ship.position, 150.0) &&
            has_line_of_sight_to_ship(state, baddie)) {
          baddie->state = 1;
          baddie->cooldown = 0.75;
          az_play_sound(&state->soundboard, AZ_SND_BLINK_MEGA_BOMB);
        }
      }
      // State 1: Explode when cooldown reaches zero.
      else {
        if (baddie->cooldown <= 0.0) {
          fire_projectile(state, baddie, AZ_PROJ_NUCLEAR_EXPLOSION,
                          0.0, 0.0, 0.0);
          az_kill_baddie(state, baddie);
        }
      }
      break;
    case AZ_BAD_BEAM_WALL: break; // Do nothing.
    case AZ_BAD_SPARK:
      if (az_random(0, 1) < 10.0 * time) {
        az_add_speck(
            state, (az_color_t){0, 255, 0, 255}, 1.0, baddie->position,
            az_vpolar(az_random(20.0, 70.0), baddie->angle +
                      az_random(AZ_DEG2RAD(-120), AZ_DEG2RAD(120))));
      }
      if (az_random(0, 1) < time) {
        const double angle = az_random(AZ_DEG2RAD(-135), AZ_DEG2RAD(135));
        fire_projectile(state, baddie, AZ_PROJ_SPARK, 0.0, 0.0, angle);
        for (int i = 0; i < 5; ++i) {
          az_add_speck(
              state, (az_color_t){0, 255, 0, 255}, 1.0, baddie->position,
              az_vpolar(az_random(20.0, 70.0), baddie->angle + angle +
                        az_random(AZ_DEG2RAD(-60), AZ_DEG2RAD(60))));
        }
      }
      break;
    case AZ_BAD_FLYER:
      {
        baddie->angle = az_vtheta(baddie->velocity);
        const double speed = 200.0;
        const double turn = AZ_DEG2RAD(120) * time;
        bool chasing_ship = false;
        if (az_ship_is_present(&state->ship) &&
            az_vwithin(baddie->position, state->ship.position, 200.0)) {
          const double goal_theta =
            az_vtheta(az_vsub(state->ship.position, baddie->position));
          if (fabs(az_mod2pi(goal_theta - baddie->angle)) <= AZ_DEG2RAD(45) &&
              has_line_of_sight_to_ship(state, baddie)) {
            baddie->angle = az_angle_towards(baddie->angle, turn, goal_theta);
            chasing_ship = true;
          }
        }
        if (!chasing_ship) {
          bool avoiding = false;
          AZ_ARRAY_LOOP(other, state->baddies) {
            if (other == baddie) continue;
            if (other->kind != AZ_BAD_FLYER) continue;
            if (az_vwithin(baddie->position, other->position,
                           2.0 * baddie->data->overall_bounding_radius)) {
              const double rel = az_mod2pi(az_vtheta(az_vsub(
                  other->position, baddie->position)) - baddie->angle);
              baddie->angle =
                az_mod2pi(baddie->angle + (rel < 0 ? turn : -turn));
              avoiding = true;
              break;
            }
          }
          if (!avoiding) {
            const double left = dist_until_hit_wall(
                state, baddie, 2000, AZ_DEG2RAD(40));
            const double right = dist_until_hit_wall(
                state, baddie, 2000, AZ_DEG2RAD(-40));
            baddie->angle =
              az_mod2pi(baddie->angle + (left > right ? turn : -turn));
          }
        }
        baddie->velocity = az_vpolar(speed, baddie->angle);
      }
      break;
    case AZ_BAD_FORCEFIEND:
      tick_forcefiend(state, baddie, time);
      break;
    case AZ_BAD_STALK_PLANT:
      {
        az_component_t *base = &baddie->components[0];
        // Get the absolute position of the base.
        const az_vector_t base_pos =
          az_vadd(baddie->position, az_vrotate(base->position, baddie->angle));
        const double base_angle = az_mod2pi(baddie->angle + base->angle);
        // Pick a new position for the head.
        const bool line_of_sight = has_line_of_sight_to_ship(state, baddie);
        const az_vector_t ship_pos = state->ship.position;
        const double head_angle = az_angle_towards(baddie->angle,
            (line_of_sight ? AZ_DEG2RAD(120) : AZ_DEG2RAD(60)) * time,
            (!line_of_sight ? base_angle :
             az_vtheta(az_vsub(ship_pos, baddie->position))));
        az_vector_t head_pos = baddie->position;
        // State 0: Move head in a circle.
        if (baddie->state == 0) {
          baddie->param += time * 400 / az_vdist(head_pos, ship_pos);
          const az_vector_t goal =
            az_vadd(az_vadd(base_pos, az_vpolar(140, base_angle)),
                    az_vpolar(40, baddie->param));
          const double tracking_base = 0.1; // smaller = faster tracking
          const az_vector_t delta =
            az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
          az_vpluseq(&head_pos, delta);
          if (baddie->cooldown <= 0.0 && line_of_sight &&
              az_vwithin(ship_pos, head_pos, 150)) {
            baddie->cooldown = az_random(0.35, 0.8);
            baddie->state = 1;
          }
        }
        // State 1: Rear back, then lunge at ship.
        else {
          if (!az_vwithin(head_pos, base_pos, 180) ||
              (line_of_sight && az_vwithin(head_pos, ship_pos, 20))) {
            baddie->cooldown = 1.5;
            baddie->state = 0;
          } else if (baddie->cooldown > 0) {
            const az_vector_t goal =
              az_vadd(base_pos, az_vpolar(50, base_angle));
            const double tracking_base = 0.001; // smaller = faster tracking
            const az_vector_t delta =
              az_vmul(az_vsub(goal, head_pos), 1.0 - pow(tracking_base, time));
            az_vpluseq(&head_pos, delta);
          } else {
            az_vpluseq(&head_pos, az_vpolar(600 * time, head_angle));
          }
        }
        // The baddie is centered on the head, so move the baddie to where we
        // want the head to be.
        baddie->position = head_pos;
        baddie->angle = head_angle;
        // Now that the head has moved, move the base component so that it
        // stays in the same absolute position as before.
        base->position = az_vrotate(az_vsub(base_pos, baddie->position),
                                    -baddie->angle);
        base->angle = az_mod2pi(base_angle - baddie->angle);
        // Arrange the stalk segments along a cubic bezier curve from the head
        // to the base.
        const az_vector_t ctrl1 = az_vsub(head_pos, az_vpolar(60, head_angle));
        const az_vector_t ctrl2 = az_vadd(base_pos, az_vpolar(50, base_angle));
        for (int i = 0; i < 9; ++i) {
          az_component_t *segment = &baddie->components[i + 3];
          const double t = (0.5 + i) / 10.0;
          const double s = 1.0 - t;
          segment->position = az_vrotate(az_vsub(
              az_vadd(az_vadd(az_vmul(head_pos, s*s*s),
                              az_vmul(ctrl1, 3*s*s*t)),
                      az_vadd(az_vmul(ctrl2, 3*s*t*t),
                              az_vmul(base_pos, t*t*t))),
              baddie->position), -baddie->angle);
          segment->angle = az_mod2pi(az_vtheta(az_vadd(
              az_vadd(az_vmul(head_pos, -3*s*s),
                      az_vmul(ctrl1, 3*s*s - 6*s*t)),
              az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
                      az_vmul(base_pos, 3*t*t)))) - baddie->angle);
        }
        // Open/close the jaws.
        const double max_angle = AZ_DEG2RAD(40);
        const double old_angle = baddie->components[1].angle;
        const double new_angle =
          (baddie->state == 0 ||
           (baddie->state == 1 && line_of_sight &&
            az_vwithin(head_pos, ship_pos, 50)) ?
           fmax(0.0, old_angle - 4.0 * time) :
           fmin(max_angle, old_angle + 2.0 * time));
        baddie->components[1].angle = new_angle;
        baddie->components[2].angle = -new_angle;
      }
      break;
    case AZ_BAD_COPTER:
      if (baddie->cooldown <= 0.0) {
        const double max_speed = 150.0, accel = 50.0;
        const double min_dist = 100.0, max_dist = 200.0;
        const double rel_angle =
          (baddie->state == 0 ? AZ_HALF_PI : -AZ_HALF_PI);
        const double dist =
          dist_until_hit_wall(state, baddie, max_dist + 1.0, rel_angle);
        const double fraction =
          fmin(1.0, fmax(0.0, dist - min_dist) / (max_dist - min_dist));
        const double speed_limit = max_speed * sqrt(fraction);
        const double speed =
          fmin(speed_limit, az_vnorm(baddie->velocity) + accel * time);
        baddie->velocity = az_vpolar(speed, baddie->angle + rel_angle);
        if (fraction <= 0.0) {
          baddie->velocity = AZ_VZERO;
          baddie->state = (baddie->state == 0 ? 1 : 0);
          baddie->cooldown = 1.0;
        }
      }
      break;
    case AZ_BAD_URCHIN:
      drift_towards_ship(state, baddie, time, 250, 300, 500);
      baddie->angle = az_mod2pi(baddie->angle + AZ_DEG2RAD(50) * time);
      break;
    case AZ_BAD_BOSS_DOOR:
      tick_boss_door(state, baddie, time);
      break;
    case AZ_BAD_ROCKET_TURRET:
      // Aim gun:
      baddie->components[0].angle =
        fmax(-1.0, fmin(1.0, az_mod2pi(az_angle_towards(
          baddie->angle + baddie->components[0].angle, 2.0 * time,
          az_vtheta(az_vsub(state->ship.position, baddie->position))) -
                                       baddie->angle)));
      // Fire:
      if (baddie->cooldown <= 0.0 &&
          angle_to_ship_within(state, baddie, baddie->components[0].angle,
                               AZ_DEG2RAD(6)) &&
          has_line_of_sight_to_ship(state, baddie)) {
        fire_projectile(state, baddie, AZ_PROJ_HYPER_ROCKET, 20.0,
                        baddie->components[0].angle, 0.0);
        baddie->cooldown = 1.5;
        az_play_sound(&state->soundboard, AZ_SND_FIRE_HYPER_ROCKET);
      }
      break;
  }

  // Move cargo with the baddie (unless the baddie killed itself).
  if (baddie->kind != AZ_BAD_NOTHING) {
    az_move_baddie_cargo(
        state, baddie,
        az_vsub(baddie->position, old_baddie_position),
        az_mod2pi(baddie->angle - old_baddie_angle));
  }
}

void az_tick_baddies(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    assert(baddie->health > 0.0);
    tick_baddie(state, baddie, time);
  }
}

/*===========================================================================*/
