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

#include "azimuth/view/baddie_spiner.h"

#include <assert.h>
#include <math.h>

#include <SDL_opengl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_spine(float flare, float frozen) {
  glBegin(GL_TRIANGLE_STRIP); {
    glColor4f(0.5f * flare, 0.3, 0, 0);
    glVertex2f(-3, 3);
    glColor3f(0.6f + 0.4f * flare, 0.7, 0.6);
    glVertex2f(5, 0);
    glColor3f(0.6f + 0.4f * flare, 0.7, frozen);
    glVertex2f(-5, 0);
    glColor4f(0, 0.3, 0, 0);
    glVertex2f(-3, -3);
  } glEnd();
}

static void draw_spiner(
    az_color_t inner, az_color_t outer, const az_baddie_t *baddie,
    float flare, float frozen, az_clock_t clock) {
  if (baddie->cooldown < 1.0) {
    for (int i = 0; i < 360; i += 45) {
      glPushMatrix(); {
        glRotated(i, 0, 0, 1);
        glTranslated(18.0 - 8.0 * baddie->cooldown, 0, 0);
        draw_spine(0, frozen);
      } glPopMatrix();
    }
  }
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(-2, 2); az_gl_color(outer);
    for (int i = 0; i <= 360; i += 15) {
      double radius = baddie->data->main_body.bounding_radius +
        0.2 * az_clock_zigzag(10, 3, clock) - 1.0;
      if (i % 45 == 0) radius -= 2.0;
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  for (int i = 0; i < 360; i += 45) {
    glPushMatrix(); {
      glRotated(i + 22.5, 0, 0, 1);
      glTranslated(16 + 0.5 * az_clock_zigzag(6, 5, clock), 0, 0);
      draw_spine(0, frozen);
    } glPopMatrix();
  }
  for (int i = 0; i < 360; i += 45) {
    glPushMatrix(); {
      glRotated(i + 11.25, 0, 0, 1);
      glTranslated(8 + 0.5 * az_clock_zigzag(6, 7, clock), 0, 0);
      draw_spine(0, frozen);
    } glPopMatrix();
  }
}

static void draw_urchin(const az_baddie_t *baddie, float frozen,
                        az_clock_t clock) {
  const float flare = baddie->armor_flare;
  for (int i = 0; i < 18; ++i) {
    glPushMatrix(); {
      glRotated(i * 20, 0, 0, 1);
      glTranslated(7.5 + 0.5 * az_clock_zigzag(6, 3, clock + i * 7), 0, 0);
      draw_spine(flare, frozen);
    } glPopMatrix();
  }
  for (int i = 0; i < 9; ++i) {
    glPushMatrix(); {
      glRotated(i * 40 + 30, 0, 0, 1);
      glTranslated(4 + 0.5 * az_clock_zigzag(7, 3, clock - i * 13), 0, 0);
      draw_spine(flare, frozen);
    } glPopMatrix();
  }
}

/*===========================================================================*/

void az_draw_bad_spine_mine(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SPINE_MINE);
  draw_urchin(baddie, frozen, clock);
}

void az_draw_bad_spiner(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SPINER);
  const float flare = baddie->armor_flare;
  draw_spiner(az_color3f(1 - frozen, 1 - 0.5 * flare, frozen),
              az_color3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen),
              baddie, flare, frozen, clock);
}

void az_draw_bad_super_spiner(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SUPER_SPINER);
  const float flare = baddie->armor_flare;
  draw_spiner(az_color3f(0.5 + 0.5 * flare - 0.5 * frozen, 0.25 + 0.5 * frozen,
                         1 - 0.75 * flare),
              az_color3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen),
              baddie, flare, frozen, clock);
}

void az_draw_bad_urchin(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_URCHIN);
  draw_urchin(baddie, frozen, clock);
}

/*===========================================================================*/
