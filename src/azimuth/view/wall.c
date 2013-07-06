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

#include "azimuth/view/wall.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/space.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static inline void az_gl_vertex(az_vector_t v) {
  glVertex2d(v.x, v.y);
}

static void draw_bezel(double bezel, bool alt, az_color_t color1,
                       az_color_t color2, az_polygon_t polygon) {
  // Draw background color:
  if (color2.a != 0) {
    glColor4ub(color2.r, color2.g, color2.b, color2.a);
    glBegin(GL_TRIANGLE_FAN); {
      for (int i = 0; i < polygon.num_vertices; ++i) {
        glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
      }
    } glEnd();
  }
  // Calculate bezel:
  const int n = polygon.num_vertices;
  assert(n >= 3);
  // Temporary table of ancillary info for each vertex:
  struct {
    az_vector_t unit;
    double length;
    bool done;
  } vinfo[n];
  // Populate vinfo table:
  for (int i = 0; i < n; ++i) {
    const az_vector_t a = polygon.vertices[az_modulo(i - 1, n)];
    const az_vector_t b = polygon.vertices[i];
    const az_vector_t c = polygon.vertices[az_modulo(i + 1, n)];
    const double ta = az_vtheta(az_vsub(a, b));
    const double tc = az_vtheta(az_vsub(c, b));
    const double tt = 0.5 * az_mod2pi_nonneg(ta - tc);
    assert(0.0 < tt && tt < AZ_TWO_PI);
    vinfo[i].unit = az_vpolar(1, tc + tt);
    vinfo[i].length = (alt ? bezel : bezel / sin(tt));
    vinfo[i].done = false;
  }
  // Refine vinfo table, by repeatedly checking if the bezline of any vertices
  // will intersect any other bezline, and if so, stopping it short.
  for (int iteration = 0; iteration < n; ++iteration) {
    double best_time = INFINITY;
    int index_of_best = -1;
    double length_for_best = 0.0;
    for (int i = 0; i < n; ++i) {
      if (vinfo[i].done) continue;
      const az_vector_t vi = polygon.vertices[i];
      for (int k = 0; k < n; ++k) {
        if (k == i) continue;
        const az_vector_t vk = polygon.vertices[k];
        az_vector_t intersect;
        if (az_ray_hits_line_segment(
                vk, az_vadd(vk, az_vmul(vinfo[k].unit, vinfo[k].length + .01)),
                vi, az_vmul(vinfo[i].unit, vinfo[i].length),
                &intersect, NULL)) {
          const double i_length = az_vdist(intersect, vi);
          const double k_length = az_vdist(intersect, vk);
          const double time = fmax(i_length, k_length);
          if (time < best_time) {
            best_time = time;
            length_for_best = i_length;
            index_of_best = i;
          }
        }
      }
    }
    if (index_of_best < 0) break;
    assert(!vinfo[index_of_best].done);
    assert(length_for_best <= vinfo[index_of_best].length);
    vinfo[index_of_best].length = length_for_best;
    vinfo[index_of_best].done = true;
  }
  // Actually draw the quad strip:
  glBegin(GL_QUAD_STRIP); {
    for (int i = n - 1, i2 = 0; i < n; i = i2++) {
      const az_vector_t b = polygon.vertices[i];
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(b.x, b.y);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      const az_vector_t bb =
        az_vadd(b, az_vmul(vinfo[i].unit, vinfo[i].length));
      glVertex2d(bb.x, bb.y);
    }
  } glEnd();
}

static void draw_cell_tri(az_color_t color1, az_color_t color2,
                          az_color_t color3, az_polygon_t polygon) {
  for (int i = 0; i < polygon.num_vertices; ++i) {
    const az_vector_t v1 = polygon.vertices[i];
    const az_vector_t v2 = polygon.vertices[(i + 1) % polygon.num_vertices];
    const az_vector_t center = az_vdiv(az_vadd(v1, v2), 3);
    glBegin(GL_TRIANGLE_FAN); {
      glColor4ub(color3.r, color3.g, color3.b, color3.a);
      glVertex2d(center.x, center.y);
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(0, 0);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2d(v1.x, v1.y); glVertex2d(v2.x, v2.y);
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(0, 0);
    } glEnd();
  }
}

static void draw_cell_quad(az_color_t color1, az_color_t color2,
                           az_color_t color3, az_polygon_t polygon) {
  assert(polygon.num_vertices % 2 == 0);
  for (int i = 0; i < polygon.num_vertices; i += 2) {
    const az_vector_t v1 = polygon.vertices[i];
    const az_vector_t v2 = polygon.vertices[(i + 1) % polygon.num_vertices];
    const az_vector_t v3 = polygon.vertices[(i + 2) % polygon.num_vertices];
    const az_vector_t center = az_vdiv(az_vadd(az_vadd(v1, v2), v3), 4);
    glBegin(GL_TRIANGLE_FAN); {
      glColor4ub(color3.r, color3.g, color3.b, color3.a);
      glVertex2d(center.x, center.y);
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(0, 0);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2d(v1.x, v1.y); glVertex2d(v2.x, v2.y); glVertex2d(v3.x, v3.y);
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2d(0, 0);
    } glEnd();
  }
}

