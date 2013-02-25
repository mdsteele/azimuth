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

#include "azimuth/view/node.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/node.h"
#include "azimuth/state/space.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

// Helper function for drawing upgrade icons for capacitors and shield
// batteries.
static void draw_tank(void) {
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.2, 0.2, 0.2);
    glVertex2f(-7, 9); glVertex2f(7, 9);
    glColor3f(0.45, 0.45, 0.45);
    glVertex2f(-7, 0); glVertex2f(7, 0);
    glColor3f(0.2, 0.2, 0.2);
    glVertex2f(-7, -9); glVertex2f(7, -9);
  } glEnd();
  for (int sign = -1; sign <= 1; sign += 2) {
    glBegin(GL_QUAD_STRIP); {
      glColor3f(0.15, 0.15, 0.15);
      glVertex2i(11 * sign, 6); glVertex2i(7 * sign, 9);
      glColor3f(0.35, 0.35, 0.35);
      glVertex2i(11 * sign, 0); glVertex2i(7 * sign, 0);
      glColor3f(0.15, 0.15, 0.15);
      glVertex2i(11 * sign, -6); glVertex2i(7 * sign, -9);
    } glEnd();
  }
}

static void draw_upgrade_icon(az_upgrade_t upgrade, az_clock_t clock) {
  const int frame = az_clock_mod(4, 10, clock);
  switch (upgrade) {
    case AZ_UPG_GUN_CHARGE:
      for (int n = 0; n < 2; ++n) {
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(1, 1, 0.5, 0.7);
          glVertex2d(0, 0);
          glColor4f(1, 1, 1, 0.0);
          for (int i = 0; i <= 360; i += 60) {
            const double radius = 13.0;
            const int offset = 15 * frame;
            const double degrees = (n == 0 ? i + offset : i - offset);
            glVertex2d(radius * cos(AZ_DEG2RAD(degrees)),
                       radius * sin(AZ_DEG2RAD(degrees)));
          }
        } glEnd();
      }
      break;
    case AZ_UPG_GUN_FREEZE:
      glBegin(GL_TRIANGLES); {
        const double radius = 12.0;
        const bool flash1 = frame % 2 != 0;
        const bool flash2 = frame / 2 != 0;
        glColor3f(0, (flash2 ? 0.5 : 1), 1);
        for (int i = 0; i < 3; ++i) {
          glVertex2d(radius * cos(AZ_DEG2RAD(120 * i + (flash1 ? 30 : 90))),
                     radius * sin(AZ_DEG2RAD(120 * i + (flash1 ? 30 : 90))));
        }
        glColor3f(0, (flash2 ? 1 : 0.5), 1);
        for (int i = 0; i < 3; ++i) {
          glVertex2d(radius * cos(AZ_DEG2RAD(120 * i + (flash1 ? 90 : 30))),
                     radius * sin(AZ_DEG2RAD(120 * i + (flash1 ? 90 : 30))));
        }
      } glEnd();
      break;
    case AZ_UPG_GUN_TRIPLE:
      glBegin(GL_QUADS); {
        glColor3f((frame == 1), 1, (frame == 1));
        glVertex2d(2, 11); glVertex2d(-2, 11);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(-2, -12); glVertex2d(2, -12);
        glColor3f((frame == 2), 1, (frame == 2));
        glVertex2d(12, 9); glVertex2d(8, 9);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(2, -12); glVertex2d(5, -12);
        glColor3f((frame == 3), 1, (frame == 3));
        glVertex2d(-8, 9); glVertex2d(-12, 9);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(-5, -11); glVertex2d(-2, -11);
      } glEnd();
      break;
    case AZ_UPG_GUN_PHASE:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0.5, 0, 0);
        glVertex2d(-10, -10);
        glColor3f(1, 1, 0.5);
        const double radius = 10.0 + 4.5 * frame;
        for (int i = 15; i <= 75; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)) - 12,
                     radius * sin(AZ_DEG2RAD(i)) - 12);
        }
      } glEnd();
      break;
    case AZ_UPG_GUN_BURST:
      glColor3f(1, 0.8, 0.75);
      glBegin(GL_QUADS); {
        for (int i = 10; i < 370; i += 45) {
          const az_vector_t vec = az_vpolar(1 + 3 * frame, AZ_DEG2RAD(i));
          glVertex2d(vec.x + 2, vec.y); glVertex2d(vec.x, vec.y + 2);
          glVertex2d(vec.x - 2, vec.y); glVertex2d(vec.x, vec.y - 2);
        }
      } glEnd();
      glPushMatrix(); {
        glRotatef(45 * frame, 0, 0, 1);
        glBegin(GL_QUADS); {
          glColor3f(0.75, 0.5, 0.25); // brown
          glVertex2f( 2.6, -4); glVertex2f( 6.6, 0); glVertex2f( 2.6,  4);
          glColor3f(0.5, 0.25, 0); // dark brown
          glVertex2f(-1.3, 0); glVertex2f( 1.3, 0);
          glColor3f(0.75, 0.5, 0.25); // brown
          glVertex2f(-2.6,  4); glVertex2f(-6.6, 0); glVertex2f(-2.6, -4);
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_UPG_GUN_BEAM:
      glBegin(GL_QUAD_STRIP); {
        glColor4f(1, 0, 0, 0.2 * (frame == 3 ? 1 : frame));
        glVertex2i(-12,  -6); glVertex2i( 6, 12);
        glColor3f(1, 0.2, 0.2);
        glVertex2i(-12, -12); glVertex2i(12, 12);
        glColor4f(1, 0, 0, 0.2 * (frame == 3 ? 1 : frame));
        glVertex2i( -6, -12); glVertex2i(12,  6);
      } glEnd();
      break;
    case AZ_UPG_ROCKET_AMMO_00:
    case AZ_UPG_ROCKET_AMMO_01:
    case AZ_UPG_ROCKET_AMMO_02:
    case AZ_UPG_ROCKET_AMMO_03:
    case AZ_UPG_ROCKET_AMMO_04:
    case AZ_UPG_ROCKET_AMMO_05:
    case AZ_UPG_ROCKET_AMMO_06:
    case AZ_UPG_ROCKET_AMMO_07:
    case AZ_UPG_ROCKET_AMMO_08:
    case AZ_UPG_ROCKET_AMMO_09:
    case AZ_UPG_ROCKET_AMMO_10:
    case AZ_UPG_ROCKET_AMMO_11:
    case AZ_UPG_ROCKET_AMMO_12:
    case AZ_UPG_ROCKET_AMMO_13:
    case AZ_UPG_ROCKET_AMMO_14:
    case AZ_UPG_ROCKET_AMMO_15:
    case AZ_UPG_ROCKET_AMMO_16:
    case AZ_UPG_ROCKET_AMMO_17:
    case AZ_UPG_ROCKET_AMMO_18:
    case AZ_UPG_ROCKET_AMMO_19:
      glColor3f(0.5, 0, 0); // dark red
      glBegin(GL_QUADS); {
        const int x = 4 - 3 * frame;
        glVertex2i(x, -10);
        glVertex2i(x + 4, -10);
        glVertex2i(x + 4, -2);
        glVertex2i(x, -2);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(-4, -9);
        glVertex2i(-4, 6);
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, -9);
        glVertex2i(0, 10);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(4, -9);
        glVertex2i(4, 6);
      } glEnd();
      glColor3f(0.5, 0, 0); // dark red
      glBegin(GL_QUADS); {
        const int x = -8 + 3 * frame;
        glVertex2i(x, -10);
        glVertex2i(x + 4, -10);
        glVertex2i(x + 4, -2);
        glVertex2i(x, -2);
      } glEnd();
      break;
    case AZ_UPG_HYPER_ROCKETS:
      glColor3f(0.5, 0, 0.5); // dark magenta
      glBegin(GL_QUADS); {
        const int x = -8 + 3 * frame;
        glVertex2i(x, (frame >= 2 ? -9 : -10));
        glVertex2i(x + 4, (frame >= 2 ? -10 : -9));
        glVertex2i(x + 4, (frame >= 2 ? -3 : -1));
        glVertex2i(x, (frame >= 2 ? -1 : -3));
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(-4, -8);
        glVertex2i(-4, 1);
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, -9);
        glVertex2i(0, 10);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2i(4, -8);
        glVertex2i(4, 1);
      } glEnd();
      glColor3f(0.6, 0, 0.6); // dark magenta
      glBegin(GL_QUADS); {
        const int x = 4 - 3 * frame;
        glVertex2i(x, (frame < 2 ? -9 : -10));
        glVertex2i(x + 4, (frame < 2 ? -10 : -9));
        glVertex2i(x + 4, (frame < 2 ? -3 : -1));
        glVertex2i(x, (frame < 2 ? -1 : -3));
      } glEnd();
      break;
    case AZ_UPG_BOMB_AMMO_00:
    case AZ_UPG_BOMB_AMMO_01:
    case AZ_UPG_BOMB_AMMO_02:
    case AZ_UPG_BOMB_AMMO_03:
    case AZ_UPG_BOMB_AMMO_04:
    case AZ_UPG_BOMB_AMMO_05:
    case AZ_UPG_BOMB_AMMO_06:
    case AZ_UPG_BOMB_AMMO_07:
    case AZ_UPG_BOMB_AMMO_08:
    case AZ_UPG_BOMB_AMMO_09:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, 0);
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          const double radius = 8;
          const double theta = AZ_DEG2RAD(i - 30 * frame);
          if (blue) glColor3f(0, 0, 0.75); // blue
          else glColor3f(0.5, 0.5, 0.5); // gray
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      break;
    case AZ_UPG_MEGA_BOMBS:
      glBegin(GL_TRIANGLE_FAN); {
        if (frame == 0) glColor3f(1, 1, 0.5); // yellow
        else glColor3f(0.5, 0.5, 0.5); // gray
        glVertex2i(0, 0);
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          const double radius = 9;
          const double theta = AZ_DEG2RAD(i + 30 * frame);
          if (blue) glColor3f(0, 0.5, 0.75); // cyan
          else if (frame == 0) glColor3f(0.75, 0.75, 0.25); // yellow
          else glColor3f(0.25, 0.25, 0.25); // dark gray
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      break;
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
      draw_tank();
      if (frame % 2) glColor3f(0, 0, 0);
      else glColor3f(0.5, 0, 0.5);
      glBegin(GL_LINES); {
        glVertex2f(-8, 0); glVertex2f(-2, 0);
        glVertex2f(-2, -4); glVertex2f(-2, 4);
        glVertex2f(3, 4); glVertex2f(1, 1);
        glVertex2f(1, 1); glVertex2f(1, -1);
        glVertex2f(1, -1); glVertex2f(3, -4);
        glVertex2f(1, 0); glVertex2f(8, 0);
      } glEnd();
      break;
    case AZ_UPG_QUANTUM_REACTOR:
      for (int i = -1; i <= 1; i += 2) {
        glPushMatrix(); {
          glTranslatef(i * 2, i * 2, 0);
          glRotatef(i * 45 * frame, 0, 0, 1);
          glScalef(i, 1, 1);
          glColor3f(0.5, 0.2, 1);
          glBegin(GL_TRIANGLES); {
            glVertex2f(-3, 6); glVertex2f(2, 4); glVertex2f(2, 8);
            glVertex2f(3, -6); glVertex2f(-2, -4); glVertex2f(-2, -8);
          } glEnd();
          glColor3f(0.2, 0.5, 1);
          glBegin(GL_POINTS); {
            glVertex2f(5, 1); glVertex2f(4, 4);
            glVertex2f(-5, -1); glVertex2f(-4, -4);
          } glEnd();
        } glPopMatrix();
      }
      break;
    case AZ_UPG_SHIELD_BATTERY_00:
    case AZ_UPG_SHIELD_BATTERY_01:
    case AZ_UPG_SHIELD_BATTERY_02:
    case AZ_UPG_SHIELD_BATTERY_03:
    case AZ_UPG_SHIELD_BATTERY_04:
    case AZ_UPG_SHIELD_BATTERY_05:
    case AZ_UPG_SHIELD_BATTERY_06:
    case AZ_UPG_SHIELD_BATTERY_07:
    case AZ_UPG_SHIELD_BATTERY_08:
    case AZ_UPG_SHIELD_BATTERY_09:
    case AZ_UPG_SHIELD_BATTERY_10:
    case AZ_UPG_SHIELD_BATTERY_11:
      draw_tank();
      if (frame % 2) glColor3f(0, 1, 0);
      else glColor3f(0, 0, 1);
      glBegin(GL_LINE_STRIP); {
        glVertex2f(4, 4); glVertex2f(-4, 4);
        glVertex2f(-4, 0); glVertex2f(4, 0);
        glVertex2f(4, -4); glVertex2f(-4, -4);
      } glEnd();
      break;
    // TODO: Draw other upgrade icons.
    default:
      glColor3f(1, (upgrade % 2), (upgrade % 4) / 2);
      glBegin(GL_QUADS); {
        glVertex2d(12, 12);
        glVertex2d(-12, 12);
        glVertex2d(-12, -12);
        glVertex2d(12, -12);
      } glEnd();
      break;
  }
  glBegin(GL_QUADS); {
    glColor4f(1, 1, 1, 0.25); glVertex2f(-12, 12);
    glColor4f(1, 1, 1, 0.15); glVertex2f(-12, -12);
    glColor4f(1, 1, 1, 0.1); glVertex2f(12, -12); glVertex2f(12, 12);
  } glEnd();
  glColor3f(1, az_clock_mod(2, 10, clock), 1);
  glBegin(GL_LINE_LOOP); {
    glVertex2f( 14,  14); glVertex2f(-14, 14);
    glVertex2f(-14, -14); glVertex2f(14, -14);
  } glEnd();
}

