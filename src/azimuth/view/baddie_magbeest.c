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

#include "azimuth/view/baddie_magbeest.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

// Comparison function for use with qsort.  Sorts doubles by their cosines.
static int compare_cosines(const void* v1, const void* v2) {
  const double c1 = cos(*(double*)v1);
  const double c2 = cos(*(double*)v2);
  return (c1 < c2 ? -1 : c1 > c2 ? 1 : 0);
}

static void draw_magbeest_leg(const az_baddie_t *baddie, int leg_index) {
  assert(0 <= leg_index && leg_index < baddie->data->num_components);
  const az_component_t *leg = &baddie->components[leg_index];
  const double length =
    baddie->data->components[leg_index].polygon.vertices[1].x;
  glPushMatrix(); {
    az_gl_translated(leg->position);
    az_gl_rotated(leg->angle);
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.2, 0.2, 0.2); glVertex2f(0,  10); glVertex2f(length,  10);
      glColor3f(0.4, 0.5, 0.5); glVertex2f(0,   0); glVertex2f(length,   0);
      glColor3f(0.2, 0.2, 0.2); glVertex2f(0, -10); glVertex2f(length, -10);
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_bad_magbeest_head(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  // TODO: draw casing
  // TODO: draw eye
}

void az_draw_bad_magbeest_legs_l(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L);
  // Legs:
  draw_magbeest_leg(baddie, 1);
  draw_magbeest_leg(baddie, 2);
  draw_magbeest_leg(baddie, 3);
  draw_magbeest_leg(baddie, 4);
  // TODO: draw magnet
  // TODO: draw base casing
}

void az_draw_bad_magbeest_legs_r(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  // Legs:
  draw_magbeest_leg(baddie, 1);
  draw_magbeest_leg(baddie, 2);
  draw_magbeest_leg(baddie, 3);
  draw_magbeest_leg(baddie, 4);
  // Gatling gun:
  glPushMatrix(); {
    const az_component_t *gun = &baddie->components[0];
    az_gl_translated(gun->position);
    az_gl_rotated(gun->angle);
    // Barrels:
    double thetas[5];
    const int num_thetas = AZ_ARRAY_SIZE(thetas);
    for (int i = 0; i < num_thetas; ++i) {
      thetas[i] = baddie->param + i * AZ_TWO_PI / num_thetas;
    }
    qsort(thetas, num_thetas, sizeof(double), compare_cosines);
    for (int i = 0; i < num_thetas; ++i) {
      const double theta = thetas[i];
      const GLfloat y_offset = 7 * sin(theta);
      const GLfloat x_ext = 0.5 * cos(theta);
      const GLfloat z_fade = 0.6 + 0.4 * cos(theta);
      glBegin(GL_TRIANGLE_STRIP); {
        const GLfloat outer = 0.25f * z_fade;
        const GLfloat inner = 0.75f * z_fade;
        glColor3f(outer, outer, outer);
        glVertex2f(12, y_offset + 3); glVertex2f(50 + x_ext, y_offset + 3);
        glColor3f(inner, inner, inner);
        glVertex2f(12, y_offset);     glVertex2f(50 + x_ext, y_offset);
        glColor3f(outer, outer, outer);
        glVertex2f(12, y_offset - 3); glVertex2f(50 + x_ext, y_offset - 3);
      } glEnd();
    }
    // Case:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6, 0.6, 0.6);
      glVertex2f(12, 15); glVertex2f(-20, 15);
      glVertex2f(-20, -15); glVertex2f(12, -15);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, 10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-17, 10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, -15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-17, -10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, -15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, -10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(13, 0);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(11, 0);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, 10);
    } glEnd();
  } glPopMatrix();
  // TODO: draw base casing
}

void az_draw_bad_magma_bomb(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGMA_BOMB);
  const float flare = baddie->armor_flare;
  const bool blink = (baddie->cooldown < 1.5 &&
                      0 != (int)(4 * baddie->cooldown) % 2);
  const double radius = baddie->data->main_body.bounding_radius;
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.35f + 0.6f * flare, 0.35f, 0.35f); glVertex2f(0, 0);
    glColor3f(0.1f + 0.4f * flare, 0.1f, 0.1f);
    for (int i = 0; i <= 360; i += 20) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_LINE_STRIP); {
    if (blink) glColor3f(0.8, 0.6, 0.6);
    else glColor3f(0.3, 0.1, 0.1);
    glVertex2f(0, radius);
    if (blink) glColor3f(1, 0.7, 0.7);
    else glColor3f(0.6, 0.4, 0.4);
    glVertex2f(0, 0);
    if (blink) glColor3f(0.8, 0.6, 0.6);
    else glColor3f(0.3, 0.1, 0.1);
    glVertex2f(0, -radius);
  } glEnd();
}

/*===========================================================================*/
