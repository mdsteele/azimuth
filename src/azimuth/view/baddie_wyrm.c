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

#include "azimuth/view/baddie_wyrm.h"

#include <math.h>

#include <SDL_opengl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_rockwyrm_segment(const az_baddie_t *baddie, int i) {
  const float hurt = 1 - baddie->health / baddie->data->max_health;
  const bool last = i == baddie->data->num_components - 1;
  const az_component_data_t *data = &baddie->data->components[i];
  const az_component_t *component = &baddie->components[i];
  glPushMatrix(); {
    az_gl_translated(component->position);
    az_gl_rotated(component->angle);
    // Spikes:
    const int limit = (last ? 7 : 5);
    for (int j = -limit; j <= limit; j += 2) {
      if (j == -1 || j == 1) continue;
      glPushMatrix(); {
        glRotated(j * 22.5, 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          const float radius = data->bounding_radius;
          glColor3f(0.75, 1, 0.25);
          glVertex2f(radius * 0.9, 0);
          glColor3f(0.25, 0.35, 0.125);
          glVertex2f(radius * 0.9, -4);
          glVertex2f(radius * 1.25, 0);
          glVertex2f(radius * 0.9, 4);
        } glEnd();
      } glPopMatrix();
    }
    // Body segment:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.75, 1.0f - hurt, 0.25 + 0.1f * hurt);
      glVertex2f(0, 0);
      const double radius = data->bounding_radius * 1.05;
      for (int j = 0; j <= 360; j += 30) {
        if (j % (last ? 360 : 180) == 0) {
          glColor3f(0.5f, 0.75f - 0.7f * hurt, 0.15f + 0.1f * hurt);
        } else {
          glColor3f(0.25, 0.35 - 0.3f * hurt, 0.125f);
        }
        glVertex2d(radius * cos(AZ_DEG2RAD(j)), radius * sin(AZ_DEG2RAD(j)));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_rockwyrm(const az_baddie_t *baddie) {
  const float flare = baddie->armor_flare;
  // Tail:
  for (int i = baddie->data->num_components - 1; i > 2; --i) {
    draw_rockwyrm_segment(baddie, i);
  }
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(1, 0.5f - 0.5f * flare, 0.1);
    glVertex2f(0, 0);
    glColor3f(0.5f, 0.25f - 0.25f * flare, 0.05);
    const double radius = baddie->data->main_body.bounding_radius;
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  // Jaws:
  for (int i = 0; i < 2; ++i) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0, 1);
        glVertex2f(0, 0);
        glColor3f(0.5f, 0, 0.05);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Neck:
  draw_rockwyrm_segment(baddie, 2);
}

void az_draw_bad_wyrm_egg(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    const double radius = baddie->data->main_body.bounding_radius;
    if (baddie->state != 1 || frozen > 0.0f ||
        (baddie->cooldown < 1.0 ? az_clock_mod(2, 4, clock) :
         az_clock_mod(2, 8, clock))) {
      glColor3f(0.6f + 0.4f * flare, 0.9f - 0.4f * flare - 0.4f * frozen,
                0.9f - 0.5f * flare);
    } else glColor3f(0, 1, 0.25);
    glVertex2d(0.35 * radius, 0);
    glColor3f(0.3f, 0.4f - 0.2f * flare - 0.2f * frozen, 0.4f - 0.2f * flare);
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

void az_draw_bad_wyrmling(const az_baddie_t *baddie, float frozen) {
  const float flare = baddie->armor_flare;
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(1, 0.5f - 0.5f * flare, 0.1);
    glVertex2f(0, 0);
    glColor3f(0.5f, 0.25f - 0.25f * flare, 0.05);
    const double radius = baddie->data->main_body.bounding_radius;
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  // Neck/tail:
  for (int i = baddie->data->num_components - 1; i >= 0; --i) {
    const bool last = i == baddie->data->num_components - 1;
    const az_component_data_t *data = &baddie->data->components[i];
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 1, 0.25);
        glVertex2f(0, 0);
        const double radius = data->bounding_radius * 1.05;
        for (int j = 0; j <= 360; j += 30) {
          if (j % (last ? 360 : 180) == 0) glColor3f(0.5, 0.75, 0.15);
          else glColor3f(0.25, 0.35, 0.125);
          glVertex2d(radius * cos(AZ_DEG2RAD(j)), radius * sin(AZ_DEG2RAD(j)));
        }
      } glEnd();
    } glPopMatrix();
  }
}

/*===========================================================================*/
