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

/*===========================================================================*/

static double mod2pi_nonneg(double theta) {
  const double theta2 = az_mod2pi(theta);
  return (theta2 < 0.0 ? theta2 + AZ_TWO_PI : theta2);
}

static void compile_wall(const az_wall_data_t *data, GLuint list) {
  glNewList(list, GL_COMPILE); {
    glColor3f(0, 0, 0);
    glBegin(GL_TRIANGLE_FAN); {
      for (int i = 0; i < data->polygon.num_vertices; ++i) {
        glVertex2d(data->polygon.vertices[i].x, data->polygon.vertices[i].y);
      }
    } glEnd();

    {
      az_color_t c1 = data->color;
      az_color_t c2 = c1; c2.a = 0;
      const int n = data->polygon.num_vertices;
      for (int i = 0; i < n; ++i) {
        az_vector_t a = data->polygon.vertices[i];
        az_vector_t b = data->polygon.vertices[(i + 1) % n];
        az_vector_t c = data->polygon.vertices[(i + 2) % n];
        az_vector_t d = data->polygon.vertices[(i + 3) % n];
        glBegin(GL_QUADS); {
          glColor4ub(c1.r, c1.g, c1.b, c1.a);
          glVertex2d(b.x, b.y);
          glVertex2d(c.x, c.y);
          glColor4ub(c2.r, c2.g, c2.b, c2.a);
          const double bezel = 18.0;
          const double td = az_vtheta(az_vsub(d, c));
          const double tb = az_vtheta(az_vsub(b, c));
          const double ttc = 0.5 * mod2pi_nonneg(tb - td);
          az_vector_t dd =
            az_vadd(c, az_vpolar(bezel / sin(ttc), td + ttc));
          glVertex2d(dd.x, dd.y);
          const double ta = az_vtheta(az_vsub(a, b));
          const double tc = az_vtheta(az_vsub(c, b));
          const double ttb = 0.5 * mod2pi_nonneg(ta - tc);
          az_vector_t aa =
            az_vadd(b, az_vpolar(bezel / sin(ttb), tc + ttb));
          glVertex2d(aa.x, aa.y);
        } glEnd();
      }
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
  const GLuint display_list =
    wall_display_lists_start + az_wall_data_index(wall->data);
  assert(glIsList(display_list));
  glPushMatrix(); {
    glTranslated(wall->position.x, wall->position.y, 0);
    glRotated(AZ_RAD2DEG(wall->angle), 0, 0, 1);
    glCallList(display_list);
  } glPopMatrix();
}

void az_draw_walls(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_WALL_NOTHING) continue;
    az_draw_wall(wall);
  }
}

/*===========================================================================*/
