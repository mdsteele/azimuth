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

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_projectile(const az_projectile_t* proj, az_clock_t clock) {
  switch (proj->kind) {
    default: // TODO: add other types
    case AZ_PROJ_GUN_NORMAL:
    case AZ_PROJ_GUN_TRIPLE:
    case AZ_PROJ_GUN_SHRAPNEL:
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
    case AZ_PROJ_GUN_CHARGED_NORMAL:
    case AZ_PROJ_GUN_CHARGED_TRIPLE:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.75); // white
        glVertex2d( 0.0,  0.0);
        glVertex2d( 4.0,  0.0);
        glVertex2d( 3.0,  3.0);
        glVertex2d( 0.0,  4.0);
        glVertex2d(-3.0,  3.0);
        glColor4f(1, 1, 1, 0); // transparent white
        glVertex2d(-20.,  0.0);
        glColor3f(1, 1, 1); // white
        glVertex2d(-3.0, -3.0);
        glVertex2d( 0.0, -4.0);
        glVertex2d( 3.0, -3.0);
        glVertex2d( 4.0,  0.0);
      } glEnd();
      break;
    case AZ_PROJ_GUN_HOMING:
    case AZ_PROJ_GUN_TRIPLE_HOMING:
    case AZ_PROJ_GUN_HOMING_SHRAPNEL:
      glBegin(GL_TRIANGLES); {
        glColor3f(0, 0, 1); // blue
        glVertex2d(4, 0);
        glVertex2d(-4, 2);
        glVertex2d(-4, -2);
      } glEnd();
      break;
    case AZ_PROJ_GUN_CHARGED_HOMING:
      glBegin(GL_TRIANGLES); {
        glColor3f(0, 0, 1); // blue
        glVertex2d(8, 0);
        glVertex2d(-8, 4);
        glVertex2d(-8, -4);
      } glEnd();
      break;
    case AZ_PROJ_GUN_BURST:
    case AZ_PROJ_GUN_TRIPLE_BURST:
    case AZ_PROJ_GUN_HOMING_BURST:
    case AZ_PROJ_GUN_BURST_PIERCE:
      glPushMatrix(); {
        glRotated(10 * az_clock_mod(36, 1, clock), 0, 0, 1);
        glColor3f(0.75, 0.5, 0.25);
        glBegin(GL_QUADS); {
          glVertex2d(5, 0);
          glVertex2d(2, 3);
          glVertex2d(-1, 0);
          glVertex2d(2, -3);
          glVertex2d(-5, 0);
          glVertex2d(-2, -3);
          glVertex2d(1, 0);
          glVertex2d(-2, 3);
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_PROJ_GUN_PIERCE:
    case AZ_PROJ_GUN_TRIPLE_PIERCE:
    case AZ_PROJ_GUN_HOMING_PIERCE:
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
    case AZ_PROJ_ROCKET:
      glColor3f(0.5, 0, 0); // dark red
      glBegin(GL_QUADS); {
        const int y = 2 - az_clock_mod(6, 2, clock);
        glVertex2i(-11, y);
        glVertex2i(-11, y + 2);
        glVertex2i(-4, y + 2);
        glVertex2i(-4, y);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(-9, -2);
        glVertex2i(2, -2);
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(-9, 0);
        glVertex2i(4, 0);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(-9, 2);
        glVertex2i(2, 2);
      } glEnd();
      glColor3f(0.5, 0, 0); // dark red
      glBegin(GL_QUADS); {
        const int y = -4 + az_clock_mod(6, 2, clock);
        glVertex2i(-11, y);
        glVertex2i(-11, y + 2);
        glVertex2i(-4, y + 2);
        glVertex2i(-4, y);
      } glEnd();
      break;
    case AZ_PROJ_BOMB:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, 0);
        const double radius = 4.0;
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          if (blue) glColor3f(0, 0, 0.75); // blue
          else glColor3f(0.5, 0.5, 0.5); // gray
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
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
      draw_projectile(proj, state->clock);
    } glPopMatrix();
  }
}

/*===========================================================================*/
