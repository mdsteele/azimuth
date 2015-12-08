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

#include "azimuth/state/cutscene.h"

#include "azimuth/state/particle.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_cutscene_add_particle(
    az_cutscene_state_t *cutscene, bool foreground, az_particle_kind_t kind,
    az_color_t color, az_vector_t position, az_vector_t velocity, double angle,
    double lifetime, double param1, double param2) {
  az_particle_t *particle = NULL;
  if (foreground) {
    AZ_ARRAY_LOOP(par, cutscene->fg_particles) {
      if (par->kind != AZ_PAR_NOTHING) continue;
      particle = par;
      break;
    }
  } else {
    AZ_ARRAY_LOOP(par, cutscene->bg_particles) {
      if (par->kind != AZ_PAR_NOTHING) continue;
      particle = par;
      break;
    }
  }
  if (particle != NULL) {
    particle->kind = kind;
    particle->color = color;
    particle->position = position;
    particle->velocity = velocity;
    particle->angle = angle;
    particle->age = 0.0;
    particle->lifetime = lifetime;
    particle->param1 = param1;
    particle->param2 = param2;
  }
}

/*===========================================================================*/
