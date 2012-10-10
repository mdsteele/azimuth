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

#include "azimuth/view/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_baddie(const az_baddie_t *baddie, unsigned long clock) {
  switch (baddie->kind) {
    case AZ_BAD_LUMP:
      glColor3f(1, 0, 1); // magenta
      glBegin(GL_LINE_LOOP); {
        glVertex2d(20, 0);
        glVertex2d(15, 15);
        glVertex2d(-15, 15);
        glVertex2d(-15, -15);
        glVertex2d(15, -15);
      } glEnd();
      break;
    case AZ_BAD_TURRET:
      glPushMatrix(); {
        glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.25, 0.25, 0.25); // dark gray
          glVertex2d( 0,  5);
          glVertex2d(30,  5);
          glColor3f(0.75, 0.75, 0.75); // light gray
          glVertex2d( 0,  0);
          glVertex2d(30,  0);
          glColor3f(0.25, 0.25, 0.25); // dark gray
          glVertex2d( 0, -5);
          glVertex2d(30, -5);
        } glEnd();
      } glPopMatrix();
      glColor3f(0.5, 0.5, 0.5); // gray
      glBegin(GL_POLYGON); {
        const double radius = 20;
        for (int i = 0; i <= 6; ++i) {
          glVertex2d(radius * cos(i * AZ_PI / 3.0),
                     radius * sin(i * AZ_PI / 3.0));
        }
      } glEnd();
      break;
    default: assert(false);
  }
}

void az_draw_baddies(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(baddie->position.x, baddie->position.y, 0);
      glRotated(AZ_RAD2DEG(baddie->angle), 0, 0, 1);
      draw_baddie(baddie, state->clock);
    } glPopMatrix();
  }
}

/*===========================================================================*/
