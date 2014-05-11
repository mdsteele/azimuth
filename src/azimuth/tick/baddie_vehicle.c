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

#include "azimuth/tick/baddie_vehicle.h"

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

static void tick_copter(az_space_state_t *state, az_baddie_t *baddie,
                        double time, double min_dist, double rel_angle) {
  if (baddie->cooldown <= 0.0) {
    const double max_speed = 150.0, accel = 50.0, max_dist = 200.0;
    if (baddie->state != 0) rel_angle += AZ_PI;
    const double dist =
      az_baddie_dist_to_wall(state, baddie, max_dist + 1.0, rel_angle);
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
}

void az_tick_bad_copter_horz(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_COPTER_HORZ);
  tick_copter(state, baddie, time, 100.0, AZ_HALF_PI);
}

void az_tick_bad_copter_vert(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_COPTER_VERT);
  tick_copter(state, baddie, time, 50.0, 0);
}

/*===========================================================================*/

static void tick_truck(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_speed, double accel, double min_dist, double max_dist,
    double turn_rate, double turn_angle) {
  // States 0 and 1: fly forward.
  if (baddie->state == 0 || baddie->state == 1) {
    const double dist =
      az_baddie_dist_to_wall(state, baddie, max_dist + 1.0, 0.0);
    const double fraction =
      fmin(1.0, fmax(0.0, dist - min_dist) / (max_dist - min_dist));
    const double speed_limit = max_speed * sqrt(fraction);
    const double speed =
      fmin(speed_limit, az_vnorm(baddie->velocity) + accel * time);
    baddie->state = (speed >= max_speed || speed < speed_limit ? 1 : 0);
    baddie->velocity = az_vpolar(speed, baddie->angle);
    if (fraction <= 0.0) {
      baddie->param = az_mod2pi(baddie->angle + turn_angle);
      baddie->state = 2;
    }
  }
  // State 2: Turn.
  else if (baddie->state == 2) {
    baddie->velocity = AZ_VZERO;
    const double goal_angle = baddie->param;
    baddie->angle =
      az_angle_towards(baddie->angle, turn_rate * time, goal_angle);
    if (baddie->angle == goal_angle) baddie->state = 0;
  } else baddie->state = 0;
}

void az_tick_bad_small_truck(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SMALL_TRUCK);
  tick_truck(state, baddie, time, 100.0, 50.0, 100.0, 200.0,
             AZ_DEG2RAD(50), AZ_DEG2RAD(90));
}

void az_tick_bad_small_auv(
    az_space_state_t *state, az_baddie_t *baddie, double time) {
  assert(baddie->kind == AZ_BAD_SMALL_AUV);
  tick_truck(state, baddie, time, 70.0, 30.0, 50.0, 100.0,
             AZ_DEG2RAD(30), AZ_DEG2RAD(-60));
}

/*===========================================================================*/
