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

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_flakker_stem(float flare, float frozen) {
  const az_color_t inner =
    az_color3f(0.7f + 0.3f * flare - 0.6f * frozen,
               0.7f - 0.4f * flare, 0.9f - 0.5f * flare);
  const az_color_t outer =
    az_color3f(0.3f + 0.3f * flare - 0.2f * frozen,
               0.3f - 0.2f * flare, 0.4f - 0.2f * flare);
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); glVertex2f(-16,   9); glVertex2f(8,  7);
    az_gl_color(inner); glVertex2f(-16,  -1); glVertex2f(8,  0);
    az_gl_color(outer); glVertex2f(-16, -11); glVertex2f(8, -6);
  } glEnd();
  az_color_t lower = inner; lower.a = 0;
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); glVertex2f(-16,   9);
    az_gl_color(inner); glVertex2f(-16,  -1);
    az_gl_color(lower); glVertex2f(-18,  -1);
    az_gl_color(outer); glVertex2f(-16, -11);
  } glEnd();
}

static void draw_stalker_stem_and_feet(float flare, float frozen,
                                       az_clock_t clock) {
  const az_color_t inner =
    az_color3f(0.7f + 0.3f * flare - 0.6f * frozen,
               0.7f - 0.4f * flare, 0.9f - 0.5f * flare);
  const az_color_t outer =
    az_color3f(0.3f + 0.3f * flare - 0.2f * frozen,
               0.3f - 0.2f * flare, 0.4f - 0.2f * flare);
  // Stem:
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); glVertex2f(0,  8); glVertex2f(10,  7);
    az_gl_color(inner); glVertex2f(0, -1); glVertex2f(10,  0);
    az_gl_color(outer); glVertex2f(0, -7); glVertex2f(10, -6);
  } glEnd();
  // Feet:
  const GLfloat offset = 0.8f * (az_clock_zigzag(5, 5, clock) - 2.0f);
  for (int j = 0; j < 4; ++j) {
    const int i = (2 + j) % 4;
    glBegin(GL_TRIANGLE_STRIP); {
      const GLfloat x = (i == 0 || i == 3 ? -20.0f : -22.0f);
      const GLfloat y = -12.0f + 8.0f * i + (2 * (i % 2) - 1) * offset;
      az_gl_color(outer); glVertex2f(0,  8); glVertex2f(x, y + 2);
      az_gl_color(inner); glVertex2f(0, -1); glVertex2f(x - 1, y);
      az_gl_color(outer); glVertex2f(0, -7); glVertex2f(x, y - 2);
    } glEnd();
  }
}

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

static void draw_shroom_cap(az_color_t inner, az_color_t outer,
                            GLfloat trans_x, bool wiggle, az_clock_t clock) {
  glPushMatrix(); {
    glTranslatef(trans_x, 0, 0);
    if (wiggle) {
      glRotatef(az_clock_zigzag(9, 2, clock) - 4, 0, 0, 1);
    }
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner);
      glVertex2f(0, 0);
      az_gl_color(outer);
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

void az_draw_bad_mycoflakker(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MYCOFLAKKER);
  const float flare = baddie->armor_flare;
  draw_flakker_stem(flare, frozen);
  draw_shroom_cap(az_color3f(0.6f + 0.4f * flare - 0.6f * frozen,
                             0.4f + 0.3f * frozen, 1.0f - 0.5f * flare),
                  az_color3f(0.2f + 0.3f * flare - 0.2f * frozen,
                             0.15f + 0.1f * frozen, 0.5f - 0.25f * flare),
                  6, (baddie->state != 0), clock);
}

void az_draw_bad_mycostalker(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MYCOSTALKER);
  const float flare = baddie->armor_flare;
  draw_stalker_stem_and_feet(flare, frozen, clock);
  draw_shroom_cap(az_color3f(0.7f + 0.3f * flare - 0.6f * frozen,
                             0.4f + 0.3f * frozen, 0.9f - 0.5f * flare),
                  az_color3f(0.3f + 0.3f * flare - 0.2f * frozen,
                             0.15f + 0.1f * frozen, 0.4f - 0.25f * flare),
                  8, (baddie->state != 0), clock);
}

void az_draw_bad_pyroflakker(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_PYROFLAKKER);
  const float flare = baddie->armor_flare;
  draw_flakker_stem(flare, frozen);
  draw_shroom_cap(az_color3f(1.0f - 0.9f * frozen,
                             0.4f + 0.3f * flare + 0.3f * frozen,
                             1.0f * frozen),
                  az_color3f(0.5f - 0.4f * frozen,
                             0.2f + 0.2f * flare + 0.2f * frozen,
                             0.5f * frozen),
                  6, (baddie->state != 0), clock);
}

void az_draw_bad_pyrostalker(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_PYROSTALKER);
  const float flare = baddie->armor_flare;
  draw_stalker_stem_and_feet(flare, frozen, clock);
  draw_shroom_cap(az_color3f(1.0f - 0.9f * frozen,
                             0.6f + 0.3f * flare + 0.1f * frozen,
                             1.0f * frozen),
                  az_color3f(0.5f - 0.4f * frozen,
                             0.3f + 0.2f * flare + 0.2f * frozen,
                             0.5f * frozen),
                  8, (baddie->state != 0), clock);
}

/*===========================================================================*/
