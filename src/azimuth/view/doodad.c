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

#include "azimuth/view/doodad.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/node.h"

/*===========================================================================*/

static void draw_drill_spikes(double angle_offset, az_clock_t clock) {
  for (int i = 0; i < 4; ++i) {
    const double radians =
      3.0 * AZ_DEG2RAD(az_clock_mod(60, 1, clock + 15 * i)) + angle_offset;
    const GLfloat x = 37.5f - 25.0f * i;
    const GLfloat y = 30 * sin(radians);
    const GLfloat c = 0.5f + 0.5f * cos(radians);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.7f * c, 0.7f * c, 0.7f * c);
      glVertex2d(x, 45 * sin(radians));
      glColor3f(0.3f * c, 0.3f * c, 0.3f * c);
      const GLfloat dy = 9 * cos(radians) * copysign(1, y);
      glVertex2f(x + 8, y); glVertex2f(x, y + dy);
      glVertex2f(x - 8, y); glVertex2f(x, y - dy);
      glVertex2f(x + 8, y);
    } glEnd();
  }
}

static void draw_drill_shaft(az_clock_t clock) {
  draw_drill_spikes(AZ_HALF_PI, clock);
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.1, 0.1, 0.1); glVertex2d(-50,  30); glVertex2d(50,  30);
    glColor3f(0.4, 0.4, 0.4); glVertex2d(-50,   0); glVertex2d(50,   0);
    glColor3f(0.1, 0.1, 0.1); glVertex2d(-50, -30); glVertex2d(50, -30);
  } glEnd();
  draw_drill_spikes(-AZ_HALF_PI, clock);
}

static void draw_chain_link_pair(void) {
  // Facing link:
  for (int r1 = 3; r1 <= 7; r1 += 4) {
    const int r2 = 5;
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 90; i <= 270; i += 30) {
        glColor3f(0.25, 0.25, 0.25);
        glVertex2d(3 + r1 * cos(AZ_DEG2RAD(i)), r1 * sin(AZ_DEG2RAD(i)));
        glColor3f(0.60, 0.60, 0.60);
        glVertex2d(3 + r2 * cos(AZ_DEG2RAD(i)), r2 * sin(AZ_DEG2RAD(i)));
      }
      for (int i = -90; i <= 90; i += 30) {
        glColor3f(0.25, 0.25, 0.25);
        glVertex2d(12 + r1 * cos(AZ_DEG2RAD(i)), r1 * sin(AZ_DEG2RAD(i)));
        glColor3f(0.60, 0.60, 0.60);
        glVertex2d(12 + r2 * cos(AZ_DEG2RAD(i)), r2 * sin(AZ_DEG2RAD(i)));
      }
      glColor3f(0.25, 0.25, 0.25); glVertex2d(3, r1);
      glColor3f(0.60, 0.60, 0.60); glVertex2d(3, r2);
    } glEnd();
  }
  // On-edge link:
  glBegin(GL_TRIANGLE_STRIP); {
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-17,  2); glVertex2f(2,  2);
    glColor3f(0.65, 0.65, 0.65); glVertex2f(-17,  0); glVertex2f(2,  0);
    glColor3f(0.25, 0.25, 0.25); glVertex2f(-17, -2); glVertex2f(2, -2);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.65, 0.65, 0.65); glVertex2f(-15, 0);
    glColor3f(0.25, 0.25, 0.25);
    glVertex2f(-17, 2); glVertex2f(-19, 0); glVertex2f(-17, -2);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.65, 0.65, 0.65); glVertex2f(0, 0);
    glColor3f(0.25, 0.25, 0.25);
    glVertex2f(2, -2); glVertex2f(4, 0); glVertex2f(2, 2);
  } glEnd();
}

