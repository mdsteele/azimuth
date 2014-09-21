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

#include "azimuth/view/baddie_machine.h"

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

static void draw_sensor_lamp(GLfloat x_offset, bool lit) {
  glPushMatrix(); {
    glTranslatef(x_offset, 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      if (lit) {
        glColor3f(1, 0.2, 0.1);
      } else glColor3f(0.3, 0.3, 0.3);
      glVertex2f(0, 0);
      glColor3f(0.1, 0.1, 0.1);
      for (int i = 0; i <= 360; i += 20) {
        glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_QUAD_STRIP); {
      for (int i = 0; i <= 360; i += 20) {
        glColor3f(0.2, 0.2, 0.2);
        glVertex2d(5 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
        glColor3f(0.5, 0.5, 0.5);
        glVertex2d(3 * cos(AZ_DEG2RAD(i)), 3 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_bad_gun_sensor(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_GUN_SENSOR);
  assert(frozen == 0.0f);
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    if (baddie->state == 0) {
      glColor3f(0.75, 0.4 + 0.35 * flare, 0.5);
    } else glColor3f(0.25 + 0.5 * flare, 0.75, 0.5);
    glVertex2f(5, 0);
    if (baddie->state == 0) {
      glColor4f(0.25, 0.2 + 0.3 * flare, 0.5 * flare, 0.5);
    } else glColor4f(0.1 + 0.15 * flare, 0.25 + 0.25 * flare,
                     0.5 * flare, 0.5);
    for (int i = 0, j = baddie->data->main_body.polygon.num_vertices;
         i >= 0; i = --j) {
      az_gl_vertex(baddie->data->main_body.polygon.vertices[i]);
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(-7, 0);
    glColor3f(0.2, 0.2, 0.2);
    const az_component_data_t *component = &baddie->data->components[0];
    for (int i = 0, j = component->polygon.num_vertices; i >= 0; i = --j) {
      az_gl_vertex(component->polygon.vertices[i]);
    }
  } glEnd();
}

static void draw_beam_sensor(const az_baddie_t *baddie, bool lamp_lit) {
  assert(baddie->kind == AZ_BAD_BEAM_SENSOR ||
         baddie->kind == AZ_BAD_BEAM_SENSOR_INV);
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    if (baddie->state == 0) {
      glColor3f(0.75, 0.25 + 0.5 * flare, 0.5); // reddish
    } else glColor3f(0.25 + 0.5 * flare, 0.75, 0.5); // greenish
    glVertex2f(8, 0);
    if (baddie->state == 0) {
      glColor3f(0.25, 0.5 * flare, 0.5 * flare); // dark red
    } else glColor3f(0.25 * flare, 0.25 + 0.25 * flare, 0.5 * flare);
    const double radius = baddie->data->main_body.bounding_radius;
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(-5, 0);
    glColor3f(0.2, 0.2, 0.2);
    const az_component_data_t *component = &baddie->data->components[0];
    for (int i = 0, j = component->polygon.num_vertices; i >= 0; i = --j) {
      az_gl_vertex(component->polygon.vertices[i]);
    }
  } glEnd();
  draw_sensor_lamp(-6, lamp_lit);
}

void az_draw_bad_beam_sensor(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_BEAM_SENSOR);
  assert(frozen == 0.0f);
  draw_beam_sensor(baddie, (baddie->state != 0));
}

void az_draw_bad_beam_sensor_inv(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_BEAM_SENSOR_INV);
  assert(frozen == 0.0f);
  draw_beam_sensor(baddie, (baddie->state == 1 || az_clock_mod(2, 10, clock)));
}

void az_draw_bad_sensor_laser(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SENSOR_LASER);
  assert(frozen == 0.0f);
  assert(baddie->armor_flare == 0.0);
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(-5, 0);
    glColor3f(0.2, 0.2, 0.2);
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int i = 0, j = polygon.num_vertices; i >= 0; i = --j) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
  draw_sensor_lamp(-3, (baddie->state == 1 || az_clock_mod(2, 10, clock)));
}

/*===========================================================================*/

static void draw_ray(
    const az_baddie_t *baddie, az_clock_t clock, bool lamp,
    az_color_t dark, az_color_t medium, az_color_t light) {
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(dark); glVertex2f(0, 7); glVertex2f(15, 7);
    az_gl_color(light); glVertex2f(0, 0); glVertex2f(15, 0);
    az_gl_color(dark); glVertex2f(0, -7); glVertex2f(15, -7);
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(dark); glVertex2f(-10, 20); glVertex2f(0, 15);
    az_gl_color(light); glVertex2f(-10, 8); glVertex2f(0, 5);
    az_gl_color(dark); glVertex2f(-10, -20); glVertex2f(0, -15);
  } glEnd();
  glPushMatrix(); {
    glTranslatef(2, -7, 0);
    glBegin(GL_TRIANGLE_FAN); {
      if (lamp) glColor3f(1, 0.2, 0.1);
      else glColor3f(0.3, 0.3, 0.3);
      glVertex2f(0, 0);
      glColor3f(0.1, 0.1, 0.1);
      for (int i = 0; i <= 360; i += 20) {
        glVertex2d(5 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_QUAD_STRIP); {
      for (int i = 0; i <= 360; i += 20) {
        az_gl_color(dark);
        glVertex2d(7 * cos(AZ_DEG2RAD(i)), 7 * sin(AZ_DEG2RAD(i)));
        az_gl_color(medium);
        glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_heat_ray(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_HEAT_RAY);
  assert(frozen == 0.0f);
  const float flare = baddie->armor_flare;
  draw_ray(baddie, clock, baddie->state == 0 ||
           (baddie->cooldown <= 1.0 && az_clock_mod(2, 4, clock)),
           color3(0.35 + 0.25 * flare, 0.25, 0.25),
           color3(0.55 + 0.25 * flare, 0.45, 0.45),
           color3(0.85 + 0.15 * flare, 0.65, 0.65));
}

void az_draw_bad_death_ray(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_DEATH_RAY);
  assert(frozen == 0.0f);
  const float flare = baddie->armor_flare;
  draw_ray(baddie, clock, baddie->state == 1 ||
           (baddie->state == 0 && az_clock_mod(2, 10, clock)),
           color3(0.2 + 0.25 * flare, 0.2, 0.1),
           color3(0.3 + 0.25 * flare, 0.3, 0.2),
           color3(0.4 + 0.15 * flare, 0.4, 0.3));
}

void az_draw_bad_boss_door(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_BOSS_DOOR);
  assert(frozen == 0.0f);
  const float flare = baddie->armor_flare;
  // Eye:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(1.0f, 0.8f - 0.7f * flare, 0.6f - 0.5f * flare);
    glVertex2f(8, 8);
    glColor3f(0.8f, 0.4f - 0.3f * flare, 0.1f);
    const double radius = baddie->data->components[0].bounding_radius;
    for (int i = 0; i <= 360; i += 10) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glPushMatrix(); {
    az_gl_rotated(baddie->components[0].angle);
    glTranslatef(15, 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(baddie->param, 0, 0.25, 1);
      glVertex2f(0, 0);
      glColor4f(baddie->param, 0, 0.25, 0.6);
      for (int i = 0; i <= 360; i += 10) {
        glVertex2d(2.5 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
  // Eyelids:
  for (int i = 1; i <= 2; ++i) {
    glPushMatrix(); {
      az_gl_rotated(baddie->components[i].angle);
      glBegin(GL_TRIANGLE_FAN); {
        const az_polygon_t poly = baddie->data->components[i].polygon;
        glColor3f(0.75, 0.75, 0.75);
        az_gl_vertex(poly.vertices[0]);
        glColor3f(0.25, 0.25, 0.25);
        for (int j = 1; j < poly.num_vertices; ++j) {
          az_gl_vertex(poly.vertices[j]);
        }
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.2, 0.2); glVertex2f(22,  0); glVertex2f(22,  1);
        glColor3f(0.4, 0.4, 0.4); glVertex2f( 0,  0); glVertex2f( 2,  1);
        glColor3f(0.2, 0.2, 0.2); glVertex2f( 0, 22); glVertex2f( 1, 22);
      } glEnd();
    } glPopMatrix();
  }
  // Body:
  glBegin(GL_TRIANGLES); {
    glColor3f(0.15, 0.15, 0.15);
    glVertex2f(4, 28); glVertex2f(2, 32); glVertex2f(7, 32.5);
    glVertex2f(4, -28); glVertex2f(2, -32); glVertex2f(7, -32.5);
    glVertex2f(-8, 43); glVertex2f(-12, 45); glVertex2f(-7.5, 49);
    glVertex2f(-8, -43); glVertex2f(-12, -45); glVertex2f(-7.5, -49);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    const az_polygon_t poly = baddie->data->main_body.polygon;
    const int half = poly.num_vertices / 2;
    glColor3f(0.6, 0.6, 0.6);
    glVertex2f(-20, 0);
    glColor3f(0.2, 0.2, 0.2);
    for (int i = 0; i < half; ++i) {
      az_gl_vertex(poly.vertices[i]);
    }
    glColor3f(0.4, 0.4, 0.4);
    glVertex2f(7, 0);
    glColor3f(0.2, 0.2, 0.2);
    for (int i = half; i < poly.num_vertices; ++i) {
      az_gl_vertex(poly.vertices[i]);
    }
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.2, 0.2, 0.2); glVertex2f(4,  23); glVertex2f(8,  22);
    glColor3f(0.6, 0.6, 0.6); glVertex2f(4,   0); glVertex2f(8,   0);
    glColor3f(0.2, 0.2, 0.2); glVertex2f(4, -23); glVertex2f(8, -22);
  } glEnd();
}

/*===========================================================================*/
