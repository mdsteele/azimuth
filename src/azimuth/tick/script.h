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
#ifndef AZIMUTH_TICK_SCRIPT_H_
#define AZIMUTH_TICK_SCRIPT_H_

#include "azimuth/state/script.h"
#include "azimuth/state/space.h"

/*===========================================================================*/

// Execute the script and modify the space state accordingly.  As a
// convenience, the script argument is permitted to be NULL, in which case this
// function simply returns immediately with no effect.
void az_run_script(az_space_state_t *state, const az_script_t *script);

void az_tick_timers(az_space_state_t *state, double time);

/*===========================================================================*/

#endif // AZIMUTH_TICK_SCRIPT_H_
