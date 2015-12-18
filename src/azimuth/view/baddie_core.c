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

#include "azimuth/view/baddie_core.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

static az_vector_t weighted_avg(az_vector_t p1, az_vector_t p2, double w2) {
  return az_vadd(az_vmul(az_vsub(p2, p1), w2), p1);
}

/*===========================================================================*/

static void draw_piston(az_vector_t start, az_vector_t end) {
  const double segment_semilength = 15;
  const int num_segments = 5;
  glPushMatrix(); {
    az_gl_translated(start);
    az_gl_rotated(az_vtheta(az_vsub(end, start)));
    const double full_length = az_vdist(end, start);
    const double inner_length = full_length - 2 * segment_semilength;
    const double step = inner_length / (num_segments - 1);
    for (int i = 0; i < num_segments; ++i) {
      const double cx = full_length - segment_semilength - i * step;
      const double lx = cx - segment_semilength;
      const double rx = cx + segment_semilength + 2 * i;
      const double segment_radius = 3 + i;
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.1, 0.1, 0.1);
        glVertex2d(lx,  segment_radius); glVertex2d(rx,  segment_radius);
        glColor3f(0.2, 0.2, 0.2);
        glVertex2d(lx,               0); glVertex2d(rx,               0);
        glColor3f(0.1, 0.1, 0.1);
        glVertex2d(lx, -segment_radius); glVertex2d(rx, -segment_radius);
      } glEnd();
    }
  } glPopMatrix();
}

static void draw_cracks(double sx, double sy, double angle, double length) {
  az_random_seed_t seed = {1 + 4987298743 * sx, 1 + 373984471 * sy};
  az_draw_cracks(&seed, (az_vector_t){sx, sy}, angle, length);
}

/*===========================================================================*/

void az_draw_bad_zenith_core(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_ZENITH_CORE);
  const float flare = baddie->armor_flare;
  const float hurt = 1.0 - baddie->health / baddie->data->max_health;

  // Pistons:
  for (int i = 0; i < 8; ++i) {
    const az_component_t *component = &baddie->components[i];
    const az_vector_t end1 = {80, 15};
    const az_vector_t end2 = az_vreflect(end1, az_vpolar(1, AZ_DEG2RAD(22.5)));
    draw_piston(az_vpolar(15, i * AZ_DEG2RAD(45) + AZ_DEG2RAD(11.25)),
                az_vadd(az_vrotate(end1, component->angle),
                        component->position));
    draw_piston(az_vpolar(15, i * AZ_DEG2RAD(45) + AZ_DEG2RAD(33.75)),
                az_vadd(az_vrotate(end2, component->angle),
                        component->position));
  }

  const az_color_t outer = color3(0.15f + 0.25f * flare, 0.25f, 0.20f);
  const az_color_t inner = color3(0.40f + 0.40f * flare, 0.45f, 0.45f);

  // Diagonal struts:
  glBegin(GL_TRIANGLE_STRIP); {
    for (int i = 0; i <= 360; i += 90) {
      az_gl_color(outer);
      glVertex2d(81 * cos(AZ_DEG2RAD(i)), 81 * sin(AZ_DEG2RAD(i)));
      az_gl_color(inner);
      glVertex2d(89 * cos(AZ_DEG2RAD(i)), 89 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();

  // Main spokes:
  for (int i = 0; i < 360; i += 90) {
    glPushMatrix(); {
      az_gl_rotated(AZ_DEG2RAD(i));
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(outer); glVertex2f(10,  10); glVertex2f(80,  10);
        az_gl_color(inner); glVertex2f(10, -10); glVertex2f(80, -10);
      } glEnd();
    } glPopMatrix();
  }
  draw_cracks(30, 10, AZ_DEG2RAD(-90), 4 * hurt);
  draw_cracks(50, -10, AZ_DEG2RAD(90), 6 * hurt);
  draw_cracks(-10, 60, AZ_DEG2RAD(0), 5 * hurt);
  draw_cracks(10, 40, AZ_DEG2RAD(180), 5 * hurt);
  draw_cracks(-10, 22, AZ_DEG2RAD(0), 2 * hurt);
  draw_cracks(-30, 10, AZ_DEG2RAD(-90), 3 * hurt);
  draw_cracks(-55, -10, AZ_DEG2RAD(90), 4 * hurt);
  draw_cracks(-20, -10, AZ_DEG2RAD(90), 2 * hurt);
  draw_cracks(-10, -25, AZ_DEG2RAD(0), 5 * hurt);
  draw_cracks(10, -42, AZ_DEG2RAD(180), 4 * hurt);
  draw_cracks(-10, -61, AZ_DEG2RAD(0), 2 * hurt);

  // Outer rim:
  glBegin(GL_TRIANGLE_STRIP); {
    const double thickness = 10;
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      az_gl_color(outer);
      az_gl_vertex(polygon.vertices[i]);
      az_gl_color(inner);
      az_gl_vertex(az_vaddlen(polygon.vertices[i], -thickness));
    }
  } glEnd();

  // Central hub:
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
    for (int i = 0; i <= 360; i += 45) {
      glVertex2d(16 * cos(AZ_DEG2RAD(i)), 16 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();

  // Armor plating:
  const az_color_t outer_edge = color3(0.5, 0.5, 0.5);
  const az_color_t outer_mid = color3(0.7, 0.7, 0.7);
  const az_color_t inner_mid = color3(0.5, 0.5, 0.6);
  const az_color_t inner_edge = color3(0.3, 0.3, 0.3);
  for (int i = 0; i < 8; ++i) {
    const az_component_t *component = &baddie->components[i];
    const az_polygon_t polygon = baddie->data->components[i].polygon;
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      const az_vector_t o1 = polygon.vertices[0];
      const az_vector_t o2 = polygon.vertices[1];
      const az_vector_t i1 = polygon.vertices[3];
      const az_vector_t i2 = polygon.vertices[2];
      const az_vector_t m1 = weighted_avg(i1, o1, 0.8);
      const az_vector_t m2 = weighted_avg(i2, o2, 0.8);
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(outer_edge); az_gl_vertex(o1); az_gl_vertex(o2);
        az_gl_color(outer_mid);  az_gl_vertex(m1); az_gl_vertex(m2);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(inner_mid);  az_gl_vertex(m1); az_gl_vertex(m2);
        az_gl_color(inner_edge); az_gl_vertex(i1); az_gl_vertex(i2);
      } glEnd();
    } glPopMatrix();
  }
}

/*===========================================================================*/
