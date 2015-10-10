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

#include "azimuth/view/baddie_chomper.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

/*===========================================================================*/

void draw_stalk(const az_baddie_t *baddie, int first_component_index,
                bool thorns, float length_scale, bool expand,
                az_color_t center_color, az_color_t side_color) {
  float y = 4.0f;
  for (int j = first_component_index; j < baddie->data->num_components; ++j) {
    glPushMatrix(); {
      const az_component_t *component = &baddie->components[j];
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      if (thorns) {
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.75, 0.75, 0.75); glVertex2f(0, 0);
          glColor3f(0.3, 0.3, 0.3); glVertex2f(4, 0); glVertex2f(0, y + 4);
          glVertex2f(-4, 0); glVertex2f(0, -y - 4); glVertex2f(4, 0);
        } glEnd();
      }
      glScalef(length_scale, 1, 1);
      glBegin(GL_QUAD_STRIP); {
        az_gl_color(side_color);   glVertex2f(-12,  y); glVertex2f(12,  y);
        az_gl_color(center_color); glVertex2f(-15,  0); glVertex2f(15,  0);
        az_gl_color(side_color);   glVertex2f(-12, -y); glVertex2f(12, -y);
      } glEnd();
      if (expand) y += 0.3f;
    } glPopMatrix();
  }
}

void draw_base(const az_baddie_t *baddie, az_color_t center_color,
               az_color_t side_color, float expand) {
  glPushMatrix(); {
    const az_component_t *base = &baddie->components[0];
    az_gl_translated(base->position);
    az_gl_rotated(base->angle);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(center_color);
      glVertex2f(0, 0);
      az_gl_color(side_color);
      glVertex2f(0, -14); glVertex2f(8, -11); glVertex2f(14, -4 - expand);
      az_gl_color(center_color);
      glVertex2f(14, 0);
      az_gl_color(side_color);
      glVertex2f(14, 4 + expand); glVertex2f(8, 11); glVertex2f(0, 14);
    } glEnd();
  } glPopMatrix();
}

void draw_core(const az_baddie_t *baddie, float flare, float frozen) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.75f + 0.25f * flare - 0.75f * frozen, 0.25f + 0.35f * frozen,
              0.5f - 0.2f * flare + 0.5f * frozen);
    glVertex2f(0, 0);
    glColor3f(0.25f + 0.1f * flare - 0.25f * frozen,
              0.2f * flare + 0.4f * frozen, 0.5f * frozen);
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int j = 0; j < polygon.num_vertices; ++j) {
      az_gl_vertex(polygon.vertices[j]);
    }
  } glEnd();
}

