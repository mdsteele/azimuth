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

#include "azimuth/tick/particle.h"

#include "azimuth/state/particle.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_tick_particles(az_space_state_t *state, double time) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) continue;
    particle->age += time;
    if (particle->age > particle->lifetime) {
      particle->kind = AZ_PAR_NOTHING;
    }
  }
}

/*===========================================================================*/
