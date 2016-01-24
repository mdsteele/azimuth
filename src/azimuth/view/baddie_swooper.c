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

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_swooper_feet(const az_baddie_t *baddie, float frozen) {
  glBegin(GL_QUADS); {
    for (int i = 0; i < 2; ++i) {
      glColor3f(0.5f, 0.15f, 0.1f + 0.6f * frozen);
      glVertex2f(0, 2); glVertex2f(0, -2);
      const GLfloat x = (baddie->state == 1 ? -20.0f : -17.0f);
      const GLfloat y = -4.0f + 8.0f * i;
      glVertex2f(x, y - 2);
      glColor3f(0.2f, 0.1f, 0.4f + 0.6f * frozen);
      glVertex2f(x, y + 2);
    }
  } glEnd();
}

static void draw_swooper_body(az_color_t inner, az_color_t outer) {
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner);
    glVertex2f(-6, 0);
    az_gl_color(outer);
    glVertex2f(8, 0); glVertex2f(7, 3);
    glVertex2f(-10, 8); glVertex2f(-16, 6);
    glVertex2f(-14, 0);
    glVertex2f(-16, -6); glVertex2f(-10, -8);
    glVertex2f(7, -3); glVertex2f(8, 0);
  } glEnd();
}

static void draw_swooper_eyes(float frozen) {
  glBegin(GL_TRIANGLES); {
    glColor3f(0.6, 0, frozen);
    glVertex2f(6, 2); glVertex2f(4, 3); glVertex2f(4, 1);
    glVertex2f(6, -2); glVertex2f(4, -3); glVertex2f(4, -1);
  } glEnd();
}

static void draw_swooper_wings(
    const az_baddie_t *baddie, az_clock_t clock,
    az_color_t thumb, az_color_t tips, az_color_t folds) {
  for (int i = -1; i <= 1; i += 2) {
    const GLfloat expand =
      (baddie->state == 1 ? 0.0f :
       (baddie->state == 0 ? 1.0f : 2.0f) *
       (GLfloat)sin((AZ_PI / 48.0) * az_clock_zigzag(24, 1, clock)));
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(thumb);
      glVertex2f(15.0f - 3.5f * expand, i * (2.0f + 6.0f * expand));
      for (int j = 0; j < 7; ++j) {
        if (j % 2) az_gl_color(folds);
        else az_gl_color(tips);
        glVertex2f((j % 2 ? -10.0f : -14.0f) + 0.5f * j,
                   i * (1.0f + (3.0f + expand) * j * j * 0.2f));
      }
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_bad_cave_swooper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_CAVE_SWOOPER);
  const float flare = baddie->armor_flare;
  draw_swooper_feet(baddie, frozen);
  draw_swooper_body(az_color3f(0.2f + 0.8f * flare, 0.4, 0.3f + 0.7f * frozen),
                    az_color3f(0.2f + 0.4f * flare, 0.25, 0.4f * frozen));
  draw_swooper_eyes(frozen);
  draw_swooper_wings(baddie, clock,
      az_color3f(0.4f + 0.6f * flare, 0.6, 0.4 + 0.6f * frozen),
      az_color3f(0.2f + 0.4f * flare, 0.5, 0.2f + 0.4f * frozen),
      az_color3f(0, 0.2, 0.1));
}

void az_draw_bad_echo_swooper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_ECHO_SWOOPER);
  const float flare = baddie->armor_flare;
  draw_swooper_feet(baddie, frozen);
  draw_swooper_body(
      az_color3f(0.3f + 0.7f * flare - 0.1f * frozen, 0.2f,
                 0.4f + 0.6f * frozen),
      az_color3f(0.1f + 0.4f * flare, 0.25f * frozen, 0.25 + 0.25f * frozen));
  draw_swooper_eyes(frozen);
  draw_swooper_wings(baddie, clock,
      az_color3f(0.4f + 0.6f * flare - 0.2f * frozen, 0.4,
                 0.6 + 0.4f * frozen),
      az_color3f(0.2f + 0.4f * flare, 0.2, 0.5f + 0.4f * frozen),
      az_color3f(0.1, 0, 0.2));
}

static az_vector_t transform(const az_baddie_t *baddie, int i, int j) {
  const az_component_t *component = &baddie->components[i];
  return az_vadd(az_vrotate(baddie->data->components[i].polygon.vertices[j],
                            component->angle), component->position);
}

static void avg_vertex(az_vector_t v1, az_vector_t v2) {
  az_gl_vertex(az_vmul(az_vadd(v1, v2), 0.5));
}

void az_draw_bad_demon_swooper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_DEMON_SWOOPER);
  const float flare = baddie->armor_flare;
  const az_color_t inner = az_color3f(0.5f + 0.5f * flare - 0.4f * frozen,
                                      0.2f + 0.4f * frozen, 0.7f * frozen);
  const az_color_t outer = az_color3f(0.25f + 0.4f * flare - 0.2f * frozen,
                                      0.1f * frozen, 0.1f + 0.25f * frozen);
  const int tip = baddie->data->num_components - 1;
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(inner); az_gl_vertex(transform(baddie, 0, 1));
    az_gl_color(outer); az_gl_vertex(transform(baddie, 0, 2));
    for (int i = 1; i < baddie->data->num_components; ++i) {
      az_gl_color(inner);
      avg_vertex(transform(baddie, i - 1, 4), transform(baddie, i, 1));
      az_gl_color(outer);
      avg_vertex(transform(baddie, i - 1, 3), transform(baddie, i, 2));
    }
    az_gl_color(inner); az_gl_vertex(transform(baddie, tip, 5));
    az_gl_color(outer); az_gl_vertex(transform(baddie, tip, 3));
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(inner); az_gl_vertex(transform(baddie, 0, 1));
    az_gl_color(outer); az_gl_vertex(transform(baddie, 0, 0));
    for (int i = 1; i < baddie->data->num_components; ++i) {
      az_gl_color(inner);
      avg_vertex(transform(baddie, i - 1, 4), transform(baddie, i, 1));
      az_gl_color(outer);
      avg_vertex(transform(baddie, i - 1, 5), transform(baddie, i, 0));
    }
    az_gl_color(inner); az_gl_vertex(transform(baddie, tip, 5));
    az_gl_color(outer); az_gl_vertex(transform(baddie, tip, 7));
  } glEnd();
  glPushMatrix(); {
    const az_component_t *component = &baddie->components[tip];
    az_gl_translated(component->position);
    az_gl_rotated(component->angle);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
      for (int i = 4; i <= 6; ++i) {
        az_gl_vertex(baddie->data->components[tip].polygon.vertices[i]);
      }
    } glEnd();
  } glPopMatrix();
  draw_swooper_feet(baddie, frozen);
  draw_swooper_body(inner, outer);
  draw_swooper_eyes(frozen);
  draw_swooper_wings(baddie, clock,
      az_color3f(0.6f + 0.4f * flare - 0.5f * frozen, 0.3f,
                 0.3f + 0.4f * frozen),
      az_color3f(0.5f + 0.4f * flare - 0.4f * frozen, 0.2f,
                 0.2f + 0.4f * frozen),
      az_color3f(0.2, 0.1, 0));
}

/*===========================================================================*/
