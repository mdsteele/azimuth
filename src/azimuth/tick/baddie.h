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
#ifndef AZIMUTH_TICK_BADDIE_H_
#define AZIMUTH_TICK_BADDIE_H_

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_baddies(az_space_state_t *state, double time);

// Called after a baddie has been killed and removed, but before its script is
// run.  Not called if the baddie dies in a way that wouldn't run its script.
void az_on_baddie_killed(az_space_state_t *state, az_baddie_kind_t kind,
                         az_vector_t position, double angle);

/*===========================================================================*/

#endif // AZIMUTH_TICK_BADDIE_H_
