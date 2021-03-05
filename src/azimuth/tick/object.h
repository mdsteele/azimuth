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
#ifndef AZIMUTH_TICK_OBJECT_H_
#define AZIMUTH_TICK_OBJECT_H_

#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/object.h"
#include "azimuth/state/space.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/
// Movement:

// Move an object relative to its current location, propagating changes to
// other objects as necessary (e.g. moving a baddie moves its cargo too, moving
// a tractor node may pull the ship along, etc).
void az_move_object(az_space_state_t *state, az_object_t *object,
                    az_vector_t delta_position, double delta_angle);

void az_move_baddie_cargo(az_space_state_t *state, az_baddie_t *baddie,
                          az_vector_t delta_position, double delta_angle);

/*===========================================================================*/
// Damage/destruction:

// Reduce the baddie's health by the given amount, if the baddie is susceptible
// to the given kind of damage.  If the the given damage kind includes
// AZ_DMGF_FREEZE, this may freeze the baddie.  Return true iff any damage was
// dealt.  If this damage is enough to destroy the baddie, remove it, add
// particles/pickups in its place, and run its script.
bool az_try_damage_baddie(az_space_state_t *state, az_baddie_t *baddie,
                          const az_component_data_t *component,
                          az_damage_flags_t damage_kind, double damage_amount);

// Kill the given baddie outright, without leaving behind any pickups or
// running any scripts.
void az_kill_baddie(az_space_state_t *state, az_baddie_t *baddie);

// Reduce the ship's shields by the given amount.  If this is enough to destroy
// the ship, change to game-over mode.
void az_damage_ship(az_space_state_t *state, double damage,
                    bool induce_temp_invincibility);

// Destroy the ship unconditionally, changing to game-over mode.
void az_kill_ship(az_space_state_t *state);

// Try to destroy the wall with the given kind of damage, and return true iff
// the wall was destroyed.
bool az_try_break_wall(az_space_state_t *state, az_wall_t *wall,
                       az_damage_flags_t damage_kind,
                       az_vector_t impact_point);

// Destroy the wall unconditionally.
void az_break_wall(az_space_state_t *state, az_wall_t *wall);

// Kill/break/destroy the object, without running scripts or leaving pickups.
void az_kill_object(az_space_state_t *state, az_object_t *object);

/*===========================================================================*/

#endif // AZIMUTH_TICK_OBJECT_H_
