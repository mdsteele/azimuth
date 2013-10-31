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

#include "azimuth/view/baddie_turret.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

static void az_gl_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

/*===========================================================================*/

static void draw_zipper_antennae(az_color_t color) {
  az_gl_color(color);
  glBegin(GL_LINE_STRIP); {
    glVertex2f(23, 4); glVertex2f(16, 0); glVertex2f(23, -4);
  } glEnd();
}

static void draw_zipper_body(
    az_color_t inner1, az_color_t inner2, az_color_t outer, double yscale) {
  for (int i = -1; i <= 1; i += 2) {
    glBegin(GL_QUAD_STRIP); {
      for (int x = 20; x >= -15; x -= 5) {
        const double y = i * yscale * (1 - pow(0.05 * x, 4) + 0.025 * x);
        if (x % 2) az_gl_color(inner1);
        else az_gl_color(inner2);
        glVertex2d(x, 0);
        az_gl_color(outer);
        glVertex2d(x, y);
      }
    } glEnd();
  }
}

static void draw_zipper_wings(
    GLfloat xcenter, double theta1, double theta2,
    float flare, float frozen, az_clock_t clock) {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      glPushMatrix(); {
        glTranslatef(xcenter, 0, 0);
        glRotated((j ? theta2 : theta1) * 6.0 *
                  (1 + az_clock_zigzag(4, 1, clock)), 0, 0, 1);
        glBegin(GL_QUAD_STRIP); {
          glColor4f(1 - frozen, 1 - flare, 1 - flare, 0.25);
          glVertex2f(-2.5, 0);
          glColor4f(0.5 - 0.5 * frozen + 0.5 * flare, 1 - flare,
                    1 - flare, 0.35);
          glVertex2f(2.5, 0);
          glVertex2f(-5.5, 14); glVertex2f(5.5, 14);
          glVertex2f(-4.5, 18);
          glColor4f(flare, 1 - flare, 1 - flare, 0.35);
          glVertex2f(4.5, 18);
          glVertex2f(-1, 21); glVertex2f(1, 21);
        } glEnd();
      } glPopMatrix();
    }
    glScalef(1, -1, 1);
  }
}

static void draw_zipper(
    az_color_t inner1, az_color_t inner2, az_color_t outer,
    float flare, float frozen, az_clock_t clock) {
  draw_zipper_body(inner1, inner2, outer, 6.0);
  draw_zipper_wings(10, 1.8, 3.8, flare, frozen, clock);
}

/*===========================================================================*/

void az_draw_bad_zipper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper(color3(0.5 + 0.5 * flare - 0.5 * frozen, 1 - flare, frozen),
              color3(0.4 - 0.4 * frozen, 0.4, frozen),
              color3(0.4 * flare, 0.5, frozen), flare, frozen, clock);
}

void az_draw_bad_armored_zipper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper(color3(0.7 + 0.25 * flare - 0.5 * frozen, 0.75 - flare,
                     0.7 + 0.3 * frozen),
              color3(0, 0.4, 0.4 + 0.6 * frozen),
              color3(0.2 + 0.4 * flare, 0.3, 0.2 + 0.8 * frozen),
              flare, frozen, clock);
}

void az_draw_bad_mini_armored_zipper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  glPushMatrix(); {
    glScalef(0.7, 0.7, 1);
    az_draw_bad_armored_zipper(baddie, frozen, clock);
  } glPopMatrix();
}

void az_draw_bad_fire_zipper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper(color3(0.5f + 0.5f * flare - 0.5f * frozen, 0.5f * frozen,
                     1.0f - flare),
              color3(0.6f - 0.6f * frozen, 0.4f, frozen),
              color3(0.25f + 0.25f * flare, 0.25f * frozen,
                     0.5f - 0.5f * flare),
              flare, frozen, clock);
}

void az_draw_bad_mosquito(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  glPushMatrix(); {
    glScalef(0.5, 0.5, 1);
    draw_zipper_antennae(color3(0.5, 0.25, 0.25));
    draw_zipper_body(color3(1 - frozen, 0.5 - 0.5 * flare, frozen),
                     color3(1 - frozen, 0.25, frozen),
                     color3(0.4 + 0.4 * flare, 0, frozen), 8);
    draw_zipper_wings(5, -1.1, 1.9, flare, frozen, clock);
  } glPopMatrix();
}

void az_draw_bad_dragonfly(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper_antennae(color3(0.5, 0.25, 0.25));
  draw_zipper_body(color3(1 - frozen, 0.5 - 0.5 * flare, frozen),
                   color3(1 - frozen, 0.25, frozen),
                   color3(0.4 + 0.4 * flare, 0, frozen), 4);
  draw_zipper_wings(5, -1.1, 1.9, flare, frozen, clock);
}

void az_draw_bad_hornet(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper_antennae(color3(0.5, 0.5, 0.25));
  draw_zipper_body(color3(1 - frozen, 1 - flare, frozen),
                   color3(1 - frozen, 0.5, frozen),
                   color3(0.4 + 0.4 * flare, 0.4, frozen), 4);
  draw_zipper_wings(5, -1.1, 1.9, flare, frozen, clock);
}

void az_draw_bad_super_hornet(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_zipper_antennae(color3(0.25, 0.5, 0.12));
  for (int y = -1; y <= 1; y += 2) {
    for (int i = -3; i <= 4; ++i) {
      glPushMatrix(); {
        glScaled(1, y, 1);
        const double x = 4 * i;
        glTranslated(x, 4 * (1 - pow(0.05 * x, 4) + 0.025 * x), 0);
        glBegin(GL_TRIANGLE_STRIP); {
          glColor3f(0.1, 0.3, 0); glVertex2f(2, -2);
          glColor3f(0.6, 0.9, 0.3); glVertex2f(i - 1, 2);
          glColor3f(0.5, 0.8, 0); glVertex2f(0, -3);
          glColor3f(0.1, 0.3, 0); glVertex2f(-2, -2);
        } glEnd();
      } glPopMatrix();
    }
  }
  draw_zipper_body(color3(0.6f - 0.6f * frozen, 0.5f - 0.5f * flare, frozen),
                   color3(0, 1.0f - 0.5f * frozen, frozen),
                   color3(0.2 + 0.6 * flare, 0.4, frozen), 4);
  draw_zipper_wings(5, -1.1, 1.9, flare, frozen, clock);
}

void az_draw_bad_switcher(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  glPushMatrix(); {
    glTranslatef(-2.5, 0, 0);
    glScalef(0.75, 1, 1);
    draw_zipper_antennae(color3(0.25, 0.75, 0.25));
    draw_zipper_body(color3(flare, 0.5 + 0.25 * frozen, 0.75),
                     color3(0.5 - 0.5 * frozen, 1, 0.5 + 0.25 * frozen),
                     color3(0.2 + 0.4 * flare, 0.1 + 0.4 * frozen,
                            0.4 + 0.4 * frozen), 8);
  } glPopMatrix();
  draw_zipper_wings(8, 1.8, 3.8, flare, frozen,
                    (baddie->state == 0 ? 4 : clock));
}

/*===========================================================================*/
