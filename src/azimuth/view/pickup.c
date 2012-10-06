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

#include "azimuth/view/pickup.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <OpenGL/gl.h>

#include "azimuth/state/pickup.h"
#include "azimuth/state/space.h"
#include "azimuth/util.h"
#include "azimuth/vector.h"

/*===========================================================================*/

static void draw_pickup(az_pickup_kind_t kind, unsigned long clock) {
  // TODO: draw different kinds of pickups differently
  double radius = 0.5 * (double)(clock % 16);
  switch (kind) {
    case AZ_PUP_ROCKETS:
      glColor3f(1, 0, 0);
      glBegin(GL_LINES); {
        glVertex2d(-2.5,  2.5);
        glVertex2d( 2.5,  2.5);
        glVertex2d(-2.5, -0.5);
        glVertex2d( 2.5, -0.5);
      } glEnd();
      glBegin(GL_LINE_STRIP); {
        glVertex2d(-2.5, -1.5);
        glVertex2d(-4.5, -4.5);
        glVertex2d(-1.5, -3.5);
      } glEnd();
      glBegin(GL_LINE_STRIP); {
        glVertex2d( 2.5, -1.5);
        glVertex2d( 4.5, -4.5);
        glVertex2d( 1.5, -3.5);
      } glEnd();
      glColor3f(0, (clock % 8 < 4 ? 0 : 1), 1);
      glBegin(GL_LINE_LOOP); {
        glVertex2d( 1.5,  4.5);
        glVertex2d( 2.5,  2.5);
        glVertex2d( 2.5, -1.5);
        glVertex2d( 1.5, -3.5);
        glVertex2d(-1.5, -3.5);
        glVertex2d(-2.5, -1.5);
        glVertex2d(-2.5,  2.5);
        glVertex2d(-1.5,  4.5);
      } glEnd();
      break;
    case AZ_PUP_BOMBS:
      glColor3f(1, (clock % 8 < 4 ? 0 : 1), 0);
      glBegin(GL_POLYGON); {
        glVertex2d( 1.5,  2.5);
        glVertex2d( 2.5,  1.5);
        glVertex2d( 2.5, -1.5);
        glVertex2d( 1.5, -2.5);
        glVertex2d(-1.5, -2.5);
        glVertex2d(-2.5, -1.5);
        glVertex2d(-2.5,  1.5);
        glVertex2d(-1.5,  2.5);
      } glEnd();
      glColor3f(0, 0, 1);
      glBegin(GL_LINE_LOOP); {
        glVertex2d( 2.5,  4.5);
        glVertex2d( 4.5,  2.5);
        glVertex2d( 4.5, -2.5);
        glVertex2d( 2.5, -4.5);
        glVertex2d(-2.5, -4.5);
        glVertex2d(-4.5, -2.5);
        glVertex2d(-4.5,  2.5);
        glVertex2d(-2.5,  4.5);
      } glEnd();
      break;
    case AZ_PUP_LARGE_SHIELDS:
      radius *= 1.3; // fallthrough
    case AZ_PUP_MEDIUM_SHIELDS:
      radius *= 1.3; // fallthrough
    case AZ_PUP_SMALL_SHIELDS:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0, 1, 1, 0);
        glVertex2d(0, 0);
        glColor4f(0, 1, 1, 1);
        for (int i = 0; i <= 16; ++i) {
          glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                     radius * sin(i * AZ_PI_EIGHTHS));
        }
      } glEnd();
      break;
    default: assert(false);
  }
}

void az_draw_pickups(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(pickup->position.x, pickup->position.y, 0);
      glRotated(AZ_RAD2DEG(-az_vtheta(pickup->position)), 0, 0, 1);
      draw_pickup(pickup->kind, state->clock);
    } glPopMatrix();
  }
}

/*===========================================================================*/
