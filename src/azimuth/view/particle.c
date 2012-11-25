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
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/particle.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void with_color_alpha(az_color_t color, double alpha_factor) {
  glColor4ub(color.r, color.g, color.b, color.a * alpha_factor);
}

static void draw_particle(const az_particle_t *particle) {
  assert(particle->age <= particle->lifetime);
  switch (particle->kind) {
    case AZ_PAR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_PAR_SPECK:
      glBegin(GL_POINTS); {
        const double ratio = particle->age / particle->lifetime;
        with_color_alpha(particle->color, 1 - ratio);
        glVertex2d(0, 0);
      } glEnd();
      break;
    case AZ_PAR_SEGMENT:
      // TODO
      break;
    case AZ_PAR_BOOM:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 0);
        glVertex2d(0, 0);
        const double ratio = particle->age / particle->lifetime;
        with_color_alpha(particle->color, 1 - ratio * ratio);
        const double radius = particle->param1 * ratio;
        for (int i = 0; i <= 16; ++i) {
          glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                     radius * sin(i * AZ_PI_EIGHTHS));
        }
      } glEnd();
      break;
    case AZ_PAR_BEAM:
      glBegin(GL_QUAD_STRIP); {
        with_color_alpha(particle->color, 0);
        glVertex2d(0, particle->param2);
        glVertex2d(particle->param1, particle->param2);
        with_color_alpha(particle->color, 1);
        glVertex2d(0, 0);
        glVertex2d(particle->param1, 0);
        with_color_alpha(particle->color, 0);
        glVertex2d(0, -particle->param2);
        glVertex2d(particle->param1, -particle->param2);
      } glEnd();
      break;
  }
}

void az_draw_particles(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(particle->position.x, particle->position.y, 0);
      glRotated(AZ_RAD2DEG(particle->angle), 0, 0, 1);
      draw_particle(particle);
    } glPopMatrix();
  }
}

/*===========================================================================*/
