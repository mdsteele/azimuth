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
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_bezel(double bezel, bool alt, az_color_t color1,
                       az_color_t color2, az_polygon_t polygon) {
  // Draw background color:
  if (color2.a != 0) {
    az_gl_color(color2);
    glBegin(GL_TRIANGLE_FAN); {
      for (int i = 0; i < polygon.num_vertices; ++i) {
        az_gl_vertex(polygon.vertices[i]);
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
      az_gl_color(color1);
      az_gl_vertex(b);
      az_gl_color(color2);
      az_gl_vertex(az_vadd(b, az_vmul(vinfo[i].unit, vinfo[i].length)));
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
      az_gl_color(color3); az_gl_vertex(center);
      az_gl_color(color1); glVertex2f(0, 0);
      az_gl_color(color2); az_gl_vertex(v1); az_gl_vertex(v2);
      az_gl_color(color1); glVertex2f(0, 0);
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
      az_gl_color(color3); az_gl_vertex(center);
      az_gl_color(color1); glVertex2f(0, 0);
      az_gl_color(color2);
      az_gl_vertex(v1); az_gl_vertex(v2); az_gl_vertex(v3);
      az_gl_color(color1); glVertex2f(0, 0);
    } glEnd();
  }
}

static void draw_girder(
    float bezel, float strut, az_color_t color1, az_color_t color2,
    az_polygon_t polygon, bool cap1, bool cap2) {
  glBegin(GL_QUADS); {
    assert(polygon.num_vertices >= 3);
    const float top = polygon.vertices[1].y;
    const float bottom = polygon.vertices[polygon.num_vertices - 1].y;
    const float right = polygon.vertices[1].x;
    const float left = polygon.vertices[2].x;
    // Struts:
    const float breadth = top - bottom;
    for (float x = left; x < right - breadth; x += breadth - strut) {
      for (int j = 0; j < 2; ++j) {
        const float y_1 = (j ? bottom : top);
        const float y_2 = (j ? top : bottom);
        az_gl_color(color1);
        glVertex2f(x, y_1); glVertex2f(x + breadth, y_2);
        az_gl_color(color2);
        glVertex2f(x + breadth + strut, y_2); glVertex2f(x + strut, y_1);
      }
    }
    // Edges:
    az_gl_color(color1);
    glVertex2f(left, top); glVertex2f(right, top);
    az_gl_color(color2);
    glVertex2f(right, top - bezel); glVertex2f(left, top - bezel);
    glVertex2f(left, bottom); glVertex2f(right, bottom);
    az_gl_color(color1);
    glVertex2f(right, bottom + bezel); glVertex2f(left, bottom + bezel);
    if (cap1) {
      glVertex2f(left, top); glVertex2f(left, bottom);
      az_gl_color(color2);
      glVertex2f(left + bezel, bottom + bezel);
      glVertex2f(left + bezel, top - bezel);
    }
    if (cap2) {
      az_gl_color(color1);
      glVertex2f(right - bezel, bottom + bezel);
      glVertex2f(right - bezel, top - bezel);
      az_gl_color(color2);
      glVertex2f(right, top);
      glVertex2f(right, bottom);
    }
  } glEnd();
}

static void draw_metal(bool alt, az_color_t color1, az_color_t color2,
                       az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1);
    glVertex2f(0, 0);
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      if ((i % 2 != 0) ^ alt) {
        az_gl_color(color1);
      } else az_gl_color(color2);
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
      az_gl_color(color1);
      az_gl_vertex(polygon.vertices[alt ? i + 1 : i]);
      az_gl_color(color2);
      az_gl_vertex(midpoints[i]);
    }
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i < n; ++i) {
      az_gl_color(color3);
      az_gl_vertex(polygon.vertices[polygon.num_vertices - i - 1]);
      az_gl_color(color2);
      az_gl_vertex(midpoints[i]);
    }
  } glEnd();
  if (alt) {
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(color1);
      az_gl_vertex(polygon.vertices[1]);
      az_gl_color(color2);
      az_gl_vertex(polygon.vertices[0]);
      az_gl_vertex(midpoints[0]);
      az_gl_color(color3);
      az_gl_vertex(polygon.vertices[polygon.num_vertices - 1]);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      const int halfway = polygon.num_vertices / 2;
      az_gl_color(color1);
      az_gl_vertex(polygon.vertices[halfway - 1]);
      az_gl_color(color2);
      az_gl_vertex(polygon.vertices[halfway]);
      az_gl_vertex(midpoints[n - 1]);
      az_gl_color(color3);
      az_gl_vertex(polygon.vertices[halfway + 1]);
    } glEnd();
  }
}

static void draw_tfqs(double param, az_color_t color1, az_color_t color2,
                      az_color_t color3, az_polygon_t polygon) {
  const int n = polygon.num_vertices;
  assert(n >= 3);
  az_vector_t midpoints[n];
  const double factor = 0.5 * (1 + param);
  for (int i = 0; i < n; ++i) {
    midpoints[i] = az_vmul(polygon.vertices[i], factor);
  }
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(0, 0);
    az_gl_color(color2);
    for (int i = n - 1, j = 0; i < n; i = j++) {
      az_gl_vertex(midpoints[i]);
    }
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    for (int i = n - 1, j = 0; i < n; i = j++) {
      az_gl_color(color2); az_gl_vertex(midpoints[i]);
      az_gl_color(color3); az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
}

static void draw_trifan(az_color_t color1, az_color_t color2,
                        az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(0, 0);
    az_gl_color(color2);
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
}

static void draw_trifan_alt(az_color_t color1, az_color_t color2,
                            az_polygon_t polygon) {
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); az_gl_vertex(polygon.vertices[0]);
    az_gl_color(color2);
    for (int i = 1; i < polygon.num_vertices; i++) {
      az_gl_vertex(polygon.vertices[i]);
    }
    az_gl_color(color1); az_gl_vertex(polygon.vertices[0]);
  } glEnd();
}

