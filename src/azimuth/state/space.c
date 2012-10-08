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

#include "azimuth/state/space.h"

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_insert_baddie(az_space_state_t *state,
                      az_baddie_t **baddie_out) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) {
      *baddie_out = baddie;
      return true;
    }
  }
  return false;
}

bool az_insert_particle(az_space_state_t *state,
                        az_particle_t **particle_out) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) {
      particle->age = 0.0;
      *particle_out = particle;
      return true;
    }
  }
  return false;
}

bool az_insert_projectile(az_space_state_t *state,
                          az_projectile_t **proj_out) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) {
      proj->age = 0.0;
      *proj_out = proj;
      return true;
    }
  }
  return false;
}

void az_try_add_pickup(az_space_state_t *state, az_pickup_kind_t kind,
                       az_vector_t position) {
  if (kind == AZ_PUP_NOTHING) return;
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) {
      pickup->kind = kind;
      pickup->position = position;
      pickup->age = 0.0;
      return;
    }
  }
}

/*===========================================================================*/
