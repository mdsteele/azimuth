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

#include "azimuth/view/minimap.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/planet.h"
#include "azimuth/state/room.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_draw_minimap_room(const az_planet_t *planet, const az_room_t *room,
                          bool visited, bool blink) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  const double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  const double max_r = min_r + bounds->r_span + AZ_SCREEN_HEIGHT;
  const double min_theta = bounds->min_theta;
  const double max_theta = min_theta + bounds->theta_span;
  const az_vector_t offset1 =
    az_vpolar(AZ_SCREEN_WIDTH/2, min_theta - AZ_HALF_PI);
  const az_vector_t offset2 =
    az_vpolar(AZ_SCREEN_WIDTH/2, max_theta + AZ_HALF_PI);
  const double step = fmax(AZ_DEG2RAD(0.1), bounds->theta_span * 0.05);

  // Fill room with color:
  const az_color_t zone_color = planet->zones[room->zone_key].color;
  if (blink) {
    glColor3f(0.75, 0.75, 0.75);
  } else if (!visited) {
    glColor3ub(zone_color.r / 4, zone_color.g / 4, zone_color.b / 4);
  } else glColor3ub(zone_color.r, zone_color.g, zone_color.b);
  if (bounds->theta_span >= 6.28) {
    glBegin(GL_POLYGON); {
      for (double theta = 0.0; theta < AZ_TWO_PI; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
    } glEnd();
  } else {
    glBegin(GL_QUAD_STRIP); {
      glVertex2d(min_r * cos(min_theta) + offset1.x,
                 min_r * sin(min_theta) + offset1.y);
      glVertex2d(max_r * cos(min_theta) + offset1.x,
                 max_r * sin(min_theta) + offset1.y);
      for (double theta = min_theta; theta <= max_theta; theta += step) {
        glVertex2d(min_r * cos(theta), min_r * sin(theta));
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
      glVertex2d(min_r * cos(max_theta) + offset2.x,
                 min_r * sin(max_theta) + offset2.y);
      glVertex2d(max_r * cos(max_theta) + offset2.x,
                 max_r * sin(max_theta) + offset2.y);
    } glEnd();
  }

  // Draw outline:
  glColor3f(0.9, 0.9, 0.9); // white
  glBegin(GL_LINE_LOOP); {
    if (bounds->theta_span >= 6.28) {
      for (double theta = 0.0; theta < AZ_TWO_PI; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
    } else {
      glVertex2d(min_r * cos(min_theta) + offset1.x,
                 min_r * sin(min_theta) + offset1.y);
      glVertex2d(max_r * cos(min_theta) + offset1.x,
                 max_r * sin(min_theta) + offset1.y);
      for (double theta = min_theta; theta <= max_theta; theta += step) {
        glVertex2d(max_r * cos(theta), max_r * sin(theta));
      }
      glVertex2d(max_r * cos(max_theta) + offset2.x,
                 max_r * sin(max_theta) + offset2.y);
      glVertex2d(min_r * cos(max_theta) + offset2.x,
                 min_r * sin(max_theta) + offset2.y);
      for (double theta = max_theta; theta >= min_theta; theta -= step) {
        glVertex2d(min_r * cos(theta), min_r * sin(theta));
      }
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_map_marker(az_vector_t center, az_clock_t clock) {
  glPushMatrix(); {
    glTranslated(center.x, center.y, 0);
    glRotatef(-3 * az_clock_mod(120, 1, clock), 0, 0, 1);
    for (int i = 0; i < 4; ++i) {
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0, 0); glVertex2f(4, 0); glColor3f(1, 1, 1);
        glVertex2f(5, -4); glVertex2f(1, 0); glVertex2f(5, 4);
      } glEnd();
      glRotated(90, 0, 0, 1);
    }
  } glPopMatrix();
}

static void draw_room_label(az_vector_t center, int num_vertices,
                            const float vertices[]) {
  glPushMatrix(); {
    glTranslatef(floorf(center.x) + 0.5f, floorf(center.y) + 0.5f, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glVertex2f(4, 4); glVertex2f(-4, 4);
      glVertex2f(-4, -4); glVertex2f(4, -4);
    } glEnd();
    glColor3f(0, 0, 0);
    glBegin(GL_LINE_STRIP); {
      for (int i = 0; i < num_vertices; ++i) {
        glVertex2f(vertices[2 * i], vertices[2 * i + 1]);
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_save_room_label(az_vector_t center) {
  const float vertices[] = {2, -2, -2, -2, -2, 0, 2, 0, 2, 2, -2, 2};
  glColor3f(0.5, 1, 0.5);
  draw_room_label(center, AZ_ARRAY_SIZE(vertices)/2, vertices);
}

void az_draw_comm_room_label(az_vector_t center) {
  const float vertices[] = {2, -2, -1, -2, -2, -1, -2, 1, -1, 2, 2, 2};
  glColor3f(0.5, 0.5, 1);
  draw_room_label(center, AZ_ARRAY_SIZE(vertices)/2, vertices);
}

void az_draw_refill_room_label(az_vector_t center) {
  const float vertices[] = {-2, 2.5, -2, -2, 2, -2, 2, 0, -2, 0, 2.5, 2.5};
  glColor3f(1, 1, 0.5);
  draw_room_label(center, AZ_ARRAY_SIZE(vertices)/2, vertices);
}

/*===========================================================================*/
