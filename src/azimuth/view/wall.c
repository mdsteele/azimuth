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

#include <GL/gl.h>

#include "azimuth/state/space.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static void with_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

static void draw_wall(const az_wall_t *wall) {
  const az_wall_data_t *data = wall->data;
  with_color(data->color);
  glBegin(GL_LINE_LOOP); {
    for (int i = 0; i < data->polygon.num_vertices; ++i) {
      glVertex2d(data->polygon.vertices[i].x, data->polygon.vertices[i].y);
    }
  } glEnd();
}

void az_draw_walls(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(wall, state->walls) {
    if (wall->kind == AZ_BAD_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(wall->position.x, wall->position.y, 0);
      glRotated(AZ_RAD2DEG(wall->angle), 0, 0, 1);
      draw_wall(wall);
    } glPopMatrix();
  }
}

/*===========================================================================*/
