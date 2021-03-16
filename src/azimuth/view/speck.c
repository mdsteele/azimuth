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

#include "azimuth/view/particle.h"

#include <assert.h>

#include <SDL_opengl.h>

#include "azimuth/state/particle.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

void az_draw_specks(const az_space_state_t *state) {
  glBegin(GL_LINES); {
    AZ_ARRAY_LOOP(speck, state->specks) {
      if (speck->kind == AZ_SPECK_NOTHING) continue;
      assert(speck->age >= 0.0);
      assert(speck->age <= speck->lifetime);
      glColor4ub(speck->color.r, speck->color.g, speck->color.b,
                 speck->color.a * (1.0 - speck->age / speck->lifetime));
      az_gl_vertex(speck->position);
      az_gl_vertex(az_vsub(speck->position, az_vunit(speck->velocity)));
    }
  } glEnd();
}

/*===========================================================================*/
