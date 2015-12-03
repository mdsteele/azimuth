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

#include "azimuth/state/victory.h"

#include "azimuth/state/baddie.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/projectile.h"
#include "azimuth/util/audio.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

void az_victory_add_baddie(az_victory_state_t *state, az_baddie_kind_t kind,
                           az_vector_t position, double angle) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind != AZ_BAD_NOTHING) continue;
    az_init_baddie(baddie, kind, position, angle);
    break;
  }
}

void az_victory_add_particle(
    az_victory_state_t *state, az_particle_kind_t kind, az_color_t color,
    az_vector_t position, az_vector_t velocity, double angle, double lifetime,
    double param1, double param2) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind != AZ_PAR_NOTHING) continue;
    particle->kind = kind;
    particle->color = color;
    particle->position = position;
    particle->velocity = velocity;
    particle->angle = angle;
    particle->age = 0.0;
    particle->lifetime = lifetime;
    particle->param1 = param1;
    particle->param2 = param2;
    break;
  }
}

void az_victory_add_projectile(az_victory_state_t *state, az_proj_kind_t kind,
                               az_vector_t position, double angle) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind != AZ_PROJ_NOTHING) continue;
    az_init_projectile(proj, kind, position, angle, 1.0, AZ_NULL_UID);
    break;
  }
}

void az_victory_clear_objects(az_victory_state_t *state) {
  AZ_ARRAY_LOOP(baddie, state->baddies) baddie->kind = AZ_BAD_NOTHING;
  AZ_ARRAY_LOOP(particle, state->particles) particle->kind = AZ_PAR_NOTHING;
  AZ_ARRAY_LOOP(proj, state->projectiles) proj->kind = AZ_PROJ_NOTHING;
}

/*===========================================================================*/
