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

#include "azimuth/view/baddie_forcefiend.h"

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

static void draw_fish(const az_baddie_t *baddie, az_color_t inner,
                      az_color_t outer, GLfloat fin_scale, az_clock_t clock) {
  const az_component_t *middle = &baddie->components[0];
  const az_component_t *tail = &baddie->components[1];
  const az_vector_t *hvertices = baddie->data->main_body.polygon.vertices;
  const az_vector_t *mvertices =
    baddie->data->components[0].polygon.vertices;
  const az_vector_t *tvertices =
    baddie->data->components[1].polygon.vertices;
  // Fins:
  for (int i = -1; i <= 1; i += 2) {
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner); glVertex2f(fin_scale, 5 * i * fin_scale);
      az_gl_color(outer); glVertex2f(8 * fin_scale, 5 * i * fin_scale);
      glVertex2f(-4 * fin_scale - az_clock_zigzag(4, 7, clock),
                 12 * i * fin_scale);
      glVertex2f(-2 * fin_scale, 5 * i * fin_scale);
    } glEnd();
  }
  glPushMatrix(); {
    az_gl_translated(middle->position);
    az_gl_rotated(middle->angle);
    for (int i = -1; i <= 1; i += 2) {
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(inner); glVertex2f(fin_scale, 3 * i * fin_scale);
        az_gl_color(outer); glVertex2f(8 * fin_scale, 3 * i * fin_scale);
        glVertex2f(-4 * fin_scale - az_clock_zigzag(4, 7, clock),
                   9 * i * fin_scale);
        glVertex2f(-2 * fin_scale, 3 * i * fin_scale);
      } glEnd();
    }
  } glPopMatrix();
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner);
    glVertex2f(0, 0);
    az_gl_color(outer);
    for (int i = 0; i < 7; ++i) az_gl_vertex(hvertices[i]);
  } glEnd();
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); az_gl_vertex(hvertices[6]);
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[1], middle->angle),
                         middle->position));
    az_gl_color(inner);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[0], middle->angle),
                         middle->position));
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[2], middle->angle),
                         middle->position));
    az_gl_color(inner); az_gl_vertex(middle->position);
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(tvertices[0], tail->angle),
                         tail->position));
    az_gl_color(inner);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[3], middle->angle),
                         middle->position));
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(tvertices[1], tail->angle),
                         tail->position));
    az_gl_vertex(az_vadd(az_vrotate(tvertices[2], tail->angle),
                         tail->position));
    az_gl_color(inner);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[3], middle->angle),
                         middle->position));
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[4], middle->angle),
                         middle->position));
    az_gl_color(inner);
    az_gl_vertex(middle->position);
    az_gl_color(outer);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[5], middle->angle),
                         middle->position));
    az_gl_color(inner);
    az_gl_vertex(az_vadd(az_vrotate(mvertices[0], middle->angle),
                         middle->position));
    az_gl_color(outer); az_gl_vertex(hvertices[0]);
    az_gl_color(inner); glVertex2f(0, 0);
  } glEnd();
}

static void draw_forcefiend_segment(az_polygon_t polygon, float flare) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.8f + 0.2f * flare, 0.1f - 0.1f * flare, 0.3f - 0.3f * flare);
    glVertex2f(0, 0);
    glColor3f(0.4f + 0.4f * flare, 0.1f - 0.1f * flare, 0.15f - 0.1f * flare);
    for (int j = polygon.num_vertices - 1, k = 0;
         j < polygon.num_vertices; j = k++) {
      az_gl_vertex(polygon.vertices[j]);
    }
  } glEnd();
}

/*===========================================================================*/