static void draw_girder(float bezel, az_color_t color1, az_color_t color2,
                        az_polygon_t polygon, bool cap1, bool cap2) {
  glBegin(GL_QUADS); {
    assert(polygon.num_vertices >= 3);
    const float top = polygon.vertices[1].y;
    const float bottom = polygon.vertices[polygon.num_vertices - 1].y;
    const float right = polygon.vertices[1].x;
    const float left = polygon.vertices[2].x;
    // Struts:
    const float breadth = top - bottom;
    const float strut = bezel * 0.66666;
    for (float x = left; x < right - breadth; x += breadth - strut) {
      for (int j = 0; j < 2; ++j) {
        const float y_1 = (j ? bottom : top);
        const float y_2 = (j ? top : bottom);
        glColor4ub(color1.r, color1.g, color1.b, color1.a);
        glVertex2f(x, y_1); glVertex2f(x + breadth, y_2);
        glColor4ub(color2.r, color2.g, color2.b, color2.a);
        glVertex2f(x + breadth + strut, y_2); glVertex2f(x + strut, y_1);
      }
    }
    // Edges:
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2f(left, top); glVertex2f(right, top);
    glColor4ub(color2.r, color2.g, color2.b, color2.a);
    glVertex2f(right, top - bezel); glVertex2f(left, top - bezel);
    glVertex2f(left, bottom); glVertex2f(right, bottom);
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2f(right, bottom + bezel); glVertex2f(left, bottom + bezel);
    if (cap1) {
      glVertex2f(left, top); glVertex2f(left, bottom);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2f(left + bezel, bottom + bezel);
      glVertex2f(left + bezel, top - bezel);
    }
    if (cap2) {
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2f(right - bezel, bottom + bezel);
      glVertex2f(right - bezel, top - bezel);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2f(right, top); glVertex2f(right, bottom);
    }
  } glEnd();
}

static void draw_metal(bool alt, az_color_t color1, az_color_t color2,
                       az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2d(0, 0);
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      if ((i % 2 != 0) ^ alt) {
        glColor4ub(color1.r, color1.g, color1.b, color1.a);
      } else glColor4ub(color2.r, color2.g, color2.b, color2.a);
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
}

static void draw_quadstrip(
    bool alt, double param, az_color_t color1, az_color_t color2,
    az_color_t color3, az_polygon_t polygon) {
  assert(param > -1.0 && param < 1.0);
  assert(polygon.num_vertices % 2 == 0);
  assert(polygon.num_vertices >= 4);
  const int n = polygon.num_vertices / 2 - (alt ? 1 : 0);
  az_vector_t midpoints[n];
  for (int i = 0; i < n; ++i) {
    const az_vector_t p1 = polygon.vertices[alt ? i + 1 : i];
    const az_vector_t p2 = polygon.vertices[polygon.num_vertices - i - 1];
    midpoints[i] = az_vadd(p2, az_vmul(az_vsub(p1, p2), 0.5 * (1.0 + param)));
  }
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i < n; ++i) {
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      az_gl_vertex(polygon.vertices[alt ? i + 1 : i]);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      az_gl_vertex(midpoints[i]);
    }
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i < n; ++i) {
      glColor4ub(color3.r, color3.g, color3.b, color3.a);
      az_gl_vertex(polygon.vertices[polygon.num_vertices - i - 1]);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      az_gl_vertex(midpoints[i]);
    }
  } glEnd();
  if (alt) {
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      az_gl_vertex(polygon.vertices[1]);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      az_gl_vertex(polygon.vertices[0]);
      az_gl_vertex(midpoints[0]);
      glColor4ub(color3.r, color3.g, color3.b, color3.a);
      az_gl_vertex(polygon.vertices[polygon.num_vertices - 1]);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      const int halfway = polygon.num_vertices / 2;
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      az_gl_vertex(polygon.vertices[halfway - 1]);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      az_gl_vertex(polygon.vertices[halfway]);
      az_gl_vertex(midpoints[n - 1]);
      glColor4ub(color3.r, color3.g, color3.b, color3.a);
      az_gl_vertex(polygon.vertices[halfway + 1]);
    } glEnd();
  }
}

static void draw_trifan(az_color_t color1, az_color_t color2,
                        az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2f(0, 0);
    glColor4ub(color2.r, color2.g, color2.b, color2.a);
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
    }
  } glEnd();
}

static void draw_trifan_alt(az_color_t color1, az_color_t color2,
                            az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2d(polygon.vertices[0].x, polygon.vertices[0].y);
    glColor4ub(color2.r, color2.g, color2.b, color2.a);
    for (int i = 1; i < polygon.num_vertices; i++) {
      glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
    }
    glColor4ub(color1.r, color1.g, color1.b, color1.a);
    glVertex2d(polygon.vertices[0].x, polygon.vertices[0].y);
  } glEnd();
}

