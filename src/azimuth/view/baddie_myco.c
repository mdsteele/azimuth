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

#include "azimuth/view/baddie_myco.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

/*===========================================================================*/

static void draw_shroom_spot(double cx, double cy, double w, double h) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(0.1, 0.2, 0.3, 0.6); glVertex2f(cx, cy);
    glColor4f(0, 0, 0.3, 0);
    for (int i = 0; i <= 360; i += 30) {
      const double y = cy + h * sin(AZ_DEG2RAD(i));
      glVertex2d(cx + w * cos(AZ_DEG2RAD(i)) - 0.02 * cy * (y - cy), y);
    }
  } glEnd();
}

/*===========================================================================*/

void az_draw_bad_mycoflakker(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  // Stem:
  {
    const az_color_t inner =
      color3(0.7f + 0.3f * flare - 0.6f * frozen,
             0.7f - 0.4f * flare, 0.9f - 0.5f * flare);
    const az_color_t outer =
      color3(0.3f + 0.3f * flare - 0.2f * frozen,
             0.3f - 0.2f * flare, 0.4f - 0.2f * flare);
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(outer); glVertex2f(-12,   9); glVertex2f(12,  7);
      az_gl_color(inner); glVertex2f(-12,  -1); glVertex2f(12,  0);
      az_gl_color(outer); glVertex2f(-12, -11); glVertex2f(12, -6);
    } glEnd();
    az_color_t lower = inner; lower.a = 0;
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(outer); glVertex2f(-12,   9);
      az_gl_color(inner); glVertex2f(-12,  -1);
      az_gl_color(lower); glVertex2f(-14,  -1);
      az_gl_color(outer); glVertex2f(-12, -11);
    } glEnd();
  }
  // Cap:
  glPushMatrix(); {
    glTranslatef(10, 0, 0);
    if (baddie->state != 0) {
      glRotatef(az_clock_zigzag(9, 2, clock) - 4, 0, 0, 1);
    }
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6f + 0.5f * flare - 0.6f * frozen, 0.4f + 0.3f * frozen,
                1.0f - 0.5f * flare);
      glVertex2f(0, 0);
      glColor3f(0.2f + 0.3f * flare - 0.2f * frozen, 0.15f + 0.1f * frozen,
                0.5f - 0.25f * flare);
      for (int i = 0; i <= 360; i += 15) {
        const double rho = 10.0 * (1.0 + cos(sin(1.5 * AZ_DEG2RAD(i))));
        glVertex2d(2 + 0.5 * rho * cos(AZ_DEG2RAD(i)),
                   rho * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    draw_shroom_spot(4, 10, 2.5, 5.5);
    draw_shroom_spot(1, -11, 3.5, 4);
    draw_shroom_spot(9, -3, 2, 4);
  } glPopMatrix();
}

/*===========================================================================*/
