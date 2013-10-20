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

#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/
// Detecting ship:

// Return true if the ship is present and within the given range of the baddie;
// false otherwise.
bool az_ship_in_range(
    const az_space_state_t *state, const az_baddie_t *baddie, double range);

// Return true if the ship is present and if the vector from the baddie to the
// ship is within the given angle of the baddie's forward direction plus the
// offset; false otherwise.
bool az_ship_within_angle(
    const az_space_state_t *state, const az_baddie_t *baddie,
    double offset, double angle);

// Return true if the ship is present and if the baddie has a clear
// line-of-sight to the ship (blocked by walls/doors, but not by other
// baddies); false otherwise.
bool az_can_see_ship(az_space_state_t *state, const az_baddie_t *baddie);

/*===========================================================================*/
// Navigation:

// Crawl along the wall.  The `rightwards` argument controls the direction of
// crawling, while the other arguments control the speed.
void az_crawl_around(
    az_space_state_t *state, az_baddie_t *baddie, double time,
    bool rightwards, double turn_rate, double max_speed, double accel);

// Move in a wiggling, snake-like motion towards the given destination.
// Components from first_tail_component on up will trail behind.
void az_snake_towards(
    az_baddie_t *baddie, double time, int first_tail_component,
    double speed, double wiggle, az_vector_t destination);

/*===========================================================================*/
// Weapons:

az_projectile_t *az_fire_baddie_projectile(
    az_space_state_t *state, az_baddie_t *baddie, az_proj_kind_t kind,
    double forward, double firing_angle, double proj_angle_offset);

/*===========================================================================*/

#endif // AZIMUTH_TICK_BADDIE_UTIL_H_
