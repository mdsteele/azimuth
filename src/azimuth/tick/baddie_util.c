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

#include "azimuth/tick/baddie_util.h"

#include <math.h>
#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_ship_in_range(
    const az_space_state_t *state, const az_baddie_t *baddie, double range) {
  return (az_ship_is_decloaked(&state->ship) &&
          az_vwithin(baddie->position, state->ship.position, range));
}

bool az_ship_within_angle(
    const az_space_state_t *state, const az_baddie_t *baddie,
    double offset, double angle) {
  return (az_ship_is_decloaked(&state->ship) &&
          fabs(az_mod2pi(baddie->angle + offset -
                         az_vtheta(az_vsub(state->ship.position,
                                           baddie->position)))) <= angle);
}

bool az_can_see_ship(az_space_state_t *state, const az_baddie_t *baddie) {
  if (!az_ship_is_decloaked(&state->ship)) return false;
  az_impact_t impact;
  az_ray_impact(state, baddie->position,
                az_vsub(state->ship.position, baddie->position),
                AZ_IMPF_BADDIE, baddie->uid, &impact);
  return (impact.type == AZ_IMP_SHIP);
}

/*===========================================================================*/

double az_baddie_dist_to_wall(
    az_space_state_t *state, const az_baddie_t *baddie,
    double max_dist, double relative_angle) {
  az_impact_t impact;
  az_circle_impact(
      state, baddie->data->overall_bounding_radius, baddie->position,
      az_vpolar(max_dist, baddie->angle + relative_angle),
      (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  return az_vdist(baddie->position, impact.position);
}


bool az_baddie_has_clear_path_to_position(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t position) {
  az_impact_t impact;
  az_circle_impact(state, baddie->data->main_body.bounding_radius,
                   baddie->position, az_vsub(position, baddie->position),
                   (AZ_IMPF_BADDIE | AZ_IMPF_SHIP), baddie->uid, &impact);
  return (impact.type == AZ_IMP_NOTHING);
}

/*===========================================================================*/

void az_crawl_around(
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

void az_trail_tail_behind(az_baddie_t *baddie, int first_tail_component,
                          az_vector_t old_position, double old_angle) {
  const double dtheta = az_mod2pi(baddie->angle - old_angle);
  const az_vector_t reldelta =
    az_vrotate(az_vsub(baddie->position, old_position), -old_angle);
  az_vector_t prev_new_pos = AZ_VZERO;
  double prev_new_angle = 0.0;
  az_vector_t prev_init_pos = AZ_VZERO;
  double prev_radius = baddie->data->main_body.bounding_radius;
  for (int i = first_tail_component; i < baddie->data->num_components; ++i) {
    // First, adjust the component so it stays in the same absolute position.
    az_component_t *component = &baddie->components[i];
    component->position = az_vrotate(component->position, -dtheta);
    component->position = az_vsub(component->position, reldelta);
    // Calculate the staple point between this component and the previous one.
    const az_component_data_t *data = &baddie->data->components[i];
    const double init_dist = az_vdist(data->init_position, prev_init_pos);
    const double staple_dist_to_prev =
      0.5 * (prev_radius + init_dist - data->bounding_radius);
    const az_vector_t staple_pos =
      az_vadd(prev_new_pos, az_vpolar(-staple_dist_to_prev, prev_new_angle));
    const double staple_dist_to_this = init_dist - staple_dist_to_prev;
    // Yank this component to the staple point.
    prev_new_pos = component->position =
      az_vadd(staple_pos, az_vwithlen(az_vsub(component->position, staple_pos),
                                      staple_dist_to_this));
    prev_new_angle = component->angle =
      az_vtheta(az_vsub(staple_pos, component->position));
    prev_init_pos = data->init_position;
    prev_radius = data->bounding_radius;
  }
}

void az_snake_towards(
    az_baddie_t *baddie, double time, int first_tail_component,
    double speed, double wiggle, az_vector_t destination) {
  const az_vector_t old_position = baddie->position;
  const double old_angle = baddie->angle;
  const double dest_theta = az_vtheta(az_vsub(destination, baddie->position));
  const az_vector_t goal =
    az_vadd(destination,
            az_vpolar(wiggle * sin(baddie->param), dest_theta + AZ_HALF_PI));
  const double new_angle = az_angle_towards(
      baddie->angle, AZ_PI * time,
      az_vtheta(az_vsub(goal, baddie->position)));
  baddie->position =
    az_vadd(baddie->position, az_vpolar(speed * time, new_angle));
  baddie->angle = new_angle;
  az_trail_tail_behind(baddie, first_tail_component, old_position, old_angle);
  baddie->param = az_mod2pi(baddie->param + AZ_TWO_PI * time);
}

static void apply_walls_to_force_field(
    az_space_state_t *state, az_baddie_t *baddie,
    double wall_far_coeff, double wall_near_coeff, az_vector_t *drift) {
  const az_vector_t pos = baddie->position;
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    if (door->kind == AZ_DOOR_FORCEFIELD && door->openness >= 1.0) continue;
    const az_vector_t delta = az_vsub(pos, door->position);
    const double dist = az_vnorm(delta) - AZ_DOOR_BOUNDING_RADIUS -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      az_vpluseq(drift, az_vwithlen(delta, wall_near_coeff));
    } else {
      az_vpluseq(drift, az_vwithlen(delta, wall_far_coeff * exp(-dist)));
    }
  }
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    const az_vector_t delta = az_vsub(pos, wall->position);
    const double dist = az_vnorm(delta) - wall->data->bounding_radius -
      baddie->data->overall_bounding_radius;
    if (dist <= 0.0) {
      az_vpluseq(drift, az_vwithlen(delta, wall_near_coeff));
    } else {
      az_vpluseq(drift, az_vwithlen(delta, wall_far_coeff * exp(-dist)));
    }
  }
}

