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

#include "azimuth/view/door.h"

#include <assert.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/door.h"
#include "azimuth/state/space.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_passage(const az_door_t *door) {
  glColor3f(0, 1, 0); // green
  glBegin(GL_LINES); {
    for (double x = 30; x >= -30; x -= 4) glVertex2d(x, 50);
    for (double y = 50; y >= -50; y -= 4) glVertex2d(-30, y);
    for (double x = -30; x <= 30; x += 4) glVertex2d(x, -50);
    for (double y = -50; y <= 50; y += 4) glVertex2d(30, y);
    for (double y = -50; y <= 50; y += 4) glVertex2d(-10, y);
  } glEnd();
}

static void draw_door_internal(const az_door_t *door) {
  az_color_t color = AZ_WHITE;
  switch (door->kind) {
    case AZ_DOOR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_DOOR_NORMAL:
      color = (az_color_t){192, 192, 192, 255};
      break;
    case AZ_DOOR_LOCKED:
      color = (az_color_t){64, 64, 64, 255};
      break;
    case AZ_DOOR_ROCKET:
      color = (az_color_t){255, 0, 0, 255};
      break;
    case AZ_DOOR_HYPER_ROCKET:
      color = (az_color_t){255, 0, 128, 255};
      break;
    case AZ_DOOR_BOMB:
      color = (az_color_t){0, 0, 255, 255};
      break;
    case AZ_DOOR_MEGA_BOMB:
      color = (az_color_t){0, 128, 255, 255};
      break;
    case AZ_DOOR_PASSAGE:
      draw_passage(door);
      return;
  }

  if (door->openness < 1.0) {
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS); {
      glVertex2d(30, 50);
      glVertex2d(30, -50);
      glVertex2d(40.0 - 10.0 * door->openness, -35);
      glVertex2d(40.0 - 10.0 * door->openness, 35);
    } glEnd();
  }
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2d(30, 50);
    glVertex2d(-30, 50);
    glColor3f(0.5, 0.5, 0.5); // mid gray
    glVertex2d(30, 30);
    glVertex2d(-30, 30);
    glColor3f(0.7, 0.7, 0.7); // light gray
    glVertex2d(30, 0);
    glVertex2d(-30, 0);
    glColor3f(0.5, 0.5, 0.5); // mid gray
    glVertex2d(30, -30);
    glVertex2d(-30, -30);
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2d(30, -50);
    glVertex2d(-30, -50);
  } glEnd();
}

void az_draw_door(const az_door_t *door) {
  assert(door->kind != AZ_DOOR_NOTHING);
  glPushMatrix(); {
    glTranslated(door->position.x, door->position.y, 0);
    glRotated(AZ_RAD2DEG(door->angle), 0, 0, 1);
    draw_door_internal(door);
  } glPopMatrix();
}

void az_draw_doors(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    if (door->kind == AZ_DOOR_PASSAGE) continue;
    az_draw_door(door);
  }
}

/*===========================================================================*/
