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

#include "azimuth/tick/door.h"

#include "azimuth/state/door.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h" // for az_dmax and az_dmin

/*===========================================================================*/

// How long a door takes to open/close, in seconds:
#define DOOR_OPEN_TIME 0.5

static void tick_door(az_space_state_t *state, az_door_t *door, double time) {
  const double delta = (1.0 / DOOR_OPEN_TIME) * time;
  if (door->is_open) {
    door->openness = az_dmin(1.0, door->openness + delta);
  } else {
    door->openness = az_dmax(0.0, door->openness - delta);
  }
}

void az_tick_doors(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    tick_door(state, door, time);
  }
}

/*===========================================================================*/
