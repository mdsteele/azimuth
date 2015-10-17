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

#pragma once
#ifndef AZIMUTH_TICK_BADDIE_UTIL_H_
#define AZIMUTH_TICK_BADDIE_UTIL_H_

#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/
// Detecting ship:

// Return true if the ship is decloaked and within the given range of the
// baddie; false otherwise.
bool az_ship_in_range(
    const az_space_state_t *state, const az_baddie_t *baddie, double range);

// Return true if the ship is decloaked and if the vector from the baddie to
// the ship is within the given angle of the baddie's forward direction plus
// the offset; false otherwise.
bool az_ship_within_angle(
    const az_space_state_t *state, const az_baddie_t *baddie,
    double offset, double angle);

// Return true if the ship is decloaked and if the baddie has a clear
// line-of-sight to the ship (blocked by walls/doors, but not by other
// baddies); false otherwise.
bool az_can_see_ship(az_space_state_t *state, const az_baddie_t *baddie);

/*===========================================================================*/
// Detecting walls:

// Return the distance a baddie could go before hitting a wall or door if it
// moved in a straight line (up to max_dist) at relative_angle from its current
// position/angle.
double az_baddie_dist_to_wall(
    az_space_state_t *state, const az_baddie_t *baddie,
    double max_dist, double relative_angle);

// Return true if the baddie could move in a straight line from its current
// location to the given position without hitting any walls or doors.
bool az_baddie_has_clear_path_to_position(
    az_space_state_t *state, az_baddie_t *baddie, az_vector_t position);

/*===========================================================================*/
// Navigation:

// Crawl along the wall.  The `rightwards` argument controls the direction of
// crawling, while the other arguments control the speed.
void az_crawl_around(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    bool rightwards, double turn_rate, double max_speed, double accel);

void az_trail_tail_behind(az_baddie_t *baddie, int first_tail_component,
                          az_vector_t old_position, double old_angle);

// Move in a wiggling, snake-like motion towards the given destination.
// Components from first_tail_component on up will trail behind.
void az_snake_towards(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    int first_tail_component, double speed, double wiggle,
    az_vector_t destination, bool ignore_walls);

void az_drift_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double max_speed, double ship_force, double wall_force);

void az_drift_towards_position(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    az_vector_t goal, double max_speed, double goal_force, double wall_force);

void az_fly_towards_ship(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    double turn_rate, double max_speed, double forward_accel,
    double lateral_decel_rate, double attack_range, double ship_coeff);

void az_fly_towards_position(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    az_vector_t goal, double turn_rate, double max_speed,
    double forward_accel, double lateral_decel_rate, double goal_coeff);

/*===========================================================================*/
// Weapons:

az_projectile_t *az_fire_baddie_projectile(
    az_space_state_t *state, az_baddie_t *baddie, az_proj_kind_t kind,
    double forward, double firing_angle, double proj_angle_offset);

/*===========================================================================*/

#endif // AZIMUTH_TICK_BADDIE_UTIL_H_
