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

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

static void az_gl_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

/*===========================================================================*/

void draw_stalk(const az_baddie_t *baddie, bool thorns, float length_scale,
                az_color_t center_color, az_color_t side_color) {
  for (int j = 3; j < baddie->data->num_components; ++j) {
    glPushMatrix(); {
      const az_component_t *component = &baddie->components[j];
      glTranslated(component->position.x, component->position.y, 0);
      glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
      if (thorns) {
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.75, 0.75, 0.75); glVertex2f(0, 0);
          glColor3f(0.3, 0.3, 0.3); glVertex2f(0, 8);
          glVertex2f(-4, 0); glVertex2f(0, -8); glVertex2f(4, 0);
        } glEnd();
      }
      glScalef(length_scale, 1, 1);
      glBegin(GL_QUAD_STRIP); {
        az_gl_color(side_color);   glVertex2f(-12,  4); glVertex2f(12,  4);
        az_gl_color(center_color); glVertex2f(-15,  0); glVertex2f(15,  0);
        az_gl_color(side_color);   glVertex2f(-12, -4); glVertex2f(12, -4);
      } glEnd();
    } glPopMatrix();
  }
}

void draw_base(const az_baddie_t *baddie, az_color_t center_color,
               az_color_t side_color) {
  glPushMatrix(); {
    const az_component_t *base = &baddie->components[0];
    glTranslated(base->position.x, base->position.y, 0);
    glRotated(AZ_RAD2DEG(base->angle), 0, 0, 1);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(center_color);
      glVertex2f(0, 0);
      az_gl_color(side_color);
      glVertex2f(0, -14); glVertex2f(8, -11); glVertex2f(14, -4);
      az_gl_color(center_color);
      glVertex2f(14, 0);
      az_gl_color(side_color);
      glVertex2f(14, 4); glVertex2f(8, 11); glVertex2f(0, 14);
    } glEnd();
  } glPopMatrix();
}

void draw_core(const az_baddie_t *baddie, float flare, float frozen) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.75 - 0.75 * frozen, 0.25 + 0.5 * flare, 0.5); // reddish
    glVertex2d(0.5 * baddie->data->main_body.bounding_radius, 0);
    glColor3f(0.25 - 0.25 * frozen, 0.5 * flare,
              0.25 * frozen + 0.5 * flare); // dark red
    const double radius = baddie->data->main_body.bounding_radius;
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

void draw_pincers(const az_baddie_t *baddie, az_color_t center_color,
                  az_color_t side_color) {
  // Teeth:
  for (int i = 1; i <= 2; ++i) {
    glPushMatrix(); {
      const az_component_t *pincer = &baddie->components[i];
      glTranslated(pincer->position.x, pincer->position.y, 0);
      glRotated(AZ_RAD2DEG(pincer->angle), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        glColor3f(0.5, 0.5, 0.5);
        const GLfloat y = (i % 2 ? -4 : 4);
        for (GLfloat x = 25 - 3 * i; x > 5; x -= 6) {
          glVertex2d(x + 2, 0); glVertex2d(x, y); glVertex2d(x - 2, 0);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Pincers:
  for (int i = 1; i <= 2; ++i) {
    glPushMatrix(); {
      const az_component_t *pincer = &baddie->components[i];
      glTranslated(pincer->position.x, pincer->position.y, 0);
      glRotated(AZ_RAD2DEG(pincer->angle), 0, 0, 1);
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(center_color);
        glVertex2f(0, 0);
        az_gl_color(side_color);
        const az_polygon_t poly = baddie->data->components[i].polygon;
        for (int j = 0, k = poly.num_vertices; j >= 0; j = --k) {
          glVertex2d(poly.vertices[j].x, poly.vertices[j].y);
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
  draw_stalk(baddie, true, 1.0, center_color, side_color);
  draw_base(baddie, center_color, side_color);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, center_color, side_color);
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
  draw_stalk(baddie, false, 0.5, center_color, side_color);
  draw_base(baddie, center_color, side_color);
  draw_core(baddie, flare, frozen);
  draw_pincers(baddie, center_color, side_color);
}

/*===========================================================================*/
