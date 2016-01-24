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

#include "azimuth/view/baddie_crawler.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_feet(az_color_t color1, az_color_t color2, az_clock_t clock,
                      int slowdown, GLfloat step_up, bool stopped) {
  glBegin(GL_QUADS); {
    const GLfloat offset =
      (stopped ? 0.0f : 0.8f * (az_clock_zigzag(5, slowdown, clock) - 2.0f));
    for (int i = 0; i < 4; ++i) {
      az_gl_color(color1);
      glVertex2f(0, 5); glVertex2f(0, -5);
      const GLfloat x = -20.0f + (i == 0 || i == 3 ? step_up : 0.0f);
      const GLfloat y = -12.0f + 8.0f * i + (2 * (i % 2) - 1) * offset;
      glVertex2f(x, y - 2);
      az_gl_color(color2);
      glVertex2f(x, y + 2);
    }
  } glEnd();
}

static void draw_normal_feet(float frozen, az_clock_t clock, bool stopped) {
  draw_feet(az_color3f(0.5f, 0.15f, 0.1f + 0.6f * frozen),
            az_color3f(0.2f, 0.1f, 0.4f + 0.6f * frozen),
            clock, 5, 2.0f, stopped);
}

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

/*===========================================================================*/

void az_draw_bad_cave_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_CAVE_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_normal_feet(frozen, clock, false);
  // Body:
  glPushMatrix(); {
    glTranslatef(-0.5f * az_clock_zigzag(5, 5, clock), 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.3f + 0.7f * flare, 0.2f, 0.4f + 0.6f * frozen);
      glVertex2f(-15.0f, az_clock_zigzag(9, 3, clock) - 4.0f);
      for (int i = -120; i <= 120; i += 5) {
        if (i % 3 == 0) {
          glColor3f(0.2f + 0.6f * flare, 0, 0.3f + 0.6f * frozen);
        } else glColor3f(0.4f, 0, 0.2f + 0.6f * frozen);
        const double rr = 1.0 +
          0.1 * (sin(AZ_DEG2RAD(i) * 2500) +
                 cos(AZ_DEG2RAD(i) * 777 *
                     (1 + az_clock_zigzag(7, 12, clock)))) +
          0.01 * az_clock_zigzag(10, 3, clock);
        glVertex2d(15 * rr * cos(AZ_DEG2RAD(i)) - 3,
                   17 * rr * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_spined_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SPINED_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_normal_feet(frozen, clock, (baddie->state == 3));
  // Body:
  glPushMatrix(); {
    glTranslatef((baddie->state == 3 ? -2.5f :
                  -0.5f * az_clock_zigzag(5, 5, clock)), 0, 0);
    for (int i = -82; i <= 82; i += 41) {
      glPushMatrix(); {
        glTranslatef(-12, 0, 0);
        glScalef(1, 0.85, 1);
        glRotatef(i, 0, 0, 1);
        glTranslatef((baddie->state == 3 ? 21 : 18), 0, 0);
        glScalef(0.7, 1, 1);
        draw_spine(flare, frozen);
      } glPopMatrix();
    }
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.2f + 0.8f * flare, 0.6f - 0.3f * flare,
                0.4f + 0.6f * frozen);
      glVertex2f(-13, 0);
      glColor3f(0.06 + 0.5f * flare, 0.24f - 0.1f * flare,
                0.12f + 0.5f * frozen);
      glVertex2f(-15, 0);
      for (int i = -120; i <= 120; i += 30) {
        glVertex2d(13 * cos(AZ_DEG2RAD(i)) - 7, 16 * sin(AZ_DEG2RAD(i)));
      }
      glVertex2f(-15, 0);
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_crab_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_CRAB_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_normal_feet(frozen, clock, (baddie->state == 3));
  // Body:
  glPushMatrix(); {
    glTranslatef((baddie->state == 3 ? -2.5f :
                  -0.25f * az_clock_zigzag(5, 5, clock)), 0, 0);
    const az_color_t inner =
      az_color3f(0.6f + 0.4f * flare - 0.4f * frozen, 0.4f - 0.2f * flare,
                 0.2f + 0.8f * frozen);
    const az_color_t outer =
      az_color3f(0.24f + 0.4f * flare - 0.15f * frozen, 0.12f - 0.05f * flare,
                 0.06f + 0.5f * frozen);
    // Antennae:
    glBegin(GL_LINE_STRIP); {
      az_gl_color(outer);
      glVertex2f(7, -9); glVertex2f(-3, 0); glVertex2f(7, 9);
    } glEnd();
    // Shell:
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner); glVertex2f(-13, 0); az_gl_color(outer);
      glVertex2f(-15, 0);
      for (int i = -120; i <= 120; i += 30) {
        glVertex2d(11 * cos(AZ_DEG2RAD(i)) - 7, 16 * sin(AZ_DEG2RAD(i)));
      }
      glVertex2f(-15, 0);
    } glEnd();
  } glPopMatrix();
  // Claws:
  for (int i = 0; i < 2; ++i) {
    glPushMatrix(); {
      az_gl_translated(baddie->components[i].position);
      az_gl_rotated(baddie->components[i].angle);
      glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6f - 0.4f * frozen, 0.3f, 0.3f + 0.7f * frozen);
        glVertex2f(-4, (i == 0 ? 3 : -3));
        glColor3f(0.24f - 0.15f * frozen, 0.1f, 0.1f + 0.5f * frozen);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
}

void az_draw_bad_ice_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_ICE_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_feet(az_color3f(0.5f + 0.5f * flare, 0.15f, 0.5f),
            az_color3f(0.2f + 0.2f * flare, 0.1f, 0.4f),
            clock, 6, 1.0f, false);
  // Body:
  glPushMatrix(); {
    glTranslatef(-0.2f * az_clock_zigzag(5, 5, clock), 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.5f + 0.5f * flare, 0.2, 0.4);
      glVertex2f(-15, 0);
      glColor3f(0.4f + 0.6f * flare, 0, 0.2);
      for (int i = -135; i <= 135; i += 5) {
        glVertex2d(13 * cos(AZ_DEG2RAD(i)) - 4, 14 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    // Ice shell:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0.35, 1, 1, 0.6);
      glVertex2f(-4, 0);
      glColor4f(0.06, 0.125, 0.2, 0.8);
      const az_component_data_t *component = &baddie->data->components[0];
      for (int i = 0, j = component->polygon.num_vertices;
           i >= 0; i = --j) {
        const az_vector_t vertex = component->polygon.vertices[i];
        glVertex2d(vertex.x, vertex.y);
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_fire_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_FIRE_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_normal_feet(frozen, clock, false);
  // Body:
  glPushMatrix(); {
    glTranslatef(-0.5f * az_clock_zigzag(5, 2, clock), 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.5f + 0.5f * flare, 0.2, 0.4);
      glVertex2f(-15, 0);
      glColor3f(0.4f + 0.6f * flare, 0, 0.2);
      for (int i = -135; i <= 135; i += 5) {
        glVertex2d(9 * cos(AZ_DEG2RAD(i)) - 4, 12 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    // Flames:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(1.0f, 0.3f + 0.5f * flare, 0.0f);
      glVertex2f(-14, 0);
      for (int i = -120; i <= 120; i += 5) {
        glColor4f(0.4f, 0.1f + 0.2f * flare, 0, 0.75f);
        const double rr = 1.0 +
          0.25 * (sin(AZ_DEG2RAD(i) * 2500) +
                  cos(AZ_DEG2RAD(i) * 777 *
                      (1 + 0.5 * az_clock_zigzag(14, 6, clock)))) +
          0.02 * az_clock_zigzag(10, 3, clock);
        glVertex2d(14 * rr * cos(AZ_DEG2RAD(i)) - 3,
                   (17 - fabs(i * 0.02)) * rr * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    // Yellow glow:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(1.0f, 0.9f, 0, 0.75f);
      glVertex2f(-9, 0);
      glColor4f(1.0f, 0.9f, 0, 0);
      const double rr = 0.9 + 0.06 * az_clock_zigzag(6, 8, clock);
      for (int i = 0; i <= 360; i += 15) {
        glVertex2d(11.0 * rr * cos(AZ_DEG2RAD(i)) - 3,
                   14.0 * rr * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_jungle_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_JUNGLE_CRAWLER);
  const float flare = baddie->armor_flare;
  draw_normal_feet(frozen, clock, false);
  // Body:
  glPushMatrix(); {
    glTranslatef(-0.5f * az_clock_zigzag(5, 5, clock), 0, 0);
    for (int i = -100; i <= 100; i += 20) {
      glPushMatrix(); {
        glTranslatef(-6, 0, 0);
        glRotatef(i, 0, 0, 1);
        glTranslatef(17, 0, 0);
        draw_spine(flare, frozen);
      } glPopMatrix();
    }
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.4f + 0.6f * flare, 0.2f, 0.2f + 0.6f * frozen);
      glVertex2f(-15.0f, az_clock_zigzag(9, 3, clock) - 4.0f);
      for (int i = -120; i <= 120; i += 5) {
        if (i % 3 == 0) {
          glColor3f(0.3f + 0.6f * flare, 0.2f, 0.6f * frozen);
        } else glColor3f(0.2f, 0.4f, 0.6f * frozen);
        const double rr = 1.0 +
          0.05 * (sin(AZ_DEG2RAD(i) * 2500) +
                  cos(AZ_DEG2RAD(i) * 777 *
                      (1 + az_clock_zigzag(7, 12, clock)))) +
          0.01 * az_clock_zigzag(10, 3, clock);
        glVertex2d(13 * rr * cos(AZ_DEG2RAD(i)) - 3,
                   16 * rr * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/
