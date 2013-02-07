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
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_bezel(double bezel, az_color_t color1, az_color_t color2,
                       az_polygon_t polygon) {
  // Draw background color:
  if (color2.a != 0) {
    glColor4ub(color2.r, color2.g, color2.b, color2.a);
    glBegin(GL_TRIANGLE_FAN); {
      for (int i = 0; i < polygon.num_vertices; ++i) {
        glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
      }
    } glEnd();
  }
  // Draw bezel:
  az_color_t c1 = color1;
  az_color_t c2 = c1; c2.a = 0;
  const int n = polygon.num_vertices;
  for (int i = 0; i < n; ++i) {
    az_vector_t a = polygon.vertices[i];
    az_vector_t b = polygon.vertices[(i + 1) % n];
    az_vector_t c = polygon.vertices[(i + 2) % n];
    az_vector_t d = polygon.vertices[(i + 3) % n];
    glBegin(GL_QUADS); {
      glColor4ub(c1.r, c1.g, c1.b, c1.a);
      glVertex2d(b.x, b.y);
      glVertex2d(c.x, c.y);
      glColor4ub(c2.r, c2.g, c2.b, c2.a);
      const double td = az_vtheta(az_vsub(d, c));
      const double tb = az_vtheta(az_vsub(b, c));
      const double ttc = 0.5 * az_mod2pi_nonneg(tb - td);
      az_vector_t dd =
        az_vadd(c, az_vpolar(bezel / sin(ttc), td + ttc));
      glVertex2d(dd.x, dd.y);
      const double ta = az_vtheta(az_vsub(a, b));
      const double tc = az_vtheta(az_vsub(c, b));
      const double ttb = 0.5 * az_mod2pi_nonneg(ta - tc);
      az_vector_t aa =
        az_vadd(b, az_vpolar(bezel / sin(ttb), tc + ttb));
      glVertex2d(aa.x, aa.y);
    } glEnd();
  }
}

static void draw_girder(float bezel, az_color_t color1, az_color_t color2,
                        az_polygon_t polygon, bool cap) {
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
        const float y1 = (j ? bottom : top);
        const float y2 = (j ? top : bottom);
        glColor4ub(color1.r, color1.g, color1.b, color1.a);
        glVertex2f(x, y1); glVertex2f(x + breadth, y2);
        glColor4ub(color2.r, color2.g, color2.b, color2.a);
        glVertex2f(x + breadth + strut, y2); glVertex2f(x + strut, y1);
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
    if (cap) {
      glVertex2f(left, top); glVertex2f(left, bottom);
      glColor4ub(color2.r, color2.g, color2.b, color2.a);
      glVertex2f(left + bezel, bottom + bezel);
      glVertex2f(left + bezel, top - bezel);
    }
  } glEnd();
}

static void compile_wall(const az_wall_data_t *data, GLuint list) {
  glNewList(list, GL_COMPILE); {
    switch (data->style) {
      case AZ_WSTY_BEZEL_12:
        draw_bezel(data->bezel, data->color1, data->color2, data->polygon);
        break;
      case AZ_WSTY_BEZEL_21:
        draw_bezel(data->bezel, data->color2, data->color1, data->polygon);
        break;
      case AZ_WSTY_GIRDER:
        draw_girder(data->bezel, data->color1, data->color2, data->polygon,
                    false);
        break;
      case AZ_WSTY_GIRDER_CAP:
        draw_girder(data->bezel, data->color1, data->color2, data->polygon,
                    true);
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

void az_draw_wall(const az_wall_t *wall) {
  assert(wall->kind != AZ_WALL_NOTHING);
  const GLuint display_list =
    wall_display_lists_start + az_wall_data_index(wall->data);
  assert(glIsList(display_list));
  glPushMatrix(); {
    glTranslated(wall->position.x, wall->position.y, 0);
    glRotated(AZ_RAD2DEG(wall->angle), 0, 0, 1);
    glCallList(display_list);
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
    az_draw_wall(wall);
  }
}

/*===========================================================================*/
