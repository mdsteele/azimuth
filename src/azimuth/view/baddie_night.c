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

#include "azimuth/view/baddie_night.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"

/*===========================================================================*/

static void gl_matrix_wobble(double param) {
  if (0.0 < param && param < 1.0) {
    const double wobble_intensity = sin(AZ_PI * param);
    const double wobble_theta = AZ_TWO_PI * param;
    const GLfloat xy = 0.3 * wobble_intensity * cos(3.0 * wobble_theta);
    const GLfloat yx = 0.1 * wobble_intensity * sin(4.0 * wobble_theta);
    GLfloat matrix[16] = {
      1, xy, 0, 0,
      yx, 1, 0, 0,
      0,  0, 1, 0,
      0,  0, 0, 1};
    glMultMatrixf(matrix);
  }
}

static void draw_nightbug_body(GLfloat green, double invis, float flare,
                               float frozen, az_clock_t clock) {
  assert(0.0 <= invis && invis <= 1.0);
  // Tail:
  glBegin(GL_TRIANGLES); {
    glColor4f(0.25, 0.12, frozen, invis); // dark brown
    glVertex2f(-16, 0);
    glColor4f(0.5, 0.2, frozen, invis * invis); // reddish-brown
    glVertex2f(12, 6);
    glVertex2f(12, -6);
  } glEnd();
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(0.8, 0.4, 0.1 + 0.9 * frozen, invis); // light red-brown
    glVertex2f(10, 0);
    glColor4f(0.5, 0.2, frozen, invis * invis); // reddish-brown
    for (int i = -90; i <= 90; i += 30) {
      glVertex2d(10 + 7 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  // Body:
  glPushMatrix(); {
    for (int i = 0; i < 2; ++i) {
      if (i == 1) glScaled(1, -1, 1);
      // Legs:
      glBegin(GL_TRIANGLES); {
        glColor4f(0.25, 0.12, 0.5 * frozen, invis); // dark brown
        const GLfloat zig = 0.5f * az_clock_zigzag(8, 3, clock) - 2.0f;
        for (int j = 0; j < 3; ++j) {
          glVertex2f(12 - 7 * j, 7 - j);
          glVertex2f(5 - 7 * j, 7 - j);
          glVertex2f(3 - 7 * j + ((j + i) % 2 ? zig : -zig), 15 - j);
        }
      } glEnd();
      // Shell:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0.75 + 0.25 * flare - 0.75 * frozen, green, frozen,
                  fmax(0.08, invis)); // yellow-brown
        glVertex2f(6, 4);
        glColor4f(0.4 + 0.4 * flare - 0.4 * frozen, 0.2, frozen,
                  fmax(0.08, invis * invis * invis)); // brown
        const GLfloat zig = 0.3f * az_clock_zigzag(5, 2, clock);
        glVertex2f(10, 0.25); glVertex2f(13, 3);
        glVertex2f(12, 7); glVertex2f(10, 10);
        glVertex2f(-5, 8 + zig); glVertex2f(-12, 4 + zig);
        glVertex2f(-10, 0.5 + zig); glVertex2f(10, 0.25);
      } glEnd();
    }
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_bad_nightbug(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_NIGHTBUG);
  const float flare = baddie->armor_flare;
  const double invis = fmax(fmax(baddie->param, flare), frozen);
  glPushMatrix(); {
    gl_matrix_wobble(baddie->param);
    draw_nightbug_body(0.5f, invis, flare, frozen, clock);
  } glPopMatrix();
}

void az_draw_bad_nightshade(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_NIGHTSHADE);
  const float flare = baddie->armor_flare;
  const double invis = fmax(fmax(baddie->param, flare), frozen);
  glPushMatrix(); {
    gl_matrix_wobble(baddie->param);
    // Mandibles:
    for (int i = 0; i < baddie->data->num_components; ++i) {
      const az_component_t *component = &baddie->components[i];
      glPushMatrix(); {
        glTranslated(component->position.x, component->position.y, 0);
        glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          const az_polygon_t polygon = baddie->data->components[i].polygon;
          glColor4f(0.5f + 0.5f * flare - 0.5f * frozen, 0.3, frozen, invis);
          glVertex2d(polygon.vertices[0].x, polygon.vertices[0].y);
          glVertex2d(polygon.vertices[1].x, polygon.vertices[1].y);
          glColor4f(0.4f + 0.4f * flare - 0.4f * frozen, 0.2,
                    0.6f * frozen, invis * invis);
          for (int j = 2; j < polygon.num_vertices; ++j) {
            glVertex2d(polygon.vertices[j].x, polygon.vertices[j].y);
          }
        } glEnd();
      } glPopMatrix();
    }
    glScalef(1.1, 1.1, 1);
    draw_nightbug_body(0.25f, invis, flare, frozen, clock);
  } glPopMatrix();
}

/*===========================================================================*/
