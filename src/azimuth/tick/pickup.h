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
#ifndef AZIMUTH_TICK_PICKUP_H_
#define AZIMUTH_TICK_PICKUP_H_

#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_pickups(az_space_state_t *state, double time);

// Add a random pickup (maybe) at the given position; the pickup is selected
// randomly based on potential_pickups, which should be one or more AZ_PUPF_*
// flags bitwise-or'd together.  If the AZ_PUPF_NOTHING flag is included, then
// there's a chance that no pickup will be dropped.  If potential_pickups is
// zero (no flags), then no pickup is placed, same as if it were just
// AZ_PUPF_NOTHING.
void az_add_random_pickup(az_space_state_t *state,
                          unsigned int potential_pickups,
                          az_vector_t position);

/*===========================================================================*/

#endif // AZIMUTH_TICK_PICKUP_H_
