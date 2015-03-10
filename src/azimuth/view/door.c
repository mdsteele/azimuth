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
#include "azimuth/view/util.h"

/*===========================================================================*/

static void gl_gray(GLfloat gray, GLfloat alpha) {
  assert(gray >= 0.0f && gray <= 1.0f);
  assert(alpha >= 0.0f && alpha <= 1.0f);
  glColor4f(gray, gray, gray, alpha);
}

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

static void draw_door_pipe(GLfloat alpha, az_color_t light_color,
                           uint8_t light_mask) {
  // Tube:
  glBegin(GL_QUAD_STRIP); {
    glColor4f(0.25, 0.25, 0.25, alpha); // dark gray
    glVertex2f(30, 50); glVertex2f(-30, 50);
    glColor4f(0.5, 0.5, 0.5, alpha); // mid gray
    glVertex2f(30, 30); glVertex2f(-30, 30);
    glColor4f(0.7, 0.7, 0.7, alpha); // light gray
    glVertex2f(30, 0); glVertex2f(-30, 0);
    glColor4f(0.5, 0.5, 0.5, alpha); // mid gray
    glVertex2f(30, -30); glVertex2f(-30, -30);
    glColor4f(0.25, 0.25, 0.25, alpha); // dark gray
    glVertex2f(30, -50); glVertex2f(-30, -50);
  } glEnd();
  // Struts:
  const GLfloat beta = pow(alpha, 4);
  for (int i = 0; i < 4; ++i) {
    const double theta = AZ_DEG2RAD(22.5 + 45 * i);
    const double yc = 50 * cos(theta);
    glBegin(GL_TRIANGLE_STRIP); {
      const double semi = 2 + 2 * sin(theta);
      gl_gray(0.1 + 0.53 * sin(theta), beta);
      glVertex2d(8, yc + semi); glVertex2d(24, yc + semi);
      gl_gray(0.1 + 0.43 * sin(theta), beta);
      glVertex2d(4, yc);        glVertex2d(24, yc);
      gl_gray(0.1 + 0.53 * sin(theta), beta);
      glVertex2d(8, yc - semi); glVertex2d(24, yc - semi);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      const double semi = 0.5 + 0.5 * sin(theta);
      gl_gray(0.1 + 0.53 * sin(theta), beta);
      glVertex2d(-30, yc + semi); glVertex2d(5, yc + semi);
      gl_gray(0.1 + 0.43 * sin(theta), beta);
      glVertex2d(-30, yc);        glVertex2d(5, yc);
      gl_gray(0.1 + 0.53 * sin(theta), beta);
      glVertex2d(-30, yc - semi); glVertex2d(5, yc - semi);
    } glEnd();
    // Lights:
    if (light_mask & (1 << i)) {
      az_color_t dim_color = light_color;
      dim_color.a = 0;
      glBegin(GL_TRIANGLE_FAN); {
        const double xr = 7 + 3 * sin(theta);
        const double yr = 3 + 3 * sin(theta);
        az_gl_color(light_color);
        glVertex2d(16, yc);
        az_gl_color(dim_color);
        for (int j = 0; j <= 360; j += 30) {
          glVertex2d(16 + xr * cos(AZ_DEG2RAD(j)),
                     yc + yr * sin(AZ_DEG2RAD(j)));
        }
      } glEnd();
    } else {
      glBegin(GL_LINES); {
        glColor4f(0, 0, 0, 0.25 * beta);
        glVertex2d(13, yc); glVertex2d(19, yc);
      } glEnd();
    }
  }
  // Rim:
  glBegin(GL_QUAD_STRIP); {
    glColor4f(0.2, 0.2, 0.2, alpha); // dark gray
    glVertex2f(31, 51); glVertex2f(24, 51);
    glColor4f(0.45, 0.45, 0.45, alpha); // mid gray
    glVertex2f(31, 31); glVertex2f(24, 31);
    glColor4f(0.65, 0.65, 0.65, alpha); // light gray
    glVertex2f(31, 0); glVertex2f(24, 0);
    glColor4f(0.45, 0.45, 0.45, alpha); // mid gray
    glVertex2f(31, -31); glVertex2f(24, -31);
    glColor4f(0.2, 0.2, 0.2, alpha); // dark gray
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
      color2 = (az_color_t){96, 0, 0, 255};
      break;
    case AZ_DOOR_HYPER_ROCKET:
      color1 = (az_color_t){255, 0, 128, 255};
      color2 = (az_color_t){128, 0, 64, 255};
      break;
    case AZ_DOOR_BOMB:
      color1 = (az_color_t){24, 24, 192, 255};
      color2 = (az_color_t){12, 12, 96, 255};
      break;
    case AZ_DOOR_MEGA_BOMB:
      color1 = (az_color_t){0, 128, 255, 255};
      color2 = (az_color_t){0, 80, 160, 255};
      break;
    case AZ_DOOR_PASSAGE:
      draw_passage();
      return;
    case AZ_DOOR_FORCEFIELD:
      draw_forcefield(door, clock);
      return;
    case AZ_DOOR_ALWAYS_OPEN:
      assert(door->is_open);
      assert(door->openness == 1.0);
      break;
    case AZ_DOOR_BOSS:
      break;
  }

  if (door->kind == AZ_DOOR_BOSS) {
    glBegin(GL_QUADS); {
      glColor4f(0.5, 0.5, 0.5, 0.5);
      glVertex2f(30, -50); glVertex2f(30, 50);
      glVertex2f(32, 48); glVertex2f(32, -48);
    } glEnd();
  } else if (door->openness < 1.0) {
    assert(door->kind != AZ_DOOR_ALWAYS_OPEN);
    glBegin(GL_QUADS); {
      const GLfloat x1 = 30.0;
      const GLfloat x2 = 40.0 - 10.0 * door->openness;
      az_gl_color(color1);
      glVertex2f(x1, 20);
      glVertex2f(x1, -20);
      glVertex2f(x2, -10);
      glVertex2f(x2, 10);
      for (int i = -1; i <= 1; i += 2) {
        az_gl_color(color2);
        glVertex2f(x2, i * 35);
        glVertex2f(x1, i * 50);
        az_gl_color(color1);
        glVertex2f(x1, i * (20 + 10.0 * door->openness));
        glVertex2f(x2, i * (10.0 + 25.0 * door->openness));
      }
    } glEnd();
  }

  if (door->lockedness > 0.0) {
    glPushMatrix(); {
      glTranslated(-12 + 11 * pow(door->lockedness, 4), 0, 0);
      for (int i = 0; i < 2; ++i) {
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.5, 0.5, 0.5); glVertex2f(30, 6);
          glColor3f(0.35, 0.35, 0.35); glVertex2f(30, 31);
          glVertex2f(42, 27); glVertex2f(42, 10);
        } glEnd();
        glBegin(GL_TRIANGLE_STRIP); {
          glColor4f(0, 0, 0, 0.5);
          glVertex2f(33, 10); glVertex2f(33, 27);
          glVertex2f(40, 12); glVertex2f(40, 25);
        } glEnd();
        glBegin(GL_TRIANGLES); {
          glColor4f(1, 1, 0, 0.5);
          if (i == 0) {
            glVertex2f(34, 26); glVertex2f(34, 22); glVertex2f(39, 24.5);
            glVertex2f(34, 17); glVertex2f(34, 13.5); glVertex2f(39, 15);
            glVertex2f(34, 17); glVertex2f(39, 19); glVertex2f(39, 15);
          } else {
            glVertex2f(34, 11); glVertex2f(34, 15); glVertex2f(39, 12.5);
            glVertex2f(34, 20); glVertex2f(34, 23.5); glVertex2f(39, 22);
            glVertex2f(34, 20); glVertex2f(39, 18); glVertex2f(39, 22);
          }
        } glEnd();
        glScalef(1, -1, 1);
      }
    } glPopMatrix();
  }

  az_color_t light_color = AZ_WHITE;
  uint8_t light_mask = 0;
  if (door->kind != AZ_DOOR_LOCKED && door->kind != AZ_DOOR_ALWAYS_OPEN) {
    switch (door->marker) {
      case AZ_DOORMARK_NORMAL: break;
      case AZ_DOORMARK_COMM:
        light_color = (az_color_t){128, 192, 255, 255};
        light_mask = az_clock_mod(2, 25, clock) == 0 ? 0 : 0xF;
        break;
      case AZ_DOORMARK_REFILL:
        light_color = (az_color_t){255, 255, 128, 255};
        light_mask = 0x1 << az_clock_mod(4, 10, clock);
        break;
      case AZ_DOORMARK_SAVE:
        light_color = (az_color_t){128, 255, 128, 255};
        light_mask = 0xE0 >> az_clock_mod(8, 7, clock);
        break;
    }
  }
  draw_door_pipe(1, light_color, light_mask);
}

/*===========================================================================*/

void az_draw_door(const az_door_t *door, az_clock_t clock) {
  assert(door->kind != AZ_DOOR_NOTHING);
  glPushMatrix(); {
    az_gl_translated(door->position);
    az_gl_rotated(door->angle);
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

void az_draw_door_pipe_fade(az_vector_t position, double angle, float alpha) {
  glPushMatrix(); {
    az_gl_translated(position);
    az_gl_rotated(angle);
    draw_door_pipe(alpha, AZ_WHITE, 0);
    glRotated(180, 0, 0, 1);
    glTranslated(60, 0, 0);
    draw_door_pipe(alpha, AZ_WHITE, 0);
  } glPopMatrix();
}

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
    az_gl_translated(center);
    az_gl_rotated(angle);
    glTranslated(30, 0, 0);
    draw_door_pipe(1, AZ_WHITE, 0);
    glRotated(180, 0, 0, 1);
    glTranslated(60, 0, 0);
    draw_door_pipe(1, AZ_WHITE, 0);
  } glPopMatrix();
}

/*===========================================================================*/