void az_draw_doodad(az_doodad_kind_t doodad_kind, az_clock_t clock) {
  assert(0 <= (int)doodad_kind && (int)doodad_kind < AZ_NUM_DOODAD_KINDS);
  switch (doodad_kind) {
    case AZ_DOOD_WARNING_LIGHT:
      glPushMatrix(); {
        glRotatef(-4.0f * az_clock_mod(90, 1, clock), 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.75, 0.75, 0.75);
          glVertex2f(0, 0);
          glColor3f(0.25, 0.25, 0.25);
          for (int i = 0; i <= 360; i += 30) {
            glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        for (int offset = 0; offset <= 180; offset += 180) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(1, 0, 0);
            glVertex2f(0, 0);
            for (int i = offset - 30; i <= offset + 30; i += 30) {
              glVertex2d(3 * cos(AZ_DEG2RAD(i)), 3 * sin(AZ_DEG2RAD(i)));
            }
          } glEnd();
        }
        glBegin(GL_TRIANGLES); {
          glColor4f(1, 0, 0, 0);
          glVertex2f(69, -40); glVertex2f(69, 40);
          glColor4f(1, 0, 0, 0.5);
          glVertex2f(0, 0); glVertex2f(0, 0);
          glColor4f(1, 0, 0, 0);
          glVertex2f(-69, -40); glVertex2f(-69, 40);
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_DOOD_PIPE_STRAIGHT:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-25, 5); glVertex2f(25, 5);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(-25, 0); glVertex2f(25, 0);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-25, -5); glVertex2f(25, -5);
      } glEnd();
      // Coupling:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(25, 6); glVertex2f(31, 6);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(25, 0); glVertex2f(31, 0);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(25, -6); glVertex2f(31, -6);
      } glEnd();
      break;
    case AZ_DOOD_PIPE_CORNER:
      for (int i = 180; i < 270; i += 15) {
        glBegin(GL_QUAD_STRIP); {
          const double c1 = cos(AZ_DEG2RAD(i));
          const double s1 = sin(AZ_DEG2RAD(i));
          const double c2 = cos(AZ_DEG2RAD(i + 15));
          const double s2 = sin(AZ_DEG2RAD(i + 15));
          glColor3f(0.1, 0.4, 0.1);
          glVertex2d(3 * c1, 3 * s1); glVertex2d(3 * c2, 3 * s2);
          glColor3f(0.65, 0.9, 0.65);
          glVertex2d(8 * c1, 8 * s1); glVertex2d(8 * c2, 8 * s2);
          glColor3f(0.1, 0.4, 0.1);
          glVertex2d(13 * c1, 13 * s1); glVertex2d(13 * c2, 13 * s2);
        } glEnd();
      }
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(0, -3); glVertex2f(2, -3);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(0, -8); glVertex2f(2, -8);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(0, -13); glVertex2f(2, -13);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-3, 0); glVertex2f(-3, 2);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(-8, 0); glVertex2f(-8, 2);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-13, 0); glVertex2f(-13, 2);
      } glEnd();
      // Couplings:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(-2, 2); glVertex2f(-2, 8);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(-8, 2); glVertex2f(-8, 8);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(-14, 2); glVertex2f(-14, 8);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(2, -2); glVertex2f(8, -2);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(2, -8); glVertex2f(8, -8);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(2, -14); glVertex2f(8, -14);
      } glEnd();
      break;
    case AZ_DOOD_PIPE_TEE:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(5, -10); glVertex2f(5, 10);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(0, -10); glVertex2f(0, 10);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-5, -10); glVertex2f(-5, 10);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(5, 5); glVertex2f(10, 5);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(0, 0); glVertex2f(10, 0);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(5, -5); glVertex2f(10, -5);
      } glEnd();
      // Couplings:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(6, 10); glVertex2f(6, 16);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(0, 10); glVertex2f(0, 16);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(-6, 10); glVertex2f(-6, 16);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(6, -10); glVertex2f(6, -16);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(0, -10); glVertex2f(0, -16);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(-6, -10); glVertex2f(-6, -16);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(10, 6); glVertex2f(16, 6);
        glColor3f(0.75, 0.85, 0.75);
        glVertex2f(10, 0); glVertex2f(16, 0);
        glColor3f(0.2, 0.35, 0.2);
        glVertex2f(10, -6); glVertex2f(16, -6);
      } glEnd();
      break;
    case AZ_DOOD_PIPE_ELBOW:
      for (int i = 180; i < 225; i += 15) {
        glBegin(GL_QUAD_STRIP); {
          const double c1 = cos(AZ_DEG2RAD(i));
          const double s1 = sin(AZ_DEG2RAD(i));
          const double c2 = cos(AZ_DEG2RAD(i + 15));
          const double s2 = sin(AZ_DEG2RAD(i + 15));
          glColor3f(0.1, 0.4, 0.1);
          glVertex2d(3 * c1, 3 * s1); glVertex2d(3 * c2, 3 * s2);
          glColor3f(0.65, 0.9, 0.65);
          glVertex2d(8 * c1, 8 * s1); glVertex2d(8 * c2, 8 * s2);
          glColor3f(0.1, 0.4, 0.1);
          glVertex2d(13 * c1, 13 * s1); glVertex2d(13 * c2, 13 * s2);
        } glEnd();
      }
      glPushMatrix(); {
        for (int i = 0; i < 2; ++i) {
          if (i != 0) {
            glScalef(1, -1, 1);
            glRotatef(-45, 0, 0, 1);
          }
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0.1, 0.4, 0.1);
            glVertex2f(-3, 0); glVertex2f(-3, 5);
            glColor3f(0.65, 0.9, 0.65);
            glVertex2f(-8, 0); glVertex2f(-8, 5);
            glColor3f(0.1, 0.4, 0.1);
            glVertex2f(-13, 0); glVertex2f(-13, 5);
          } glEnd();
          // Coupling:
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0.2, 0.35, 0.2);
            glVertex2f(-2, 5); glVertex2f(-2, 11);
            glColor3f(0.75, 0.85, 0.75);
            glVertex2f(-8, 5); glVertex2f(-8, 11);
            glColor3f(0.2, 0.35, 0.2);
            glVertex2f(-14, 5); glVertex2f(-14, 11);
          } glEnd();
        }
      } glPopMatrix();
      break;
    case AZ_DOOD_PIPE_SHORT:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-10, 5); glVertex2f(10, 5);
        glColor3f(0.65, 0.9, 0.65);
        glVertex2f(-10, 0); glVertex2f(10, 0);
        glColor3f(0.1, 0.4, 0.1);
        glVertex2f(-10, -5); glVertex2f(10, -5);
      } glEnd();
      break;
    case AZ_DOOD_SUPPLY_BOX:
      glColor3f(0.28, 0.28, 0.28);
      glBegin(GL_POLYGON); {
        glVertex2f( 20,  15); glVertex2f(-20,  15);
        glVertex2f(-20, -15); glVertex2f( 20, -15);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); glVertex2f( 20,  15);
        glColor3f(0.15, 0.15, 0.15); glVertex2f( 25,  20);

        glColor3f(0.25, 0.25, 0.25); glVertex2f(-14,  15);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-19,  20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-18,  13);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-23,  18);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-20,   9);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-25,  14);

        glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, -15);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-25, -20);

        glColor3f(0.25, 0.25, 0.25); glVertex2f( 14, -15);
        glColor3f(0.15, 0.15, 0.15); glVertex2f( 19, -20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f( 18, -13);
        glColor3f(0.15, 0.15, 0.15); glVertex2f( 23, -18);
        glColor3f(0.25, 0.25, 0.25); glVertex2f( 20,  -9);
        glColor3f(0.15, 0.15, 0.15); glVertex2f( 25, -14);

        glColor3f(0.25, 0.25, 0.25); glVertex2f( 20,  15);
        glColor3f(0.15, 0.15, 0.15); glVertex2f( 25,  20);
      } glEnd();
      break;
    case AZ_DOOD_RED_TUBE_WINDOW:
      // Glass:
      glBegin(GL_QUADS); {
        glColor4f(1, 1, 1, 0.25); glVertex2f(-85, 64);
        glColor4f(1, 1, 1, 0.35); glVertex2f(-85, 0);
        glColor4f(1, 1, 1, 0.2); glVertex2f(85, 0); glVertex2f(85, 64);
        glVertex2f(85, -64); glVertex2f(85, 0);
        glColor4f(1, 1, 1, 0.35); glVertex2f(-85, 0);
        glColor4f(1, 1, 1, 0.25); glVertex2f(-85, -64);
      } glEnd();
      // Siding:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(100, 78); glVertex2f(85, 64);
        glVertex2f(-100, 78); glVertex2f(-85, 64);
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(-100, 0); glVertex2f(-85, 0);
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(-100, -78); glVertex2f(-85, -64);
        glVertex2f(100, -78); glVertex2f(85, -64);
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(100, 0); glVertex2f(85, 0);
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(100, 78); glVertex2f(85, 64);
      } glEnd();
      break;
    case AZ_DOOD_OCTO_WINDOW:
      // Glass:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.5); glVertex2f(-0, 0); glColor4f(1, 1, 1, 0.2);
        glVertex2f(47, 68); glVertex2f(-47, 68); glVertex2f(-74, 41);
        glVertex2f(-74, -41); glVertex2f(-47, -68); glVertex2f(47, -68);
        glVertex2f(74, -41); glVertex2f(74, 41); glVertex2f(47, 68);
      } glEnd();
      // Siding:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.75, 0.75, 0.375);
        glVertex2f(47, 68); glVertex2f(37, 48);
        glVertex2f(-47, 68); glVertex2f(-37, 48);
        glVertex2f(-74, 41); glVertex2f(-54, 31);
        glVertex2f(-74, -41); glVertex2f(-54, -31);
        glVertex2f(-47, -68); glVertex2f(-37, -48);
        glVertex2f(47, -68); glVertex2f(37, -48);
        glVertex2f(74, -41); glVertex2f(54, -31);
        glVertex2f(74, 41); glVertex2f(54, 31);
        glVertex2f(47, 68); glVertex2f(37, 48);
      } glEnd();
      break;
    case AZ_DOOD_MACHINE_FAN:
      glBegin(GL_QUAD_STRIP); {
        for (int i = 45; i <= 405; i += 90) {
          const double c = cos(AZ_DEG2RAD(i));
          const double s = sin(AZ_DEG2RAD(i));
          glColor3f(0.25, 0.25, 0.25); glVertex2d(45.25 * c, 45.25 * s);
          glColor3f(0.15, 0.15, 0.15); glVertex2d(55 * c, 55 * s);
        }
      } glEnd();
      glColor3f(0.28, 0.28, 0.28);
      glBegin(GL_QUADS); {
        glVertex2f( 32,  32); glVertex2f(-32,  32);
        glVertex2f(-32, -32); glVertex2f( 32, -32);
      } glEnd();
      glColor3f(0.15, 0.15, 0.15);
      glBegin(GL_POLYGON); {
        for (int i = 0; i < 360; i += 15) {
          glVertex2d(28 * cos(AZ_DEG2RAD(i)), 28 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glPushMatrix(); {
        glRotatef(5.0f * az_clock_mod(72, 1, clock), 0, 0, 1);
        for (int i = 0; i < 5; ++i) {
          glRotatef(72, 0, 0, 1);
          glBegin(GL_TRIANGLES); {
            glColor3f(0.4, 0.4, 0.4);
            glVertex2f(0, 0); glVertex2f(28, 0);
            glColor3f(0.2, 0.2, 0.2);
            glVertex2f(19, 19);
          } glEnd();
        }
      } glPopMatrix();
      break;
    case AZ_DOOD_YELLOW_TUBE_INSIDE:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.6, 0.6, 0.5); glVertex2f(-70,  70); glVertex2f(70,  70);
        glColor3f(0.4, 0.4, 0.3); glVertex2f(-67,  45); glVertex2f(67,  45);
        glColor3f(0.2, 0.2, 0.15); glVertex2f(-65,  0); glVertex2f(65,   0);
        glColor3f(0.4, 0.4, 0.3); glVertex2f(-67, -45); glVertex2f(67, -45);
        glColor3f(0.6, 0.6, 0.5); glVertex2f(-70, -70); glVertex2f(70, -70);
      } glEnd();
      break;
    case AZ_DOOD_NPS_ENGINE:
      // Siding:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-25, -25);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-30, -30);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(25, -25);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(28, -30);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(33, -17);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(38, -20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(33, 17);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(38, 20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(25, 25);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(28, 30);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-25, 25);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-30, 30);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-25, -25);
        glColor3f(0.15, 0.15, 0.15); glVertex2f(-30, -30);
      } glEnd();
      // Front panel:
      glBegin(GL_POLYGON); {
        glColor3f(0.28, 0.28, 0.28);
        glVertex2f(-25, -25); glVertex2f(25, -25); glVertex2f(33, -17);
        glVertex2f(33, 17); glVertex2f(25, 25); glVertex2f(-25, 25);
      } glEnd();
      // Pistons:
      for (int i = 0; i < 2; ++i) {
        glBegin(GL_QUAD_STRIP); {
          const double pump = (1.0 / 60) * az_clock_mod(60, 1, clock + 30 * i);
          const GLfloat x = (pump < 0.9 ? pump * 30.0 : (1.0 - pump) * 270.0);
          const GLfloat y = 20 * i - 10;
          glColor3f(0.2, 0.2, 0.2);
          glVertex2f(37, y + 6); glVertex2f(36 + x, y + 6);
          glColor3f(0.6, 0.6, 0.6); glVertex2f(36, y); glVertex2f(36 + x, y);
          glColor3f(0.2, 0.2, 0.2);
          glVertex2f(37, y - 6); glVertex2f(36 + x, y - 6);
        } glEnd();
      }
      // Lights:
      for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
          glPushMatrix(); {
            glTranslatef(14 * i, 14 * j, 0);
            // Light bulb:
            glBegin(GL_TRIANGLE_FAN); {
              if ((i * j < 0) ^ (az_clock_mod(2, 37, clock) == 0)) {
                glColor3f(1, 0.25, 0);
              } else glColor3f(0.2, 0.2, 0.2);
              glVertex2f(0, 0);
              glColor3f(0, 0, 0);
              for (int k = 0; k <= 360; k += 20) {
                glVertex2d(4 * cos(AZ_DEG2RAD(k)), 4 * sin(AZ_DEG2RAD(k)));
              }
            } glEnd();
            // Rim:
            glBegin(GL_QUAD_STRIP); {
              for (int k = 0; k <= 360; k += 20) {
                glColor3f(0.45, 0.45, 0.45);
                glVertex2d(4 * cos(AZ_DEG2RAD(k)), 4 * sin(AZ_DEG2RAD(k)));
                glColor3f(0.35, 0.35, 0.35);
                glVertex2d(6 * cos(AZ_DEG2RAD(k)), 6 * sin(AZ_DEG2RAD(k)));
              }
            } glEnd();
          } glPopMatrix();
        }
      }
      // Spinner:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.3, 0.3, 0.3); glVertex2f(0, 0); glColor3f(0, 0, 0);
        for (int k = 0; k <= 360; k += 90) {
          glVertex2d(7 * cos(AZ_DEG2RAD(k)), 7 * sin(AZ_DEG2RAD(k)));
        }
      } glEnd();
      glBegin(GL_TRIANGLES); {
        const int k = -90 * az_clock_mod(8, 14, clock);
        glColor3f(0.7, 0.7, 0); glVertex2d(0, 0); glColor3f(0.4, 0.4, 0);
        glVertex2d(6 * cos(AZ_DEG2RAD(k)), 6 * sin(AZ_DEG2RAD(k)));
        glVertex2d(6 * cos(AZ_DEG2RAD(k + 90)), 6 * sin(AZ_DEG2RAD(k + 90)));
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        for (int k = 0; k <= 360; k += 90) {
          glColor3f(0.45, 0.45, 0.45);
          glVertex2d(7 * cos(AZ_DEG2RAD(k)), 7 * sin(AZ_DEG2RAD(k)));
          glColor3f(0.35, 0.35, 0.35);
          glVertex2d(10 * cos(AZ_DEG2RAD(k)), 10 * sin(AZ_DEG2RAD(k)));
        }
      } glEnd();
      break;
    case AZ_DOOD_DRILL_TIP:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.4, 0.4, 0.4); glVertex2d(0, 0); glColor3f(0.1, 0.1, 0.1);
        glVertex2d(0, 30); glVertex2d(-30, 0); glVertex2d(0, -30);
      } glEnd();
      break;
    case AZ_DOOD_DRILL_SHAFT_STILL:
      draw_drill_shaft(3);
      break;
    case AZ_DOOD_DRILL_SHAFT_SPIN:
      draw_drill_shaft(clock);
      break;
    case AZ_DOOD_GRASS_TUFT_1:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-1.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(-6, 15);
        glColor3f(0,    0.3, 0); glVertex2f(-4, 15); glVertex2f(-12, 26);
        glColor3f(0.25, 0.5, 0); glVertex2f(-6, 15);
        glColor3f(0,    0.3, 0); glVertex2f(-8, 14);
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-6.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(2.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(1, 15);
        glColor3f(0,    0.3, 0); glVertex2f(3, 15); glVertex2f(4, 30);
        glColor3f(0.25, 0.5, 0); glVertex2f(1, 15);
        glColor3f(0,    0.3, 0); glVertex2f(-1, 15);
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-2.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(6.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(8, 14);
        glColor3f(0,    0.3, 0); glVertex2f(10, 14); glVertex2f(18, 5);
        glColor3f(0.25, 0.5, 0); glVertex2f(8, 14);
        glColor3f(0,    0.3, 0); glVertex2f(6, 14);
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(1.5, 0);
      } glEnd();
      break;
    case AZ_DOOD_GRASS_TUFT_2:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-1.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(-8, 17);
        glColor3f(0,    0.3, 0); glVertex2f(-6, 17); glVertex2f(-9, 30);
        glColor3f(0.25, 0.5, 0); glVertex2f(-8, 17);
        glColor3f(0,    0.3, 0); glVertex2f(-10, 16);
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-6.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(2.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(0, 18);
        glColor3f(0,    0.3, 0); glVertex2f(2, 18); glVertex2f(-4, 36);
        glColor3f(0.25, 0.5, 0); glVertex2f(0, 18);
        glColor3f(0,    0.3, 0); glVertex2f(-2, 18);
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-2.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(6.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(8, 12);
        glColor3f(0,    0.3, 0); glVertex2f(10, 12); glVertex2f(7, 24);
        glColor3f(0.25, 0.5, 0); glVertex2f(8, 12);
        glColor3f(0,    0.3, 0); glVertex2f(6, 12);
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(1.5, 0);
      } glEnd();
      break;
    case AZ_DOOD_GRASS_TUFT_3:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-6.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(-8, 12);
        glColor3f(0,    0.3, 0); glVertex2f(-10, 11); glVertex2f(-16, 17);
        glColor3f(0.25, 0.5, 0); glVertex2f(-8, 12);
        glColor3f(0,    0.3, 0); glVertex2f(-6, 13);
        glColor3f(0.25, 0.5, 0); glVertex2f(-4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-1.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(-2.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(-1, 15);
        glColor3f(0,    0.3, 0); glVertex2f(-3, 15); glVertex2f(2, 30);
        glColor3f(0.25, 0.5, 0); glVertex2f(-1, 15);
        glColor3f(0,    0.3, 0); glVertex2f(1, 15);
        glColor3f(0.25, 0.5, 0); glVertex2f(0,  0);
        glColor3f(0,    0.3, 0); glVertex2f(2.5, 0);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(1.5, 0);
        glColor3f(0.25, 0.5, 0); glVertex2f(6, 15);
        glColor3f(0,    0.3, 0); glVertex2f(4, 15); glVertex2f(12, 26);
        glColor3f(0.25, 0.5, 0); glVertex2f(6, 15);
        glColor3f(0,    0.3, 0); glVertex2f(8, 14);
        glColor3f(0.25, 0.5, 0); glVertex2f(4,  0);
        glColor3f(0,    0.3, 0); glVertex2f(6.5, 0);
      } glEnd();
      break;
    case AZ_DOOD_RED_TUBE_BROKEN_WINDOW:
      // Broken glass:
      glBegin(GL_TRIANGLES); {
        glColor4f(1, 1, 1, 0.31);
        glVertex2f( 60,  64); glVertex2f( 30,  64); glVertex2f( 40,  49);
        glVertex2f( 30,  64); glVertex2f( 10,  64); glVertex2f( 18,  42);
        glVertex2f( 10,  64); glVertex2f( -5,  64); glVertex2f(  4,  53);
        glVertex2f( -5,  64); glVertex2f(-48,  64); glVertex2f(-25,  50);
        glVertex2f(-48,  64); glVertex2f(-82,  64); glVertex2f(-60,  56);
        glVertex2f(-85,  60); glVertex2f(-85,  37); glVertex2f(-55,  45);
        glVertex2f(-85,  37); glVertex2f(-85,  24); glVertex2f(-76,  29);
        glVertex2f(-85,  24); glVertex2f(-85,   1); glVertex2f(-70,  15);
        glVertex2f(-85,   1); glVertex2f(-85, -15); glVertex2f(-74, -12);
        glVertex2f(-85, -15); glVertex2f(-85, -47); glVertex2f(-59, -27);
        glVertex2f(-85, -50); glVertex2f(-85, -64); glVertex2f(-62, -45);
        glVertex2f(-85, -64); glVertex2f(-55, -64); glVertex2f(-62, -45);
        glVertex2f(-55, -64); glVertex2f(-32, -64); glVertex2f(-40, -42);
        glVertex2f(-32, -64); glVertex2f(-18, -64); glVertex2f(-26, -56);
        glVertex2f(-18, -64); glVertex2f(  0, -64); glVertex2f( -6, -52);
        glVertex2f(  0, -64); glVertex2f( 12, -64); glVertex2f(  7, -54);
      } glEnd();
      // Siding:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(76, 78); glVertex2f(68, 64);
        glVertex2f(-100, 78); glVertex2f(-85, 64);
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(-100, 0); glVertex2f(-85, 0);
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(-100, -78); glVertex2f(-85, -64);
        glVertex2f(30, -78); glVertex2f(18, -64);
      } glEnd();
      break;
    case AZ_DOOD_SHORT_CHAIN:
      draw_chain_link_pair();
      break;
    case AZ_DOOD_LONG_CHAIN:
      glPushMatrix(); {
        glTranslatef(-45, 0, 0);
        for (int i = 0; i < 3; ++i) {
          draw_chain_link_pair();
          glTranslatef(30, 0, 0);
        }
      } glPopMatrix();
      break;
    case AZ_DOOD_NUCLEAR_BOMB:
      // Main cylinder:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-36,  30); glVertex2f(36,  30);
        glColor3f(0.50, 0.50, 0.50); glVertex2f(-36,  18); glVertex2f(36,  18);
        glColor3f(0.70, 0.70, 0.70); glVertex2f(-36,   0); glVertex2f(36,   0);
        glColor3f(0.50, 0.50, 0.50); glVertex2f(-36, -18); glVertex2f(36, -18);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-36, -30); glVertex2f(36, -30);
      } glEnd();
      // End caps:
      for (int j = -1; j <= 1; j += 2) {
        float x_1 = j * 40;
        float y_1 = 30;
        for (int i = 1; i <= 4; ++i) {
          const float x_2 = x_1 + j * 2;
          const float y_2 = 8 * cos(AZ_DEG2RAD(i * 20)) + 22;
          glBegin(GL_TRIANGLE_STRIP); {
            glColor3f(0.25, 0.25, 0.25);
            glVertex2f(x_1, y_1); glVertex2f(x_2, y_2);
            glColor3f(0.50, 0.50, 0.50);
            glVertex2f(x_1, 0.6f * y_1); glVertex2f(x_2, 0.6f * y_2);
            glColor3f(0.70, 0.70, 0.70);
            glVertex2f(x_1, 0); glVertex2f(x_2, 0);
            glColor3f(0.50, 0.50, 0.50);
            glVertex2f(x_1, -0.6f * y_1); glVertex2f(x_2, -0.6f * y_2);
            glColor3f(0.25, 0.25, 0.25);
            glVertex2f(x_1, -y_1); glVertex2f(x_2, -y_2);
          } glEnd();
          x_1 = x_2;
          y_1 = y_2;
        }
        glBegin(GL_TRIANGLE_STRIP); {
          glColor3f(0.20, 0.20, 0.20);
          glVertex2f(j * 36,  31); glVertex2f(j * 40,  31);
          glColor3f(0.45, 0.45, 0.45);
          glVertex2f(j * 36,  19); glVertex2f(j * 40,  19);
          glColor3f(0.65, 0.65, 0.65);
          glVertex2f(j * 36,   0); glVertex2f(j * 40,   0);
          glColor3f(0.45, 0.45, 0.45);
          glVertex2f(j * 36, -19); glVertex2f(j * 40, -19);
          glColor3f(0.20, 0.20, 0.20);
          glVertex2f(j * 36, -31); glVertex2f(j * 40, -31);
        } glEnd();
      }
      // Radiation symbol:
      glColor4f(1, 1, 0, 0.3);
      glBegin(GL_TRIANGLE_FAN); {
        for (int i = 30; i < 360; i += 60) {
          glVertex2d(16 * cos(AZ_DEG2RAD(i)), 15.5 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glColor4f(0, 0, 0, 0.65);
      glBegin(GL_TRIANGLE_FAN); {
        glVertex2d(0, 0);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(2.5 * cos(AZ_DEG2RAD(i)), 2.5 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      for (int j = 60; j < 420; j += 120) {
        glBegin(GL_TRIANGLE_STRIP); {
          for (int i = j - 30; i <= j + 30; i += 10) {
            glVertex2d(4.5 * cos(AZ_DEG2RAD(i)), 4.5 * sin(AZ_DEG2RAD(i)));
            glVertex2d(11 * cos(AZ_DEG2RAD(i)), 10.5 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      }
      break;
    case AZ_DOOD_HANGING_VINE:
      glBegin(GL_TRIANGLE_STRIP); {
        for (int i = 0; i <= 60; i += 3) {
          const GLfloat deviation = 0.1 * ((29 * i) % 12);
          glColor3f(0.3, 0.5, 0); glVertex2f(30 - i, deviation);
          glColor3f(0.1, 0.25, 0); glVertex2f(30 - i, -3.0f + deviation);
        }
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        for (int i = 0; i <= 60; i += 3) {
          const GLfloat deviation = 0.1 * ((29 * i) % 12);
          glColor3f(0.3, 0.5, 0); glVertex2f(30 - i, deviation);
          glColor3f(0.1, 0.25, 0); glVertex2f(30 - i, 3.0f + deviation);
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.3, 0.5, 0); glVertex2f(30, 0); glColor3f(0.1, 0.25, 0);
        for (int i = -90; i <= 90; i += 30) {
          glVertex2d(30 + 5 * cos(AZ_DEG2RAD(i)), 3 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glPushMatrix(); {
        glTranslatef(-26, 0, 0);
        for (int i = 0; i < 8; ++i) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.4, 0.6, 0); glVertex2f(0, 5); glColor3f(0.2, 0.4, 0);
            glVertex2f(0, 0); glVertex2f(3, 2); glVertex2f(5, 7);
            glVertex2f(5 + (i % 3) +
                       0.1 * az_clock_zigzag(15, 2, clock + 4 * i),
                       15 + ((i + 1) % 3));
            glVertex2f(-2, 11); glVertex2f(-3, 7);
            glVertex2f(-1, 2); glVertex2f(0, 0);
          } glEnd();
          glTranslatef(6.6 + (i % 2), (i % 3) - 1, 0);
          glScalef(1, -1, 1);
        }
      } glPopMatrix();
      break;
    case AZ_DOOD_RED_TUBE_WINDOW_BEND:
      // Glass:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor4f(1, 1, 1, 0.2); glVertex2f(-32.31, -64);
        glColor4f(1, 1, 1, 0.3); glVertex2f(-32.31, 0);
        glColor4f(1, 1, 1, 0.2); glVertex2f(26.51, -64);
        glColor4f(1, 1, 1, 0.3); glVertex2f(0, 0);
        glColor4f(1, 1, 1, 0.25); glVertex2f(68.10, -22.41);
        glColor4f(1, 1, 1, 0.35); glVertex2f(22.85, 22.85);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor4f(1, 1, 1, 0.2); glVertex2f(-32.31, 64);
        glColor4f(1, 1, 1, 0.3); glVertex2f(-32.31, 0);
        glColor4f(1, 1, 1, 0.2); glVertex2f(-26.51, 64);
        glColor4f(1, 1, 1, 0.3); glVertex2f(0, 0);
        glColor4f(1, 1, 1, 0.2); glVertex2f(-22.41, 68.10);
        glColor4f(1, 1, 1, 0.35); glVertex2f(22.85, 22.85);
      } glEnd();
      // Siding:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(-47.31, 0); glVertex2f(-32.31, 0);
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(-47.31, -78); glVertex2f(-32.31, -64);
        glVertex2f(32.31, -78); glVertex2f(26.51, -64);
        glVertex2f(88.61, -21.70); glVertex2f(68.10, -22.41);
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(33.45, 33.45); glVertex2f(22.85, 22.85);
        glColor3f(0.5, 0.44, 0.44);
        glVertex2f(-21.70, 88.61); glVertex2f(-22.41, 68.10);
        glVertex2f(-32.31, 78); glVertex2f(-26.51, 64);
        glVertex2f(-47.31, 78); glVertex2f(-32.31, 64);
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(-47.31, 0); glVertex2f(-32.31, 0);
      } glEnd();
      break;
    case AZ_DOOD_DOOR_PIPE:
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
      break;
    case AZ_DOOD_RED_TUBE_INSIDE:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.6, 0.5, 0.5); glVertex2f(-110,  70); glVertex2f(110,  70);
        glColor3f(0.4, 0.3, 0.3); glVertex2f(-107,  45); glVertex2f(107,  45);
        glColor3f(0.2, 0.15, 0.15); glVertex2f(-105, 0); glVertex2f(105,   0);
        glColor3f(0.4, 0.3, 0.3); glVertex2f(-107, -45); glVertex2f(107, -45);
        glColor3f(0.6, 0.5, 0.5); glVertex2f(-110, -70); glVertex2f(110, -70);
      } glEnd();
      break;
  }
}

/*===========================================================================*/
