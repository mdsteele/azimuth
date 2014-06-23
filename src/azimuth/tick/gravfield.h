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
#ifndef AZIMUTH_TICK_GRAVFIELD_H_
#define AZIMUTH_TICK_GRAVFIELD_H_

#include <stdbool.h>

#include "azimuth/state/space.h"

/*===========================================================================*/

void az_tick_gravfields(az_space_state_t *state, double time);

// Apply gravfield forces to the ship.  Additionally, set *ship_is_in_water to
// whether or not the ship is currently within at least one water gravfield,
// and set *ship_is_in_lava to whether or not the ship is currently within at
// least one lava gravfield.
void az_apply_gravfields_to_ship(az_space_state_t *state, double time,
                                 bool *ship_is_in_water,
                                 bool *ship_is_in_lava);

/*===========================================================================*/

#endif // AZIMUTH_TICK_GRAVFIELD_H_