static void compile_wall(const az_wall_data_t *data, GLuint list) {
  glNewList(list, GL_COMPILE); {
    switch (data->style) {
      case AZ_WSTY_BEZEL_12:
        draw_bezel(data->bezel, false, data->color1, data->color2,
                   data->polygon);
        break;
      case AZ_WSTY_BEZEL_21:
        draw_bezel(data->bezel, false, data->color2, data->color1,
                   data->polygon);
        break;
      case AZ_WSTY_BEZEL_ALT_12:
        draw_bezel(data->bezel, true, data->color1, data->color2,
                   data->polygon);
        break;
      case AZ_WSTY_BEZEL_ALT_21:
        draw_bezel(data->bezel, true, data->color2, data->color1,
                   data->polygon);
        break;
      case AZ_WSTY_CELL_TRI:
        draw_cell_tri(data->color1, data->color2, data->color3, data->polygon);
        break;
      case AZ_WSTY_CELL_QUAD:
        draw_cell_quad(data->color1, data->color2, data->color3,
                       data->polygon);
        break;
      case AZ_WSTY_GIRDER:
        draw_girder(data->bezel, data->color1, data->color2, data->polygon,
                    false, false);
        break;
      case AZ_WSTY_GIRDER_CAP:
        draw_girder(data->bezel, data->color1, data->color2, data->polygon,
                    true, false);
        break;
      case AZ_WSTY_GIRDER_CAPS:
        draw_girder(data->bezel, data->color1, data->color2, data->polygon,
                    true, true);
        break;
      case AZ_WSTY_METAL:
        draw_metal(false, data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_METAL_ALT:
        draw_metal(true, data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_123:
        draw_quadstrip(false, data->bezel, data->color1, data->color2,
                       data->color3, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_213:
        draw_quadstrip(false, data->bezel, data->color2, data->color1,
                       data->color3, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_321:
        draw_quadstrip(false, data->bezel, data->color3, data->color2,
                       data->color1, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_ALT_123:
        draw_quadstrip(true, data->bezel, data->color1, data->color2,
                       data->color3, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_ALT_213:
        draw_quadstrip(true, data->bezel, data->color2, data->color1,
                       data->color3, data->polygon);
        break;
      case AZ_WSTY_QUADSTRIP_ALT_321:
        draw_quadstrip(true, data->bezel, data->color3, data->color2,
                       data->color1, data->polygon);
        break;
      case AZ_WSTY_TRIFAN:
        draw_trifan(data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_TRIFAN_ALT:
        draw_trifan_alt(data->color1, data->color2, data->polygon);
        break;
    }
  } glEndList();
}

static GLuint wall_display_lists_start;

void az_init_wall_drawing(void) {
  wall_display_lists_start = glGenLists(AZ_NUM_WALL_DATAS);
  if (wall_display_lists_start == 0u) {
    AZ_FATAL("glGenLists failed.\n");
  }
  for (int i = 0; i < AZ_NUM_WALL_DATAS; ++i) {
    compile_wall(az_get_wall_data(i), wall_display_lists_start + i);
  }
}

/*===========================================================================*/

void az_draw_wall_data(const az_wall_data_t *data, az_clock_t clock) {
  const GLuint display_list =
    wall_display_lists_start + az_wall_data_index(data);
  assert(glIsList(display_list));
  if (data->underglow) {
    glBegin(GL_TRIANGLE_FAN); {
      const GLfloat gray = 0.002f * az_clock_zigzag(100, 1, clock);
      glColor3f(gray, gray, gray);
      for (int i = 0; i < data->polygon.num_vertices; ++i) {
        glVertex2d(data->polygon.vertices[i].x, data->polygon.vertices[i].y);
      }
    } glEnd();
  }
  glCallList(display_list);
}

void az_draw_wall(const az_wall_t *wall, az_clock_t clock) {
  assert(wall->kind != AZ_WALL_NOTHING);
  glPushMatrix(); {
    glTranslated(wall->position.x, wall->position.y, 0);
    glRotated(AZ_RAD2DEG(wall->angle), 0, 0, 1);
    az_draw_wall_data(wall->data, clock);
    if (wall->flare > 0.0) {
      assert(wall->flare <= 1.0);
      assert(wall->kind != AZ_WALL_INDESTRUCTIBLE);
      switch (wall->kind) {
        case AZ_WALL_NOTHING:
        case AZ_WALL_INDESTRUCTIBLE:
          AZ_ASSERT_UNREACHABLE();
        case AZ_WALL_DESTRUCTIBLE_CHARGED:
          glColor4f(0.5, 0.5, 0.5, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_ROCKET:
          glColor4f(0.5, 0, 0, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_HYPER_ROCKET:
          glColor4f(0.5, 0, 0.5, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_BOMB:
          glColor4f(0, 0, 0.5, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_MEGA_BOMB:
          glColor4f(0, 0.5, 0.5, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_CPLUS:
          glColor4f(0, 0.5, 0, 0.75 * wall->flare);
          break;
      }
      glBegin(GL_TRIANGLE_FAN); {
        const az_polygon_t polygon = wall->data->polygon;
        for (int i = 0; i < polygon.num_vertices; ++i) {
          glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
        }
      } glEnd();
    }
  } glPopMatrix();
}

void az_draw_walls(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    az_draw_wall(wall, state->clock);
  }
}

/*===========================================================================*/