void az_draw_bad_forcefiend(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_FORCEFIEND);
  const float flare = baddie->armor_flare;
  // Stinger:
  glPushMatrix(); {
    const az_component_t *component = &baddie->components[11];
    az_gl_translated(component->position);
    az_gl_rotated(component->angle);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6, 0.5, 0.5); glVertex2f(16, 0);
      glColor3f(0.3, 0.25, 0.25);
      const az_polygon_t polygon = baddie->data->components[11].polygon;
      for (int i = 0; i < polygon.num_vertices; ++i) {
        az_gl_vertex(polygon.vertices[i]);
      }
    } glEnd();
  } glPopMatrix();
  // Pincers:
  for (int i = 0; i < 2; ++i) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.6, 0.5, 0.5); glVertex2f(0, 0);
        glColor3f(0.3, 0.25, 0.25);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Claws:
  for (int i = 4; i <= 7; i += 3) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.6, 0.5, 0.5); glVertex2f(0, 0);
        glColor3f(0.3, 0.25, 0.25);
        const az_polygon_t polygon = baddie->data->components[i].polygon;
        for (int j = polygon.num_vertices - 1, k = 0;
             j < polygon.num_vertices; j = k++) {
          az_gl_vertex(polygon.vertices[j]);
        }
      } glEnd();
    } glPopMatrix();
  }
  // Lower arms:
  for (int i = 3; i <= 6; i += 3) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      draw_forcefiend_segment(baddie->data->components[i].polygon, flare);
    } glPopMatrix();
  }
  // Upper arms:
  for (int i = 2; i <= 5; i += 3) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      draw_forcefiend_segment(baddie->data->components[i].polygon, flare);
    } glPopMatrix();
  }
  // Body:
  for (int i = 10; i >= 8; --i) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      draw_forcefiend_segment(baddie->data->components[i].polygon, flare);
    } glPopMatrix();
  }
  draw_forcefiend_segment(baddie->data->main_body.polygon, flare);
}

void az_draw_bad_force_egg(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_FORCE_EGG);
  const float flare = baddie->armor_flare;
  glPushMatrix(); {
    for (double rho = baddie->data->main_body.bounding_radius;
         rho > 0.0; rho -= 3.1) {
      const int num_steps = round(rho * AZ_TWO_PI / 8.0);
      const GLfloat step = 360.0 / num_steps;
      glRotatef(0.5f * step, 0, 0, 1);
      for (int i = 0; i < num_steps; ++i) {
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.66f + 0.34f * flare, 0.56f - 0.4f * flare,
                    0.4f - 0.3f * flare);
          glVertex2f(rho - 3, 0);
          glColor3f(0.2, 0.2, 0.2);
          glVertex2f(rho - 4, -4); glVertex2f(rho - 2, -5);
          glVertex2f(rho + 3,  0); glVertex2f(rho - 2,  5);
          glVertex2f(rho - 4,  4);
        } glEnd();
        glRotatef(step, 0, 0, 1);
      }
    }
  } glPopMatrix();
}

void az_draw_bad_forceling(const az_baddie_t *baddie, float frozen,
                           az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_FORCELING);
  const float flare = baddie->armor_flare;
  const az_color_t inner =
    color3(0.8f + 0.2f * flare, 0.1f - 0.1f * flare + 0.5f * frozen,
           0.3f - 0.3f * flare + 0.7f * frozen);
  const az_color_t outer =
    color3(0.4f + 0.4f * flare, 0.1f - 0.1f * flare + 0.3f * frozen,
           0.15f - 0.1f * flare + 0.5f * frozen);
  draw_fish(baddie, inner, outer, 0.6, clock);
}

void az_draw_bad_small_fish(const az_baddie_t *baddie, float frozen,
                            az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SMALL_FISH);
  const float flare = baddie->armor_flare;
  const az_color_t inner =
    color3(0.9f + 0.1f * flare, 0.5f * flare + 0.5f * frozen,
           0.4f - 0.4f * flare + 0.6f * frozen);
  const az_color_t outer =
    color3(0.4f + 0.4f * flare, 0.3f * flare + 0.3f * frozen,
           0.3f - 0.2f * flare + 0.3f * frozen);
  draw_fish(baddie, inner, outer, 1.0, clock);
}

/*===========================================================================*/