void draw_pincers(const az_baddie_t *baddie, bool thorns,
                  az_color_t center_color, az_color_t side_color) {
  // Teeth:
  for (int i = 1; i <= 2; ++i) {
    glPushMatrix(); {
      const az_component_t *pincer = &baddie->components[i];
      az_gl_translated(pincer->position);
      az_gl_rotated(pincer->angle);
      glBegin(GL_TRIANGLES); {
        glColor3f(0.5, 0.5, 0.5);
        const GLfloat y = (i % 2 ? -4 : 4);
        for (GLfloat x = 25 - 3 * i; x > 5; x -= 6) {
          glVertex2f(x + 2, 0); glVertex2f(x, y); glVertex2f(x - 2, 0);
        }
      } glEnd();
      // Thorns:
      if (thorns) {
        for (int j = 10; j <= 150; j += 20) {
          glBegin(GL_TRIANGLE_FAN); {
            const double sign = 3 - i * 2;
            glColor3f(0.75, 0.75, 0.75);
            glVertex2d(9 + 14 * cos(AZ_DEG2RAD(j)),
                       sign * (3 + 7 * sin(AZ_DEG2RAD(j))));
            glColor3f(0.3, 0.3, 0.3);
            glVertex2d(9 + 14 * cos(AZ_DEG2RAD(j - 10)),
                       sign * (3 + 7 * sin(AZ_DEG2RAD(j - 10))));
            glVertex2d(9 + 20 * cos(AZ_DEG2RAD(j)),
                       sign * (3 + 14 * sin(AZ_DEG2RAD(j))));
            glVertex2d(9 + 14 * cos(AZ_DEG2RAD(j + 10)),
                       sign * (3 + 7 * sin(AZ_DEG2RAD(j + 10))));
          } glEnd();
        }
      }
    } glPopMatrix();
  }
  // Pincers:
  for (int i = 1; i <= 2; ++i) {
    glPushMatrix(); {
      const az_component_t *pincer = &baddie->components[i];
      az_gl_translated(pincer->position);
      az_gl_rotated(pincer->angle);
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(center_color);
        glVertex2f(0, 0);
        az_gl_color(side_color);
        const az_polygon_t poly = baddie->data->components[i].polygon;
        for (int j = 0, k = poly.num_vertices; j >= 0; j = --k) {
          az_gl_vertex(poly.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
}

/*===========================================================================*/

void az_draw_bad_chomper_plant(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_CHOMPER_PLANT);
  const float flare = baddie->armor_flare;
  const az_color_t center_color =
    color3(0.5f - 0.5f * frozen, 1.0f - 0.5f * flare, frozen);
  const az_color_t side_color =
    color3(0.4f * flare, 0.3f - 0.3f * flare, 0.4f * frozen);
  draw_stalk(baddie, 3, true, 1.0, false, center_color, side_color);
  draw_base(baddie, center_color, side_color, 0);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, false, center_color, side_color);
}

void az_draw_bad_aquatic_chomper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_AQUATIC_CHOMPER);
  const float flare = baddie->armor_flare;
  const az_color_t center_color =
    color3(0.4f + 0.5f * flare, 0.3f - 0.2f * flare + 0.5f * frozen,
           0.8f - 0.5f * flare);
  const az_color_t side_color =
    color3(0.1f + 0.3f * flare, 0.2f - 0.2f * flare + 0.3f * frozen, 0.35f);
  draw_stalk(baddie, 3, false, 0.5, false, center_color, side_color);
  draw_base(baddie, center_color, side_color, 0);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, false, center_color, side_color);
}

void az_draw_bad_jungle_chomper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_JUNGLE_CHOMPER);
  const float flare = baddie->armor_flare;
  const az_color_t center_color =
    color3(0.75f - 0.75f * frozen, 1.0f - 0.5f * flare, frozen);
  const az_color_t side_color =
    color3(0.15f + 0.4f * flare, 0.3f - 0.3f * flare, 0.4f * frozen);
  draw_stalk(baddie, 3, true, 1.0, false, center_color, side_color);
  draw_base(baddie, center_color, side_color, 0);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, true, center_color, side_color);
}

void az_draw_bad_fire_chomper(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_FIRE_CHOMPER);
  const float flare = baddie->armor_flare;
  const az_color_t center_color =
    color3(0.8f - 0.7f * frozen, 0.3f + 0.2f * flare + 0.5f * frozen,
           0.1f - 0.1f * flare + 0.9f * frozen);
  const az_color_t side_color =
    color3(0.4f - 0.4f * frozen, 0.2f + 0.2f * flare + 0.4f * frozen,
           0.1f - 0.1f * flare + 0.6f * frozen);
  draw_stalk(baddie, 3, false, 0.5, false, center_color, side_color);
  draw_base(baddie, center_color, side_color, 0);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, true, center_color, side_color);
}

void az_draw_bad_spiked_vine(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SPIKED_VINE);
  const float flare = baddie->armor_flare;
  const az_color_t center_color =
    color3(0.4f + 0.2f * flare - 0.4f * frozen,
           0.8f - 0.3f * flare, 0.4f - 0.1f * flare + 0.6f * frozen);
  const az_color_t side_color =
    color3(0.4f * flare, 0.3f - 0.1f * flare,
           0.2f - 0.1f * flare + 0.4f * frozen);
  draw_stalk(baddie, 1, true, 0.5 + fmin(0.5, baddie->param / 28.0), true,
             center_color, side_color);
  glBegin(GL_TRIANGLE_FAN); {
    const az_polygon_t tip = baddie->data->main_body.polygon;
    az_gl_color(center_color);
    az_gl_vertex(tip.vertices[0]);
    az_gl_color(side_color);
    for (int i = 1; i < tip.num_vertices; ++i) {
      az_gl_vertex(tip.vertices[i]);
    }
  } glEnd();
  draw_base(baddie, center_color, side_color,
            3.0 * fmin(1.0, baddie->param / 14.0));
}

/*===========================================================================*/