static void draw_volcanic(double bezel, az_color_t color1, az_color_t color2,
                          az_color_t color3, az_polygon_t polygon,
                          double bounding_radius) {
  assert(bezel > 0.0);
  draw_bezel(bezel, true, color2, color1, polygon);
  const double xstep = 10.0 + 0.2 * bezel;
  const double ystep = (sqrt(3) / 3.0) * xstep;
  az_color_t color4 = color2; color4.a = 0;
  az_random_seed_t seed = {3, 7};
  bool indent = false;
  for (double y = -bounding_radius; y < bounding_radius; y += ystep) {
    for (double x = -bounding_radius; x < bounding_radius; x += xstep) {
      const double rx = 0.4 * xstep * 0.25 * (4.0 + az_rand_sdouble(&seed));
      const double ry = 0.4 * xstep * 0.25 * (4.0 + az_rand_sdouble(&seed));
      const double cx = x + 0.3 * xstep * az_rand_sdouble(&seed) +
        (indent ? 0.5 * xstep : 0);
      const double cy = y + 0.3 * ystep * az_rand_sdouble(&seed);
      if (!az_polygon_contains_circle(polygon, fmax(rx, ry),
                                      (az_vector_t){cx, cy})) continue;
      const double theta = AZ_TWO_PI * az_rand_udouble(&seed);
      az_color_t color5 = color3;
      color5.a = (double)color5.a * (1.0 - hypot(cx, cy) / bounding_radius);
      glBegin(GL_TRIANGLE_FAN); {
        az_gl_color(color5); glVertex2f(cx, cy); az_gl_color(color4);
        for (int i = 0; i <= 360; i += 45) {
          glVertex2d(cx + rx * cos(AZ_DEG2RAD(i) + theta),
                     cy + ry * sin(AZ_DEG2RAD(i) + theta));
        }
      } glEnd();
    }
    indent = !indent;
  }
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
        draw_girder(data->bezel, data->bezel * 0.66666f, data->color1,
                    data->color2, data->polygon, false, false);
        break;
      case AZ_WSTY_GIRDER_CAP:
        draw_girder(data->bezel, data->bezel * 0.66666f, data->color1,
                    data->color2, data->polygon, true, false);
        break;
      case AZ_WSTY_GIRDER_CAPS:
        draw_girder(data->bezel, data->bezel * 0.66666f, data->color1,
                    data->color2, data->polygon, true, true);
        break;
      case AZ_WSTY_HEAVY_GIRDER:
        draw_girder(data->bezel, data->bezel * 3.5f, data->color1,
                    data->color2, data->polygon, false, false);
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
      case AZ_WSTY_TFQS_123:
        draw_tfqs(data->bezel, data->color1, data->color2, data->color3,
                  data->polygon);
        break;
      case AZ_WSTY_TFQS_213:
        draw_tfqs(data->bezel, data->color2, data->color1, data->color3,
                  data->polygon);
        break;
      case AZ_WSTY_TFQS_321:
        draw_tfqs(data->bezel, data->color3, data->color2, data->color1,
                  data->polygon);
        break;
      case AZ_WSTY_TRIFAN:
        draw_trifan(data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_TRIFAN_ALT:
        draw_trifan_alt(data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_VOLCANIC:
        draw_volcanic(data->bezel, data->color1, data->color2, data->color3,
                      data->polygon, data->bounding_radius);
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
  if (data->underglow.a != 0) {
    glBegin(GL_TRIANGLE_FAN); {
      const float mult = 0.01f * az_clock_zigzag(100, 1, clock);
      glColor4f(((float)data->underglow.r / 255.0f) * mult,
                ((float)data->underglow.g / 255.0f) * mult,
                ((float)data->underglow.b / 255.0f) * mult,
                ((float)data->underglow.a / 255.0f));
      glVertex2f(0, 0);
      for (int i = 0; i < data->polygon.num_vertices; ++i) {
        az_gl_vertex(data->polygon.vertices[i]);
      }
      az_gl_vertex(data->polygon.vertices[0]);
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
          glColor4f(0.75, 0, 0.65, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_BOMB:
          glColor4f(0, 0, 0.5, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_MEGA_BOMB:
          glColor4f(0, 0.65, 0.75, 0.75 * wall->flare);
          break;
        case AZ_WALL_DESTRUCTIBLE_CPLUS:
          glColor4f(0, 0.5, 0, 0.75 * wall->flare);
          break;
      }
      glBegin(GL_TRIANGLE_FAN); {
        glVertex2f(0, 0);
        const az_polygon_t polygon = wall->data->polygon;
        for (int i = 0; i < polygon.num_vertices; ++i) {
          az_gl_vertex(polygon.vertices[i]);
        }
        az_gl_vertex(polygon.vertices[0]);
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
