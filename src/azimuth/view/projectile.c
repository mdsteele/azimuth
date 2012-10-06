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

#include "azimuth/view/projectile.h"

#include <OpenGL/gl.h>

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_projectile(const az_projectile_t* proj) {
  switch (proj->kind) {
    default: // TODO: add other types
    case AZ_PROJ_GUN_NORMAL:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.75); // white
        glVertex2d( 0.0,  0.0);
        glVertex2d( 2.0,  0.0);
        glVertex2d( 1.5,  1.5);
        glVertex2d( 0.0,  2.0);
        glVertex2d(-1.5,  1.5);
        glColor4f(1, 1, 1, 0); // transparent white
        glVertex2d(-10.,  0.0);
        glColor3f(1, 1, 1); // white
        glVertex2d(-1.5, -1.5);
        glVertex2d( 0.0, -2.0);
        glVertex2d( 1.5, -1.5);
        glVertex2d( 2.0,  0.0);
      } glEnd();
      break;
    case AZ_PROJ_GUN_PIERCE:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0, 1, 0.75); // magenta
        glVertex2d(2, 0);
        glColor4f(1, 0, 1, 0); // transparent magenta
        glVertex2d(0, 4);
        glVertex2d(-50, 0);
        glVertex2d(0, -4);
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glVertex2d(-2, 0);
        glColor4f(1, 0, 1, 0.75); // magenta
        glVertex2d(-6, 8);
        glVertex2d(2, 0);
        glVertex2d(-6, -8);
      } glEnd();
      break;
  }
}

void az_draw_projectiles(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(proj->position.x, proj->position.y, 0);
      glRotated(AZ_RAD2DEG(az_vtheta(proj->velocity)), 0, 0, 1);
      draw_projectile(proj);
    } glPopMatrix();
  }
}

/*===========================================================================*/
