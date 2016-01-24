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

static az_vector_t transform(const az_component_t *component, az_vector_t v) {
  return az_vadd(az_vrotate(v, component->angle), component->position);
}

static void avg_vertex(az_vector_t v1, az_vector_t v2) {
  az_gl_vertex(az_vmul(az_vadd(v1, v2), 0.5));
}

static void draw_fins(az_color_t inner, az_color_t outer, GLfloat fin_scale,
                      GLfloat y_base, GLfloat y_tip, az_clock_t clock) {
  for (int i = -1; i <= 1; i += 2) {
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner); glVertex2f(fin_scale, i * fin_scale * y_base);
      az_gl_color(outer); glVertex2f(8 * fin_scale, i * fin_scale * y_base);
      glVertex2f(-4 * fin_scale - az_clock_zigzag(4, 7, clock),
                 i * fin_scale * y_tip);
      glVertex2f(-2 * fin_scale, i * fin_scale * y_base);
    } glEnd();
  }
}

static void draw_fish(const az_baddie_t *baddie, az_color_t inner,
                      az_color_t outer, GLfloat fin_scale, az_clock_t clock) {
  assert(baddie->data->num_components == 2);
  const az_component_t *middle = &baddie->components[0];
  const az_component_t *tail = &baddie->components[1];
  const az_vector_t *hvertices = baddie->data->main_body.polygon.vertices;
  const az_vector_t *mvertices = baddie->data->components[0].polygon.vertices;
  const az_vector_t *tvertices = baddie->data->components[1].polygon.vertices;
  // Fins:
  draw_fins(inner, outer, fin_scale, 5, 12, clock);
  glPushMatrix(); {
    az_gl_translated(middle->position);
    az_gl_rotated(middle->angle);
    draw_fins(inner, outer, fin_scale, 3, 9, clock);
  } glPopMatrix();
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
    for (int i = 0; i < 7; ++i) az_gl_vertex(hvertices[i]);
  } glEnd();
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); az_gl_vertex(hvertices[6]);
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer); az_gl_vertex(transform(middle, mvertices[1]));
    az_gl_color(inner); az_gl_vertex(transform(middle, mvertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(middle, mvertices[2]));
    az_gl_color(inner); az_gl_vertex(middle->position);
    az_gl_color(outer); az_gl_vertex(transform(tail, tvertices[0]));
    az_gl_color(inner); az_gl_vertex(transform(middle, mvertices[3]));
    az_gl_color(outer); az_gl_vertex(transform(tail, tvertices[1]));
                        az_gl_vertex(transform(tail, tvertices[2]));
    az_gl_color(inner); az_gl_vertex(transform(middle, mvertices[3]));
    az_gl_color(outer); az_gl_vertex(transform(middle, mvertices[4]));
    az_gl_color(inner); az_gl_vertex(middle->position);
    az_gl_color(outer); az_gl_vertex(transform(middle, mvertices[5]));
    az_gl_color(inner); az_gl_vertex(transform(middle, mvertices[0]));
    az_gl_color(outer); az_gl_vertex(hvertices[0]);
    az_gl_color(inner); glVertex2f(0, 0);
  } glEnd();
}

