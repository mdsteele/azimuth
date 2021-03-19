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

#include "azimuth/view/baddie_clam.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <SDL_opengl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

void az_draw_bad_clam(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_CLAM);
  const float flare = baddie->armor_flare;
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
  for (int i = 0; i < baddie->data->num_components; ++i) {
    glPushMatrix(); {
      const az_component_t *component = &baddie->components[i];
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      az_polygon_t polygon = baddie->data->components[i].polygon;
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 0.25, 1);
        glVertex2f(0, 0);
        glColor3f(0.2 + 0.4 * flare, 0.2 - 0.2 * flare, 0.2 - 0.2 * flare);
        for (int j = 0; j < polygon.num_vertices; ++j) {
          glVertex2d(polygon.vertices[j].x, polygon.vertices[j].y);
        }
      } glEnd();
      glBegin(GL_QUADS); {
        const int n = polygon.num_vertices;
        glVertex2d(polygon.vertices[0].x, polygon.vertices[0].y);
        glVertex2d(polygon.vertices[n - 1].x, polygon.vertices[n - 1].y);
        glColor4f(0.25, 0, 0.5, 0);
        glVertex2d(polygon.vertices[n - 2].x, polygon.vertices[n - 2].y);
        glVertex2d(polygon.vertices[1].x, polygon.vertices[1].y);
      } glEnd();
    } glPopMatrix();
  }
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(0.5, 0.4, 0.3, 0);
    glVertex2f(-4, 0);
    for (int i = 135; i <= 225; i += 15) {
      if (i == 225) glColor4f(0.5, 0.4, 0.3, 0);
      glVertex2d(15 * cos(AZ_DEG2RAD(i)), 13 * sin(AZ_DEG2RAD(i)));
      if (i == 135) glColor3f(0.2, 0.15, 0.3);
    }
  } glEnd();
}

void az_draw_bad_grabber_plant(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_GRABBER_PLANT);
  const float flare = baddie->armor_flare;
  // Tongue:
  glPushMatrix(); {
    const az_component_t *tongue = &baddie->components[9];
    const GLfloat length = az_vnorm(tongue->position);
    const GLfloat width = 3.0 - length / 300.0;
    const az_color_t inner =
      az_color3f(0.5f + 0.3f * flare - 0.5f * frozen, 0.2f + 0.4f * frozen,
                 0.35f - 0.2f * flare + 0.4f * frozen);
    const az_color_t outer =
      az_color3f(0.2f + 0.1f * flare - 0.2f * frozen,
                 0.1f * flare + 0.3f * frozen, 0.3f * frozen);
    az_gl_rotated(tongue->angle);
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(outer); glVertex2f(0, -width); glVertex2f(length, -width);
      az_gl_color(inner); glVertex2f(0,      0); glVertex2f(length,      0);
      az_gl_color(outer); glVertex2f(0,  width); glVertex2f(length,  width);
    } glEnd();
    glTranslatef(length, 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
      for (int i = -135; i <= 135; i += 15) {
        glVertex2d(6 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
  // Core:
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
  // Teeth:
  for (int i = 0; i < 2; ++i) {
    glPushMatrix(); {
      const az_component_t *jaw = &baddie->components[i];
      az_gl_translated(jaw->position);
      az_gl_rotated(jaw->angle);
      glBegin(GL_TRIANGLES); {
        glColor3f(0.5, 0.5, 0.5);
        const GLfloat y = (i % 2 ? 2 : -2);
        for (GLfloat x = 18 - i; x > 0; x -= 5) {
          glVertex2f(x + 1.5f, 0); glVertex2f(x, y); glVertex2f(x - 1.5f, 0);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Jaws:
  for (int i = 0; i < 2; ++i) {
    glPushMatrix(); {
      const az_component_t *component = &baddie->components[i];
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      const az_polygon_t polygon = baddie->data->components[i].polygon;
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5f - 0.5f * frozen, 0.75, 0.25f + 0.75f * frozen);
        glVertex2f(0, 0);
        glColor3f(0.1f + 0.4f * flare, 0.3f - 0.3f * flare + 0.3f * frozen,
                  0.2f - 0.2f * flare + 0.5f * frozen);
        for (int j = 0; j < polygon.num_vertices; ++j) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_vertex(polygon.vertices[0]);
        az_gl_vertex(polygon.vertices[polygon.num_vertices - 1]);
        glColor4f(0.25, 0, 0.5, 0);
        az_gl_vertex(polygon.vertices[1]);
        az_gl_vertex(polygon.vertices[polygon.num_vertices - 2]);
      } glEnd();
    } glPopMatrix();
  }
  // Base:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.5f - 0.5f * frozen, 0.75, 0.25f + 0.75f * frozen);
    glVertex2f(-22, 0);
    glColor3f(0.1f + 0.4f * flare, 0.3f - 0.3f * flare + 0.3f * frozen,
              0.2f - 0.2f * flare + 0.5f * frozen);
    const az_polygon_t polygon = baddie->data->components[2].polygon;
    for (int j = 0; j < polygon.num_vertices; ++j) {
      az_gl_vertex(polygon.vertices[j]);
    }
  } glEnd();
}

/*===========================================================================*/