/*===========================================================================*/

static void draw_doodad(az_doodad_kind_t doodad_kind, az_clock_t clock) {
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
      for (int i = 180; i < 270; i += 30) {
        glBegin(GL_QUAD_STRIP); {
          const double c1 = cos(AZ_DEG2RAD(i));
          const double s1 = sin(AZ_DEG2RAD(i));
          const double c2 = cos(AZ_DEG2RAD(i + 30));
          const double s2 = sin(AZ_DEG2RAD(i + 30));
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
  }
}

/*===========================================================================*/

static void draw_node_internal(const az_node_t *node, az_clock_t clock) {
  switch (node->kind) {
    case AZ_NODE_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_NODE_SAVE_POINT:
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_LOOP); {
        glVertex2d(25, 18);
        glVertex2d(-25, 18);
        glVertex2d(-25, -18);
        glVertex2d(25, -18);
      } glEnd();
      break;
    case AZ_NODE_TRACTOR:
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_LOOP); {
        // TODO: make this look better
        const double radius = 10.0;
        for (int i = 0; i < 16; ++i) {
          glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                     radius * sin(i * AZ_PI_EIGHTHS));
        }
      } glEnd();
      break;
    case AZ_NODE_UPGRADE:
      draw_upgrade_icon(node->subkind.upgrade, clock);
      break;
    case AZ_NODE_REFILL:
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_LOOP); {
        glVertex2d(10, 0); glVertex2d(0, 10);
        glVertex2d(-10, 0); glVertex2d(0, -10);
      } glEnd();
      break;
    case AZ_NODE_COMM:
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_LOOP); {
        glVertex2d(10, 0);
        glVertex2d(-10, 10);
        glVertex2d(-10, -10);
      } glEnd();
      break;
    case AZ_NODE_DOODAD_FG:
    case AZ_NODE_DOODAD_BG:
      draw_doodad(node->subkind.doodad, clock);
      break;
  }
}

void az_draw_node(const az_node_t *node, az_clock_t clock) {
  assert(node->kind != AZ_NODE_NOTHING);
  glPushMatrix(); {
    glTranslated(node->position.x, node->position.y, 0);
    glRotated(AZ_RAD2DEG(node->angle), 0, 0, 1);
    draw_node_internal(node, clock);
  } glPopMatrix();
}

void az_draw_background_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_DOODAD_BG) {
      az_draw_node(node, state->clock);
    }
  }
}

void az_draw_upgrade_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_UPGRADE) {
      az_draw_node(node, state->clock);
    }
  }
}

void az_draw_middle_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_NOTHING ||
        node->kind == AZ_NODE_UPGRADE ||
        node->kind == AZ_NODE_DOODAD_FG ||
        node->kind == AZ_NODE_DOODAD_BG) continue;
    az_draw_node(node, state->clock);
  }
}

void az_draw_foreground_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_DOODAD_FG) {
      az_draw_node(node, state->clock);
    }
  }
}

/*===========================================================================*/