static void draw_forcefiend_bone(const az_baddie_t *baddie, int idx,
                                 GLfloat cx) {
  const az_component_t *component = &baddie->components[idx];
  glPushMatrix(); {
    az_gl_translated(component->position);
    az_gl_rotated(component->angle);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6, 0.5, 0.5); glVertex2f(cx, 0);
      glColor3f(0.3, 0.25, 0.25);
      const az_polygon_t polygon = baddie->data->components[idx].polygon;
      for (int j = polygon.num_vertices - 1, k = 0;
           j < polygon.num_vertices; j = k++) {
        az_gl_vertex(polygon.vertices[j]);
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_forcefiend_arm(az_polygon_t polygon, az_color_t inner,
                                az_color_t outer) {
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner);
    glVertex2f(0, 0);
    az_gl_color(outer);
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
  const float hurt = 1.0 - baddie->health / baddie->data->max_health;
  const az_color_t inner = az_color3f(0.8f + 0.2f * flare - 0.3f * hurt,
                                      0.1f - 0.1f * flare + 0.6f * hurt,
                                      0.3f - 0.3f * flare);
  const az_color_t midst = az_color3f(0.6f + 0.1f * flare - 0.1f * hurt,
                                      0.1f - 0.1f * flare + 0.3f * hurt,
                                      0.3f - 0.3f * flare);
  const az_color_t outer = az_color3f(0.35f + 0.2f * flare - 0.1f * hurt,
                                      0.1f - 0.1f * flare + 0.1f * hurt,
                                      0.15f - 0.1f * flare);
  // Stinger:
  draw_forcefiend_bone(baddie, 11, 13);
  // Pincers:
  draw_forcefiend_bone(baddie, 0, 0);
  draw_forcefiend_bone(baddie, 1, 0);
  // Lower arms:
  for (int i = 3; i <= 6; i += 3) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      draw_forcefiend_arm(baddie->data->components[i].polygon, midst, outer);
    } glPopMatrix();
  }
  // Upper arms:
  for (int i = 2; i <= 5; i += 3) {
    const az_component_t *component = &baddie->components[i];
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(outer); glVertex2f(0,  9); glVertex2f(40,  4);
        az_gl_color(midst); glVertex2f(0,  0); glVertex2f(40,  0);
        az_gl_color(outer); glVertex2f(0, -9); glVertex2f(40, -4);
      } glEnd();
    } glPopMatrix();
  }
  // Claws:
  draw_forcefiend_bone(baddie, 4, 0);
  draw_forcefiend_bone(baddie, 7, 0);
  // Body:
  const az_component_t *segment1 = &baddie->components[8];
  const az_component_t *segment2 = &baddie->components[9];
  const az_component_t *segment3 = &baddie->components[10];
  const az_polygon_t poly0 = baddie->data->main_body.polygon;
  const az_polygon_t poly1 = baddie->data->components[8].polygon;
  const az_polygon_t poly2 = baddie->data->components[9].polygon;
  const az_polygon_t poly3 = baddie->data->components[10].polygon;
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
    for (int i = 0; i < 5; ++i) az_gl_vertex(poly0.vertices[i]);
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); az_gl_vertex(poly0.vertices[4]);
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer); avg_vertex(poly0.vertices[5],
                                   transform(segment1, poly1.vertices[1]));
    az_gl_color(midst); avg_vertex(poly0.vertices[6],
                                   transform(segment1, poly1.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment1, poly1.vertices[2]));
    az_gl_color(inner); az_gl_vertex(segment1->position);
    az_gl_color(outer); avg_vertex(transform(segment1, poly1.vertices[3]),
                                   transform(segment2, poly2.vertices[1]));
    az_gl_color(midst); avg_vertex(transform(segment1, poly1.vertices[4]),
                                   transform(segment2, poly2.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment2, poly2.vertices[2]));
    az_gl_color(inner); az_gl_vertex(segment2->position);
    az_gl_color(outer); avg_vertex(transform(segment2, poly2.vertices[3]),
                                   transform(segment3, poly3.vertices[1]));
    az_gl_color(midst); avg_vertex(transform(segment2, poly2.vertices[4]),
                                   transform(segment3, poly3.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment3, poly3.vertices[2]));
    az_gl_color(inner); az_gl_vertex(segment3->position);
    az_gl_color(outer); az_gl_vertex(transform(segment3, poly3.vertices[3]));
    az_gl_color(midst); az_gl_vertex(transform(segment3, poly3.vertices[4]));
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); az_gl_vertex(poly0.vertices[0]);
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer); avg_vertex(poly0.vertices[7],
                                   transform(segment1, poly1.vertices[7]));
    az_gl_color(midst); avg_vertex(poly0.vertices[6],
                                   transform(segment1, poly1.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment1, poly1.vertices[6]));
    az_gl_color(inner); az_gl_vertex(segment1->position);
    az_gl_color(outer); avg_vertex(transform(segment1, poly1.vertices[5]),
                                   transform(segment2, poly2.vertices[7]));
    az_gl_color(midst); avg_vertex(transform(segment1, poly1.vertices[4]),
                                   transform(segment2, poly2.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment2, poly2.vertices[6]));
    az_gl_color(inner); az_gl_vertex(segment2->position);
    az_gl_color(outer); avg_vertex(transform(segment2, poly2.vertices[5]),
                                   transform(segment3, poly3.vertices[7]));
    az_gl_color(midst); avg_vertex(transform(segment2, poly2.vertices[4]),
                                   transform(segment3, poly3.vertices[0]));
    az_gl_color(outer); az_gl_vertex(transform(segment3, poly3.vertices[6]));
    az_gl_color(inner); az_gl_vertex(segment3->position);
    az_gl_color(outer); az_gl_vertex(transform(segment3, poly3.vertices[5]));
    az_gl_color(midst); az_gl_vertex(transform(segment3, poly3.vertices[4]));
  } glEnd();
}

void az_draw_bad_force_egg(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_FORCE_EGG);
  const float flare = baddie->armor_flare;
  glPushMatrix(); {
    for (double rho = baddie->data->main_body.bounding_radius;
         rho > 0.0; rho -= 3.1) {
      const int num_steps = (int)round(rho * AZ_TWO_PI / 8.0);
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
    az_color3f(0.8f + 0.2f * flare, 0.1f - 0.1f * flare + 0.5f * frozen,
               0.3f - 0.3f * flare + 0.7f * frozen);
  const az_color_t outer =
    az_color3f(0.4f + 0.4f * flare, 0.1f - 0.1f * flare + 0.3f * frozen,
               0.15f - 0.1f * flare + 0.5f * frozen);
  draw_fish(baddie, inner, outer, 0.6, clock);
}

void az_draw_bad_small_fish(const az_baddie_t *baddie, float frozen,
                            az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SMALL_FISH);
  const float flare = baddie->armor_flare;
  const az_color_t inner =
    az_color3f(0.9f + 0.1f * flare, 0.5f * flare + 0.5f * frozen,
               0.4f - 0.4f * flare + 0.6f * frozen);
  const az_color_t outer =
    az_color3f(0.4f + 0.4f * flare, 0.3f * flare + 0.3f * frozen,
               0.3f - 0.2f * flare + 0.3f * frozen);
  draw_fish(baddie, inner, outer, 1.0, clock);
}

