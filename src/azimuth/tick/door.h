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
#ifndef AZIMUTH_TICK_DOOR_H_
#define AZIMUTH_TICK_DOOR_H_

#include "azimuth/state/space.h"

/*===========================================================================*/

// If the given kind of damage is capable of opening the given door, open the
// door, running the door's script if necessary and also playing a sound
// effect.  Otherwise, do nothing.
void az_try_open_door(az_space_state_t *state, az_door_t *door,
                      az_damage_flags_t damage_kind);

void az_tick_doors(az_space_state_t *state, double time);

/*===========================================================================*/

#endif // AZIMUTH_TICK_DOOR_H_
