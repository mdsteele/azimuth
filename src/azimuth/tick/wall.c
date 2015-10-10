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

#include "azimuth/tick/wall.h"

#include <assert.h>
#include <math.h>

#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

// How long it takes a wall's flare to die down, in seconds:
#define AZ_WALL_FLARE_TIME 0.4

void az_tick_walls(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    if (wall->kind == AZ_WALL_INDESTRUCTIBLE) {
      assert(wall->flare == 0.0);
    } else {
      assert(wall->flare >= 0.0);
      assert(wall->flare <= 1.0);
      wall->flare = fmax(0.0, wall->flare - time / AZ_WALL_FLARE_TIME);
    }
  }
}

/*===========================================================================*/