void az_draw_bad_large_fish(const az_baddie_t *baddie, float frozen,
                            az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_LARGE_FISH);
  const az_component_t *mid1 = &baddie->components[0];
  const az_component_t *mid2 = &baddie->components[1];
  const az_component_t *tail = &baddie->components[2];
  const az_vector_t *hvertices = baddie->data->main_body.polygon.vertices;
  const az_vector_t *m1vertices = baddie->data->components[0].polygon.vertices;
  const az_vector_t *m2vertices = baddie->data->components[1].polygon.vertices;
  const az_vector_t *tvertices = baddie->data->components[2].polygon.vertices;
  const float flare = baddie->armor_flare;
  const az_color_t inner =
    az_color3f(0.8f + 0.2f * flare, 0.1f - 0.1f * flare + 0.5f * frozen,
               0.3f - 0.3f * flare + 0.7f * frozen);
  const az_color_t outer =
    az_color3f(0.4f + 0.4f * flare, 0.3f * flare + 0.3f * frozen,
               0.3f - 0.2f * flare + 0.3f * frozen);
  const az_color_t tip = az_color3f(0.3, 0.25, 0.25);
  // Fins:
  glPushMatrix(); {
    glTranslatef(-5, 0, 0);
    draw_fins(inner, outer, 1.5, 5, 13, clock);
  } glPopMatrix();
  glPushMatrix(); {
    az_gl_translated(mid1->position);
    az_gl_rotated(mid1->angle);
    draw_fins(inner, outer, 1.5, 3, 9, clock);
  } glPopMatrix();
  // Pincers:
  for (int i = -1; i <= 1; i += 2) {
    glPushMatrix(); {
      glScalef(1, i, 1);
      glTranslatef(13, 1, 0);
      glRotatef(0.5f * az_clock_zigzag(30, 1, clock), 0, 0, 1);
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.6, 0.5, 0.5); glVertex2f(0, 0); az_gl_color(tip);
        glVertex2f(0, -1); glVertex2f(12, 0);
        glVertex2f(7, 3); glVertex2f(0, 5);
      } glEnd();
    } glPopMatrix();
  }
  // Head:
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner); glVertex2f(0, 0); az_gl_color(outer);
    for (int i = 1; i < 6; ++i) az_gl_vertex(hvertices[i]);
  } glEnd();
  // Body:
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer); az_gl_vertex(hvertices[5]);
    az_gl_color(inner); glVertex2f(0, 0);
    az_gl_color(outer); avg_vertex(hvertices[6],
                                   transform(mid1, m1vertices[1]));
    az_gl_color(inner); avg_vertex(hvertices[7],
                                   transform(mid1, m1vertices[0]));
    az_gl_color(outer); avg_vertex(transform(mid1, m1vertices[2]),
                                   transform(mid2, m2vertices[1]));
    az_gl_color(inner); avg_vertex(transform(mid1, m1vertices[3]),
                                   transform(mid2, m2vertices[0]));
    az_gl_color(outer); avg_vertex(transform(mid2, m2vertices[2]),
                                   transform(tail, tvertices[0]));
    az_gl_color(inner); avg_vertex(transform(mid2, m2vertices[3]),
                                   transform(tail, tvertices[3]));
    az_gl_color(tip); az_gl_vertex(transform(tail, tvertices[1]));
    az_gl_color(outer); avg_vertex(transform(mid2, m2vertices[4]),
                                   transform(tail, tvertices[2]));
    az_gl_color(inner); avg_vertex(transform(mid2, m2vertices[3]),
                                   transform(tail, tvertices[3]));
    az_gl_color(outer); avg_vertex(transform(mid1, m1vertices[4]),
                                   transform(mid2, m2vertices[5]));
    az_gl_color(inner); avg_vertex(transform(mid1, m1vertices[3]),
                                   transform(mid2, m2vertices[0]));
    az_gl_color(outer); avg_vertex(hvertices[0],
                                   transform(mid1, m1vertices[5]));
    az_gl_color(inner); avg_vertex(hvertices[7],
                                   transform(mid1, m1vertices[0]));
    az_gl_color(outer); az_gl_vertex(hvertices[1]);
    az_gl_color(inner); glVertex2f(0, 0);
  } glEnd();
}

/*===========================================================================*/
