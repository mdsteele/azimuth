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

#include "azimuth/view/ship.h"

#include <OpenGL/gl.h>

#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util.h"

/*===========================================================================*/

void az_draw_ship(const az_space_state_t* state) {
  glPushMatrix(); {
    glTranslated(state->ship.position.x, state->ship.position.y, 0);
    glRotated(AZ_RAD2DEG(state->ship.angle), 0, 0, 1);
    // TODO: tractor beam, if active
    // Exhaust:
    if (1 /*FIXME: only when using engines*/) {
      double zig = az_clock_zigzag(10, 1, state->clock);
      // From port engine:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor4f(1, 0.5, 0, 0); // transparent orange
        glVertex2d(-13, 12);
        glColor4f(1, 0.75, 0, 0.9); // orange
        glVertex2d(-13, 9);
        glColor4f(1, 0.5, 0, 0); // transparent orange
        glVertex2d(-23 - zig, 9);
        glVertex2d(-13, 7);
      } glEnd();
      // From starboard engine:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor4f(1, 0.5, 0, 0); // transparent orange
        glVertex2d(-13, -12);
        glColor4f(1, 0.75, 0, 0.9); // orange
        glVertex2d(-13, -9);
        glColor4f(1, 0.5, 0, 0); // transparent orange
        glVertex2d(-23 - zig, -9);
        glVertex2d(-13, -7);
      } glEnd();
    }
    // TODO: exhaust for lateral thrusters and afterburner, if active
    // Engines:
    glBegin(GL_QUADS); {
      // Struts:
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( -2,  9);
      glVertex2d(-10,  9);
      glVertex2d(-10, -9);
      glVertex2d( -2, -9);
      // Port engine:
      glVertex2d(-13,  12);
      glVertex2d(  3,  12);
      glColor3f(0.75, 0.75, 0.75); // light gray
      glVertex2d(  5,   7);
      glVertex2d(-14,   7);
      // Starboard engine:
      glVertex2d(  5,  -7);
      glVertex2d(-14,  -7);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d(-13, -12);
      glVertex2d(  3, -12);
    } glEnd();
    // Main body:
    glBegin(GL_QUAD_STRIP); {
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( 12,  4);
      glVertex2d(-17,  4);
      glColor3f(0.75, 0.75, 0.75); // light gray
      glVertex2d( 17,  0);
      glVertex2d(-17,  0);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( 12, -4);
      glVertex2d(-17, -4);
    } glEnd();
    // Windshield:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0, 0.5, 0.5); // dim cyan
      glVertex2d(11,  2);
      glColor3f(0, 1, 1); // cyan
      glVertex2d(15,  0);
      glVertex2d( 9,  0);
      glColor3f(0, 0.5, 0.5); // dim cyan
      glVertex2d(11, -2);
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/
