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
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/door.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_passage(void) {
  glColor3f(0, 1, 0); // green
  glBegin(GL_LINE_STRIP); {
    glVertex2f(30, 50); glVertex2f(-30, 50);
    glVertex2f(-30, -50); glVertex2f(30, -50);
  } glEnd();
  glBegin(GL_LINES); {
    for (GLfloat y = -50; y <= 50; y += 4) glVertex2f(30, y);
    for (GLfloat y = -50; y <= 50; y += 4) glVertex2f(-10, y);
  } glEnd();
}

static void draw_forcefield(const az_door_t *door, az_clock_t clock) {
  if (door->openness < 1.0) {
    const GLfloat alpha = 1.0f - door->openness;
    const GLfloat blue = (az_clock_mod(2, 2, clock) ? 1.0f : 0.5f);
    glBegin(GL_QUAD_STRIP); {
      glColor4f(1, 1, blue, 0.25f * alpha);
      glVertex2f(-10, -50); glVertex2f(-10, 50);
      glColor4f(1, 1, blue,
                (0.75f + 0.05f * az_clock_zigzag(5, 3, clock)) * alpha);
      glVertex2f(0, -50); glVertex2f(0, 50);
      glColor4f(1, 1, blue, 0.25f * alpha);
      glVertex2f(10, -50); glVertex2f(10, 50);
    } glEnd();
  }
  for (GLfloat sign = -1.0f; sign <= 2.0f; sign += 2.0f) {
    glBegin(GL_QUAD_STRIP); {
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2f(-14, sign * 54); glVertex2f(-12, sign * 48);
      glColor3f(0.7, 0.7, 0.7); // light gray
      glVertex2f(0, sign * 54); glVertex2f(0, sign * 48);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2f(14, sign * 54); glVertex2f(12, sign * 48);
    } glEnd();
  }
}

static void draw_door_pipe(void) {
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2f(30, 50); glVertex2f(-30, 50);
    glColor3f(0.5, 0.5, 0.5); // mid gray
    glVertex2f(30, 30); glVertex2f(-30, 30);
    glColor3f(0.7, 0.7, 0.7); // light gray
    glVertex2f(30, 0); glVertex2f(-30, 0);
    glColor3f(0.5, 0.5, 0.5); // mid gray
    glVertex2f(30, -30); glVertex2f(-30, -30);
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2f(30, -50); glVertex2f(-30, -50);
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.2, 0.2, 0.2); // dark gray
    glVertex2f(31, 51); glVertex2f(24, 51);
    glColor3f(0.45, 0.45, 0.45); // mid gray
    glVertex2f(31, 31); glVertex2f(24, 31);
    glColor3f(0.65, 0.65, 0.65); // light gray
    glVertex2f(31, 0); glVertex2f(24, 0);
    glColor3f(0.45, 0.45, 0.45); // mid gray
    glVertex2f(31, -31); glVertex2f(24, -31);
    glColor3f(0.2, 0.2, 0.2); // dark gray
    glVertex2f(31, -51); glVertex2f(24, -51);
  } glEnd();
}

static void draw_door_internal(const az_door_t *door, az_clock_t clock) {
  az_color_t color1 = AZ_WHITE, color2 = AZ_WHITE;
  switch (door->kind) {
    case AZ_DOOR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_DOOR_UNLOCKED:
      if (az_clock_mod(2, 4, clock)) {
        color1 = (az_color_t){192, 192, 64, 255};
        color2 = (az_color_t){128, 128, 48, 255};
        break;
      } // else fallthrough
    case AZ_DOOR_NORMAL:
      color1 = (az_color_t){192, 192, 192, 255};
      color2 = (az_color_t){128, 128, 128, 255};
      break;
    case AZ_DOOR_LOCKED:
      color1 = (az_color_t){80, 80, 80, 255};
      color2 = (az_color_t){48, 48, 48, 255};
      break;
    case AZ_DOOR_ROCKET:
      color1 = (az_color_t){192, 0, 0, 255};
      color2 = (az_color_t){128, 0, 0, 255};
      break;
    case AZ_DOOR_HYPER_ROCKET:
      color1 = (az_color_t){255, 0, 128, 255};
      color2 = (az_color_t){192, 0, 96, 255};
      break;
    case AZ_DOOR_BOMB:
      color1 = (az_color_t){0, 0, 192, 255};
      color2 = (az_color_t){0, 0, 128, 255};
      break;
    case AZ_DOOR_MEGA_BOMB:
      color1 = (az_color_t){0, 128, 255, 255};
      color2 = (az_color_t){0, 96, 192, 255};
      break;
    case AZ_DOOR_PASSAGE:
      draw_passage();
      return;
    case AZ_DOOR_FORCEFIELD:
      draw_forcefield(door, clock);
      return;
  }

  if (door->openness < 1.0) {
    glBegin(GL_QUADS); {
      const GLfloat x1 = 30.0;
      const GLfloat x2 = 40.0 - 10.0 * door->openness;
      glColor4ub(color1.r, color1.g, color1.b, color1.a);
      glVertex2f(x1, 20);
      glVertex2f(x1, -20);
      glVertex2f(x2, -10);
      glVertex2f(x2, 10);
      for (int i = -1; i <= 1; i += 2) {
        glColor4ub(color2.r, color2.g, color2.b, color2.a);
        glVertex2f(x2, i * 35);
        glVertex2f(x1, i * 50);
        glColor4ub(color1.r, color1.g, color1.b, color1.a);
        glVertex2f(x1, i * (20 + 10.0 * door->openness));
        glVertex2f(x2, i * (10.0 + 25.0 * door->openness));
      }
    } glEnd();
  }
  draw_door_pipe();
}

/*===========================================================================*/

void az_draw_door(const az_door_t *door, az_clock_t clock) {
  assert(door->kind != AZ_DOOR_NOTHING);
  glPushMatrix(); {
    glTranslated(door->position.x, door->position.y, 0);
    glRotated(AZ_RAD2DEG(door->angle), 0, 0, 1);
    draw_door_internal(door, clock);
  } glPopMatrix();
}

void az_draw_doors(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(door, state->doors) {
    if (door->kind == AZ_DOOR_NOTHING) continue;
    if (door->kind == AZ_DOOR_PASSAGE) continue;
    az_draw_door(door, state->clock);
  }
}

/*===========================================================================*/

void az_draw_door_shift(az_vector_t entrance_position, double entrance_angle,
                        az_vector_t exit_position, double exit_angle,
                        double progress) {
  const az_vector_t start_position =
    az_vadd(entrance_position, az_vpolar(-30, entrance_angle));
  const az_vector_t end_position =
    az_vadd(exit_position, az_vpolar(-30, exit_angle));
  const az_vector_t center =
    az_vadd(start_position,
            az_vmul(az_vsub(end_position, start_position), progress));
  const double angle =
    entrance_angle + az_mod2pi(exit_angle + AZ_PI - entrance_angle) * progress;
  glPushMatrix(); {
    glTranslated(center.x, center.y, 0);
    glRotated(AZ_RAD2DEG(angle), 0, 0, 1);
    glTranslated(30, 0, 0);
    draw_door_pipe();
    glRotated(180, 0, 0, 1);
    glTranslated(60, 0, 0);
    draw_door_pipe();
  } glPopMatrix();
}

/*===========================================================================*/
