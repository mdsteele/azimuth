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

#include "azimuth/view/baddie_kilofuge.h"

#include <assert.h>
#include <math.h>

#include <SDL_opengl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_eyeball(az_vector_t position, double angle, double radius,
                         float flare, float hurt) {
  glPushMatrix(); {
    az_gl_translated(position);
    az_gl_rotated(angle);
    // Eyeball:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(1.0f, (1.0f - 0.6f * hurt) * (1.0f - 0.8f * flare),
                (1.0f - 0.8f * hurt) * (1.0f - 0.8f * flare));
      glVertex2f(0, 0);
      glColor3f(0.5f, (0.5f - 0.3f * hurt) * (1.0f - 0.8f * flare),
                  (0.5f - 0.4f * hurt) * (1.0f - 0.8f * flare));
      for (int j = 0; j <= 360; j += 20) {
        glVertex2d(radius * cos(AZ_DEG2RAD(j)), radius * sin(AZ_DEG2RAD(j)));
      }
    } glEnd();
    // Pupil:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0, 0, 0); glVertex2d(0.7 * radius, 0);
      glColor4f(0, 0, 0, 0.7f);
      for (int j = 0; j <= 360; j += 30) {
        glVertex2d(0.2 * radius * cos(AZ_DEG2RAD(j)) + 0.7 * radius,
                   0.3 * radius * sin(AZ_DEG2RAD(j)));
      }
    } glEnd();
  } glPopMatrix();
}

static const struct {
  az_vector_t position;
  double radius;
  int angle_index;
} small_eyeballs[] = {
  {{ 97,   0}, 7.0, 0},
  {{120,  15}, 6.0, 1}, {{110,  35}, 5.0, 1},
  {{120, -15}, 6.0, 2}, {{110, -35}, 5.0, 2}
};

/*===========================================================================*/

void az_draw_bad_kilofuge(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_KILOFUGE);
  const float flare = baddie->armor_flare;
  const float hurt = 1.0 - baddie->health / baddie->data->max_health;
  const az_color_t inner =
    az_color3f(0.6f, 0.4f + 0.2f * hurt, 0.6f - 0.2f * hurt);
  const az_color_t outer =
    az_color3f(0.2f, 0.1f + 0.1f * hurt, 0.2f - 0.1f * hurt);
  // Legs:
  for (int i = 5; i < 11; ++i) {
    glPushMatrix(); {
      const az_component_t *leg = &baddie->components[i];
      az_gl_translated(leg->position);
      az_gl_rotated(leg->angle);
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(inner); glVertex2f(-50, -copysign(5, leg->position.y));
        az_gl_color(outer);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Main eyes:
  for (int i = 0; i < 3; ++i) {
    draw_eyeball(baddie->components[i].position, baddie->components[i].angle,
                 baddie->data->components[i].bounding_radius, flare, hurt);
  }
  // Body:
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer);
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int j = polygon.num_vertices - 1, k = 0;
         j < polygon.num_vertices; j = k++) {
      az_gl_vertex(polygon.vertices[j]);
    }
  } glEnd();
  // Small eyes:
  AZ_ARRAY_LOOP(eye, small_eyeballs) {
    draw_eyeball(eye->position, baddie->components[eye->angle_index].angle,
                 eye->radius, flare, hurt);
  }
  // Pincers:
  for (int i = 3; i < 5; ++i) {
    glPushMatrix(); {
      az_gl_translated(baddie->components[i].position);
      az_gl_rotated(baddie->components[i].angle);
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(inner);
        glVertex2f(50, (i == 3 ? 15 : -15));
        az_gl_color(outer);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
}

void az_draw_bad_ice_crystal(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_ICE_CRYSTAL);
  const float flare = baddie->armor_flare;
  const az_polygon_t polygon = baddie->data->main_body.polygon;
  for (int i = 0; i < polygon.num_vertices; ++i) {
    const int j = (i + 1) % polygon.num_vertices;
    if (i == 0) glColor4f(0.5f, 1.0f - flare, 1.0f - flare, 0.25f);
    else if (i == 2) glColor4f(0.5f * flare, 0.75f - 0.75f * flare,
                               0.75f - 0.75f * flare, 0.25f);
    else glColor4f(0.5f * flare, 1.0f - flare, 1.0f - flare, 0.25f);
    glBegin(GL_TRIANGLES); {
      az_gl_vertex(polygon.vertices[i]);
      az_gl_vertex(polygon.vertices[j]);
      glColor4f(flare, 1 - flare, 1 - 0.5 * flare, 0.75);
      glVertex2f(0, 0);
    } glEnd();
  }
}

/*===========================================================================*/