static az_vector_t force_field_to_ship(
    az_space_state_t *state, az_baddie_t *baddie,
    double ship_coeff, double ship_min_range, double ship_max_range,
    double wall_far_coeff, double wall_near_coeff) {
  az_vector_t drift = AZ_VZERO;
  if (az_ship_in_range(state, baddie, ship_max_range)) {
    drift = az_vwithlen(az_vsub(state->ship.position, baddie->position),
                        (az_ship_in_range(state, baddie, ship_min_range) ?
                         -ship_coeff : ship_coeff));
  }
  apply_walls_to_force_field(state, baddie, wall_far_coeff,
                             wall_near_coeff, &drift);
  return drift;
}

static az_vector_t force_field_to_position(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t goal,
    double goal_coeff, double wall_far_coeff, double wall_near_coeff) {
  az_vector_t drift = az_vwithlen(az_vsub(goal, baddie->position), goal_coeff);
  apply_walls_to_force_field(state, baddie, wall_far_coeff,
                             wall_near_coeff, &drift);
  return drift;
}

void az_drift_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_speed, double ship_force, double wall_force) {
  az_vector_t drift = force_field_to_ship(
      state, baddie, ship_force, 0.0, 1000.0, wall_force, 2.0 * wall_force);
  AZ_ARRAY_LOOP(other, state->baddies) {
    if (other->kind == AZ_BAD_NOTHING) continue;
    if (other == baddie) continue;
    if (az_baddie_has_flag(other, AZ_BADF_INCORPOREAL)) continue;
    if (az_circle_touches_baddie(other, baddie->data->overall_bounding_radius,
                                 baddie->position, NULL)) {
      const az_vector_t delta = az_vsub(baddie->position, other->position);
      az_vpluseq(&drift, az_vwithlen(delta, 2.5 * wall_force));
    }
  }
  baddie->velocity =
    az_vcaplen(az_vadd(baddie->velocity, az_vmul(drift, time)), max_speed);
}

static void fly_common(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double turn_rate, double max_speed, double forward_accel,
    double lateral_decel_rate, az_vector_t drift, double goal_theta) {
  const double backward_accel = 80.0;
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


void az_fly_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double turn_rate, double max_speed, double forward_accel,
    double lateral_decel_rate, double attack_range, double ship_coeff) {
  const az_vector_t drift =
    (baddie->cooldown > 1.0 ?
     force_field_to_ship(state, baddie, -100.0, 0.0, 500.0, 100.0, 200.0) :
     force_field_to_ship(state, baddie, ship_coeff, attack_range, 1000.0,
                         100.0, 200.0));
  const double goal_theta =
    az_vtheta(baddie->cooldown <= 0.0 && az_ship_in_range(state, baddie, 120) ?
              az_vsub(state->ship.position, baddie->position) : drift);
  fly_common(state, baddie, time, turn_rate, max_speed, forward_accel,
             lateral_decel_rate, drift, goal_theta);
}

void az_fly_towards_position(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    az_vector_t goal, double turn_rate, double max_speed,
    double forward_accel, double lateral_decel_rate, double goal_coeff) {
  const az_vector_t drift =
    force_field_to_position(state, baddie, goal, goal_coeff, 100.0, 200.0);
  const double goal_theta = az_vtheta(drift);
  fly_common(state, baddie, time, turn_rate, max_speed, forward_accel,
             lateral_decel_rate, drift, goal_theta);
}

/*===========================================================================*/

az_projectile_t *az_fire_baddie_projectile(
    az_space_state_t *state, az_baddie_t *baddie, az_proj_kind_t kind,
    double forward, double firing_angle, double proj_angle_offset) {
  const double theta = firing_angle + baddie->angle;
  return az_add_projectile(
      state, kind, az_vadd(baddie->position, az_vpolar(forward, theta)),
      theta + proj_angle_offset, 1.0, baddie->uid);
}

/*===========================================================================*/
