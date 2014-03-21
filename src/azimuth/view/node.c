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
#include "azimuth/view/doodad.h"
#include "azimuth/view/string.h"
#include "azimuth/view/wall.h"

/*===========================================================================*/
// Consoles:

static void draw_console(const az_node_t *node, az_clock_t clock) {
  assert(node->kind == AZ_NODE_CONSOLE);
  switch (node->subkind.console) {
    case AZ_CONS_COMM:
      // Platform:
      glBegin(GL_POLYGON); {
        glColor3f(0.1, 0.1, 0.1);
        glVertex2f(26, 16); glVertex2f(26, -16);
        glVertex2f(-17, -16); glVertex2f(-20, -13);
        glVertex2f(-20, 13); glVertex2f(-17, 16);
      } glEnd();
      // Glow:
      glBegin(GL_TRIANGLE_FAN); {
        if (node->status == AZ_NS_READY) glColor4f(1, 1, 0.5, 0.3);
        else glColor4f(1, 1, 1, 0.1);
        glVertex2f(0, 0);
        glColor4f(1, 1, 1, 0);
        const double radius = 10 + az_clock_zigzag(6, 6, clock);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Port:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.05, 0.15, 0.05);
        glVertex2f(26, 3.5); glVertex2f(15, 1.5);
        glColor3f(0.325, 0.4, 0.325);
        glVertex2f(26, 0); glVertex2f(15, 0);
        glColor3f(0.05, 0.15, 0.05);
        glVertex2f(26, -3.5); glVertex2f(15, -1.5);
      } glEnd();
      // Box:
      glBegin(GL_QUADS); {
        glColor3f(0.4, 0.4, 0.4);
        glVertex2f(29, 18); glVertex2f(29, -19);
        glVertex2f(48, -19); glVertex2f(48, 18);
        const int slowdown = (node->status == AZ_NS_ACTIVE ? 6 : 16);
        for (int i = 0; i < 3; ++i) {
          const int size = 5;
          const int x = 30 + (size + 1) * i;
          for (int j = 0; j < 6; ++j) {
            const int y = (size + 1) * (j - 3);
            switch ((az_clock_mod(5, slowdown, clock) + i * 29 +
                     az_clock_mod(j + 5, slowdown, clock)) % 5) {
              case 0: glColor3f(0, 0, 0); break;
              case 1: glColor3f(1, 0, 0); break;
              case 2: glColor3f(0, 0, 1); break;
              case 3: glColor3f(1, 1, 1); break;
              case 4: glColor3f(0, 1, 0); break;
            }
            glVertex2i(x, y); glVertex2i(x + size, y);
            glVertex2i(x + size, y + size); glVertex2i(x, y + size);
          }
        }
      } glEnd();
      // Siding:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.4, 0.4, 0.4); glVertex2f(29,  18);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(26,  21);
        glColor3f(0.4, 0.4, 0.4); glVertex2f(29, -19);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(26, -22);
        glColor3f(0.4, 0.4, 0.4); glVertex2f(48, -19);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(51, -22);
        glColor3f(0.4, 0.4, 0.4); glVertex2f(48,  18);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(51,  21);
        glColor3f(0.4, 0.4, 0.4); glVertex2f(29,  18);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(26,  21);
      } glEnd();
      break;
    case AZ_CONS_REFILL:
      // Frame:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.5, 0.5, 0.5); glVertex2f(25, -20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(29, -24);
        glColor3f(0.5, 0.5, 0.5); glVertex2f(30, 0);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(35, 0);
        glColor3f(0.5, 0.5, 0.5); glVertex2f(25, 20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(29, 24);
        glColor3f(0.5, 0.5, 0.5); glVertex2f(-20, 20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-24, 24);
        glColor3f(0.5, 0.5, 0.5); glVertex2f(-20, -20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(-24, -24);
        glColor3f(0.5, 0.5, 0.5); glVertex2f(25, -20);
        glColor3f(0.25, 0.25, 0.25); glVertex2f(29, -24);
      } glEnd();
      // Glow:
      if (node->status == AZ_NS_ACTIVE) {
        glColor4f(1, 1, 0, 0.1f + 0.03f * az_clock_zigzag(6, 3, clock));
        glBegin(GL_POLYGON); {
          glVertex2f(25, -20); glVertex2f(30, 0); glVertex2f(25, 20);
          glVertex2f(-20, 20); glVertex2f(-20, -20);
        } glEnd();
      } else {
        glBegin(GL_QUADS); {
          const int index = (node->status == AZ_NS_READY ?
                             4 - az_clock_mod(5, 6, clock) :
                             az_clock_mod(5, 10, clock));
          glColor4f(1, 1, (node->status == AZ_NS_READY ? 0.0f : 1.0f), 0.5);
          switch (index) {
            case 0: glVertex2f(-20, 20); glVertex2f(-20, -20); break;
            case 1: glVertex2f(-20, 20); glVertex2f( 25,  20); break;
            case 2: glVertex2f( 25, 20); glVertex2f( 30,   0); break;
            case 3: glVertex2f( 30,  0); glVertex2f( 25, -20); break;
            case 4: glVertex2f(25, -20); glVertex2f(-20, -20); break;
            default: AZ_ASSERT_UNREACHABLE();
          }
          glColor4f(1, 1, (node->status == AZ_NS_READY ? 0.0f : 1.0f), 0);
          switch (index) {
            case 0: glVertex2f(-15, -20); glVertex2f(-15,   20); break;
            case 1: glVertex2f(26.5, 15); glVertex2f(-20,   15); break;
            case 2: glVertex2f( 24,   0); glVertex2f( 19,   20); break;
            case 3: glVertex2f( 19, -20); glVertex2f( 24,    0); break;
            case 4: glVertex2f(-20, -15); glVertex2f(26.5, -15); break;
            default: AZ_ASSERT_UNREACHABLE();
          }
        } glEnd();
      }
      // Pipes:
      for (int i = -1; i <= 1; i += 2) {
        const int port_inner = (node->status == AZ_NS_ACTIVE ? 12 : 15);
        const int port_outer = port_inner + 12;
        // Port:
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.05, 0.15, 0.05);
          glVertex2f(1.5, i * port_outer); glVertex2f(-0.5, i * port_inner);
          glColor3f(0.325, 0.4, 0.325);
          glVertex2f(-3, i * port_outer); glVertex2f(-3, i * port_inner);
          glColor3f(0.05, 0.15, 0.05);
          glVertex2f(-7.5, i * port_outer); glVertex2f(-5.5, i * port_inner);
        } glEnd();
        // Pipe:
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.05, 0.15, 0.05);
          glVertex2f(1.5, i * port_outer); glVertex2f(1.5, i * 30);
          glColor3f(0.325, 0.4, 0.325);
          glVertex2f(-3, i * port_outer); glVertex2f(-3, i * 30);
          glColor3f(0.05, 0.15, 0.05);
          glVertex2f(-7.5, i * port_outer); glVertex2f(-7.5, i * 30);
        } glEnd();
        // Sub-coupling:
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.1, 0.175, 0.1);
          glVertex2f(2, i * port_outer); glVertex2f(2, i * (port_outer + 2));
          glColor3f(0.375, 0.425, 0.375);
          glVertex2f(-3, i * port_outer); glVertex2f(-3, i * (port_outer + 2));
          glColor3f(0.1, 0.175, 0.1);
          glVertex2f(-8, i * port_outer); glVertex2f(-8, i * (port_outer + 2));
        } glEnd();
        // Coupling:
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.1, 0.175, 0.1);
          glVertex2f(3, i * 36); glVertex2f(3, i * 30);
          glColor3f(0.375, 0.425, 0.375);
          glVertex2f(-3, i * 36); glVertex2f(-3, i * 30);
          glColor3f(0.1, 0.175, 0.1);
          glVertex2f(-9, i * 36); glVertex2f(-9, i * 30);
        } glEnd();
      }
      break;
    case AZ_CONS_SAVE:
      // Pipe:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.05, 0.2, 0.05);
        glVertex2f(-40, 5); glVertex2f(-30, 5);
        glColor3f(0.325, 0.45, 0.325);
        glVertex2f(-40, 0); glVertex2f(-30, 0);
        glColor3f(0.05, 0.2, 0.05);
        glVertex2f(-40, -5); glVertex2f(-30, -5);
      } glEnd();
      // Coupling:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.1, 0.175, 0.1);
        glVertex2f(-46, 6); glVertex2f(-40, 6);
        glColor3f(0.375, 0.425, 0.375);
        glVertex2f(-46, 0); glVertex2f(-40, 0);
        glColor3f(0.1, 0.175, 0.1);
        glVertex2f(-46, -6); glVertex2f(-40, -6);
      } glEnd();
      // Port:
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.05, 0.15, 0.05);
        glVertex2f(-24, 4.5); glVertex2f(-12, 2.5);
        glColor3f(0.325, 0.4, 0.325);
        glVertex2f(-24, 0); glVertex2f(-12, 0);
        glColor3f(0.05, 0.15, 0.05);
        glVertex2f(-24, -4.5); glVertex2f(-12, -2.5);
      } glEnd();
      glBegin(GL_QUADS); {
        // Connecting strut:
        glColor3f(0.35, 0.35, 0.35);
        glVertex2f(-24, 21); glVertex2f(-30, 21);
        glVertex2f(-30, -21); glVertex2f(-24, -21);
        // Glow:
        if (node->status == AZ_NS_ACTIVE) {
          if (az_clock_mod(2, 5, clock)) glColor4f(1, 1, 0.5, 0.5);
          else glColor4f(0.5, 1, 1, 0.5);
          glVertex2f(-35, 20); glVertex2f(35, 20);
          glVertex2f(35, -20); glVertex2f(-35, -20);
        } else {
          const GLfloat ampl = 3.0f + az_clock_zigzag(30, 1, clock) *
            (node->status == AZ_NS_READY ? 0.3f : 0.1f);
          glColor4f(1, 1, 1, 0);
          glVertex2f(35, 20 - ampl); glVertex2f(-35, 20 - ampl);
          glColor4f(1, 1, (node->status == AZ_NS_READY ? 0.0f : 1.0f), 0.5);
          glVertex2f(-35, 20); glVertex2f(35, 20);
          glVertex2f(-35, -20); glVertex2f(35, -20);
          glColor4f(1, 1, 1, 0);
          glVertex2f(35, -20 + ampl); glVertex2f(-35, -20 + ampl);
        }
        // Top arm:
        glColor3f(0.25, 0.25, 0.25);
        glVertex2f(-25, 30); glVertex2f(25, 30);
        glColor3f(0.75, 0.75, 0.75);
        glVertex2f(35, 20); glVertex2f(-35, 20);
        // Bottom arm:
        glVertex2f(35, -20); glVertex2f(-35, -20);
        glColor3f(0.25, 0.25, 0.25);
        glVertex2f(-25, -30); glVertex2f(25, -30);
      } glEnd();
      break;
  }
}

/*===========================================================================*/
// Tractor nodes:

static void draw_tractor_node(az_node_status_t status, az_clock_t clock) {
  for (int i = 0; i < 3; ++i) {
    glBegin(GL_QUAD_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(0, 3); glVertex2f(12, 3);
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f(0, 0);
      glColor3f(0.5, 0.5, 0.5);
      glVertex2f(13.5, 0);
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(0, -3); glVertex2f(12, -3);
    } glEnd();
    glRotatef(120, 0, 0, 1);
  }
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.75, 0.75, 0.75);
    glVertex2f(0, 0);
    glColor3f(0.35, 0.35, 0.35);
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(6 * cos(AZ_DEG2RAD(i)), 6 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    switch (status) {
      case AZ_NS_FAR: glColor3f(0, 0, 0); break;
      case AZ_NS_NEAR: glColor3f(1, 1, 0.5); break;
      case AZ_NS_READY:
        if (az_clock_mod(2, 5, clock)) glColor3f(1, 0, 1);
        else glColor3f(1, 1, 0.5);
        break;
      case AZ_NS_ACTIVE: glColor3f(1, 0, 1); break;
    }
    glVertex2f(0, 0);
    glColor4f(0, 0, 0, 0);
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

/*===========================================================================*/
// Upgrades:

// Helper function for drawing upgrade icons for explosives upgrades.
static void draw_explosives_casing(void) {
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.25, 0.2, 0.2);
    glVertex2f(-6, 10); glVertex2f(-6, -10);
    glColor3f(0.65, 0.6, 0.6);
    glVertex2f(0, 10); glVertex2f(0, -10);
    glColor3f(0.25, 0.2, 0.2);
    glVertex2f(6, 10); glVertex2f(6, -10);
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0, 0, 0);
    glVertex2f(-6.1, 4); glVertex2f(-6.1, -7);
    glVertex2f(-2, 7); glVertex2f(-2, -5);
    glVertex2f(2, 4); glVertex2f(2, -7);
    glVertex2f(6.1, 6); glVertex2f(6.1, -4);
  } glEnd();
}

// Helper function for drawing upgrade icons for armor upgrades.
static void draw_armor(void) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.75, 0.75, 0.75);
    glVertex2f(0, 0);
    glColor3f(0.15, 0.15, 0.15);
    glVertex2f(0, -11); glVertex2f(11, 0);
    glVertex2f(0, 11); glVertex2f(-11, 0);
    glVertex2f(0, -11);
  } glEnd();
}

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
    case AZ_UPG_GUN_HOMING:
      glPushMatrix(); {
        glRotatef(45 * frame, 0, 0, 1);
        glBegin(GL_TRIANGLES); {
          glColor3f(0, 0.375, 1);
          glVertex2f( 8,  6); glVertex2f(6, -2); glVertex2f(10, -2);
          glVertex2f(-8, -6); glVertex2f(-6, 2); glVertex2f(-10, 2);
        } glEnd();
        glBegin(GL_POINTS); {
          for (int i = 0; i < 8; ++i) {
            glColor4f(0, 0.375, 1, 1.0f - 0.125f * i);
            const double radius = 8.0;
            const double theta = AZ_DEG2RAD(-10) * i;
            const double x = radius * cos(theta);
            const double y = radius * sin(theta);
            glVertex2d(x, y); glVertex2d(-x, -y);
          }
        } glEnd();
      } glPopMatrix();
      glPushMatrix(); {
        glRotatef(-45 * frame, 0, 0, 1);
        glBegin(GL_TRIANGLES); {
          glColor3f(0, 0.375, 1);
          glVertex2f(-3, -4); glVertex2f( 1, -2.5); glVertex2f( 1, -5.5);
          glVertex2f( 3,  4); glVertex2f(-1,  2.5); glVertex2f(-1,  5.5);
        } glEnd();
      } glPopMatrix();
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
    case AZ_UPG_GUN_PIERCE: {
      const int offset = frame - 3;
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0, 1, 0.75); // magenta
        glVertex2f(9 + offset, -9 - offset);
        glColor4f(1, 0, 1, 0); // transparent magenta
        glVertex2f(9 + offset, -3 - offset);
        glVertex2f(-10 - offset, 10 + offset);
        glVertex2f(3 + offset, -9 - offset);
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat wiggle = 7 + frame;
        glVertex2f(6 + offset, -6 - offset);
        glColor4f(1, 0, 1, 0.75); // magenta
        glVertex2f(wiggle + offset, 4 - offset);
        glVertex2f(10 + offset, -10 - offset);
        glVertex2f(-4 + offset, -wiggle - offset);
      } glEnd();
    } break;
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
    case AZ_UPG_HYPER_ROCKETS:
      glPushMatrix(); {
        glRotatef(-90, 0, 0, 1);
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
      } glPopMatrix();
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
    case AZ_UPG_HIGH_EXPLOSIVES:
      draw_explosives_casing();
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat f = 0.7f + 0.215f * frame;
        glColor3f(1, 1, 0.2); glVertex2f(0, 0);
        glColor3f(0.9f, 0.1 + 0.07f * frame, 0);
        glVertex2f(0, 7*f); glVertex2f(2, 2); glVertex2f(8*f, 4*f);
        glVertex2f(4, -1); glVertex2f(7*f, -6*f); glVertex2f(2, -4);
        glVertex2f(1*f, -8*f); glVertex2f(-1, -3); glVertex2f(-6*f, -6*f);
        glVertex2f(-4, 0); glVertex2f(-7*f, 5*f); glVertex2f(-2, 3);
        glVertex2f(0, 7*f);
      } glEnd();
      break;
    case AZ_UPG_ATTUNED_EXPLOSIVES:
      draw_explosives_casing();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 1, 1); glVertex2f(0, 0);
        const double radius = 6 + 0.95 * (frame == 3 ? 1 : frame);
        glColor4f(0, 0.5, 1, 0.5);
        for (int i = 0; i <= 360; i += 10) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_UPG_RETRO_THRUSTERS:
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat zig = 3 * (frame == 3 ? 1 : frame);
        glColor4f(1, 0.75, 0, 0.9); // orange
        glVertex2f(-3, 0);
        glColor4f(1, 0.5, 0, 0); // transparent orange
        glVertex2f(-3, 5);
        glVertex2f(6 + zig, 0);
        glVertex2f(-3, -5);
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f(-12, 7); glVertex2f(-2, 6);
        glColor3f(0.5, 0.5, 0.5); // gray
        glVertex2f(-12, 0); glVertex2f(-2, 0);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f(-12, -7); glVertex2f(-2, -6);
      } glEnd();
      break;
    case AZ_UPG_CPLUS_DRIVE:
      glBegin(GL_TRIANGLES); {
        for (int i = 1; i < 4; ++i) {
          const float left = 5.0f * ((i + frame) % 4) - 10.0f;
          glColor4f(0, 1, 0, 0.3f * i);
          glVertex2f(left, 6); glVertex2f(left, -6);
          glVertex2f(left + 6, 0);
        }
      } glEnd();
      break;
    case AZ_UPG_ORION_BOOSTER:
      glBegin(GL_QUAD_STRIP); {
        const int offset = (frame == 3 ? 1 : 0);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f(-2, 12); glVertex2f(3 - offset, 7 + offset);
        glColor3f(0.5, 0.5, 0.5); // gray
        glVertex2f(-12, 12); glVertex2f(-3 - offset, 3 + offset);
        glColor3f(0.25, 0.25, 0.25); // dark gray
        glVertex2f(-12, 2); glVertex2f(-7 - offset, -3 + offset);
      } glEnd();
      glPushMatrix(); {
        glTranslatef(3, -3, 0);
        glRotatef(-45, 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(0.5, 0.75, 1, 0.3);
          glVertex2f(0, 0);
          glColor4f(1, 1, 1, 0.7);
          const double radius = 2.5 * (frame + 1);
          for (int i = 0; i <= 360; i += 20) {
            glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                       0.7 * radius * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_UPG_HARDENED_ARMOR:
      draw_armor();
      if (frame % 2) glColor3f(0, 0, 1);
      else glColor3f(0.5, 0.25, 0);
      glBegin(GL_LINES); {
        glVertex2f(-6, -8); glVertex2f(-6, 8);
        glVertex2f(-6, 0); glVertex2f(6, 0);
        glVertex2f(6, -8); glVertex2f(6, 8);
      } glEnd();
      break;
    case AZ_UPG_DYNAMIC_ARMOR:
      draw_armor();
      if (frame % 2) glColor3f(0, 1, 0);
      else glColor3f(1, 1, 0);
      glBegin(GL_LINE_LOOP); {
        glVertex2f(-6, 7); glVertex2f(-6, -7); glVertex2f(2, -7);
        glVertex2f(6, -3); glVertex2f(6, 3); glVertex2f(2, 7);
      } glEnd();
      break;
    case AZ_UPG_THERMAL_ARMOR:
      draw_armor();
      if (frame % 2) glColor3f(1, 0, 0);
      else glColor3f(0, 1, 1);
      glBegin(GL_LINES); {
        glVertex2f(-6, 6); glVertex2f(6, 6);
        glVertex2f(0, 6); glVertex2f(0, -7);
      } glEnd();
      break;
    case AZ_UPG_REACTIVE_ARMOR:
      draw_armor();
      if (frame % 2) glColor3f(1, 1, 0);
      else glColor3f(1, 0, 0);
      glBegin(GL_LINE_STRIP); {
        glVertex2f(-6, -7); glVertex2f(-6, 7); glVertex2f(6, 7);
        glVertex2f(6, 0); glVertex2f(-6, 0); glVertex2f(6, -7);
      } glEnd();
      break;
    case AZ_UPG_FUSION_REACTOR:
      glPushMatrix(); {
        const double spin1 = 3 * (frame - 1);
        const double spin2 = 3 * (((frame + 2) % 4) - 1);
        glRotatef(215.0f + 22.5f * frame, 0, 0, 1);
        glColor4f(1, 1, 1, 0.5);
        glBegin(GL_LINE_STRIP); {
          for (int i = 0; i <= 180; i += 20) {
            glVertex2d(12 * cos(AZ_DEG2RAD(i)), spin1 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        glBegin(GL_LINE_STRIP); {
          for (int i = -90; i <= 90; i += 20) {
            glVertex2d(spin2 * cos(AZ_DEG2RAD(i)), 12 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(1, 0.25, 0);
          glVertex2f(0, 0);
          glColor3f(0.3, 0.07, 0);
          for (int i = 0; i <= 360; i += 20) {
            glVertex2d(5.5 * cos(AZ_DEG2RAD(i)), 5.5 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        glColor4f(1, 1, 1, 0.5);
        glBegin(GL_LINE_STRIP); {
          for (int i = 180; i <= 360; i += 20) {
            glVertex2d(12 * cos(AZ_DEG2RAD(i)), spin1 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        glBegin(GL_LINE_STRIP); {
          for (int i = 90; i <= 270; i += 20) {
            glVertex2d(spin2 * cos(AZ_DEG2RAD(i)), 12 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      } glPopMatrix();
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
    case AZ_UPG_TRACTOR_BEAM:
      glPushMatrix(); {
        glTranslatef(-6, 6, 0);
        glScalef(0.5, 0.5, 1);
        draw_tractor_node(AZ_NS_ACTIVE, 0);
      } glPopMatrix();
      glBegin(GL_QUAD_STRIP); {
        const GLfloat green = (frame % 2 == 0 ? 0.75f : 0.25f);
        glColor4f(1, green, 1, 0);
        glVertex2f(-5, 9); glVertex2f(12, -8);
        glColor4f(1, green, 1, 0.5);
        glVertex2f(-7, 7); glVertex2f(12, -12);
        glColor4f(1, green, 1, 0);
        glVertex2f(-9, 5); glVertex2f(8, -12);
      } glEnd();
      break;
    case AZ_UPG_INFRASCANNER:
      glBegin(GL_QUAD_STRIP); {
        glColor4f(1, 1, 0.5, 0.25);
        const GLfloat r1 = 6.0f + 0.5f * (frame == 3 ? 1 : frame);
        const GLfloat r2 = r1 + 1.5f;
        for (int i = 0; i <= 360; i += 20) {
          glVertex2d(r1 * cos(AZ_DEG2RAD(i)), r1 * sin(AZ_DEG2RAD(i)));
          glVertex2d(r2 * cos(AZ_DEG2RAD(i)), r2 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat outer = 12 + (frame == 3 ? 1 : frame);
        const GLfloat inner = 2 + (frame == 3 ? 1 : frame);
        glColor3f(1, 1, 1);
        glVertex2f(0, 0);
        glColor4f(1, 1, 0, 0);
        glVertex2f(outer, 0); glVertex2f(inner, inner);
        glVertex2f(0, outer); glVertex2f(-inner, inner);
        glVertex2f(-outer, 0); glVertex2f(-inner, -inner);
        glVertex2f(0, -outer); glVertex2f(inner, -inner);
        glVertex2f(outer, 0);
      } glEnd();
      break;
    case AZ_UPG_MAGNET_SWEEP:
      glBegin(GL_TRIANGLE_FAN); {
        const double scale = 1 - (0.33 * (frame == 3 ? 1 : frame));
        glColor4f(1, 1, 0.5, 0.5); glVertex2f(0, 8);
        glColor4f(1, 1, 0.5, 0);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(8 * scale * cos(AZ_DEG2RAD(i)),
                     5 * scale * sin(AZ_DEG2RAD(i)) + 8);
        }
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        for (int i = -200; i <= 20; i += 20) {
          const double c = cos(AZ_DEG2RAD(i)), s = sin(AZ_DEG2RAD(i));
          glColor3f(1, 0, 0); glVertex2d(8 * c, 8 * s - 3);
          glColor3f(0.7, 0, 0); glVertex2d(4 * c, 4 * s - 3);
        }
      } glEnd();
      glBegin(GL_QUADS); {
        for (int i = -1; i <= 1; i += 2) {
          glColor3f(0.5, 0.6, 0.6);
          glVertex2f(i * 2, 6); glVertex2f(i * 6, 6);
          glColor3f(1, 0, 0);
          glVertex2f(i * 7.7, -1);
          glColor3f(0.7, 0, 0);
          glVertex2f(i * 3.9, -2.3);
        }
      }
      break;
    case AZ_UPG_TRACTOR_CLOAK:
      // TODO: upgrade icon for Tractor Cloak
      break;
    case AZ_UPG_MILLIWAVE_RADAR:
      // TODO: upgrade icon for Milliwave Radar
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
    case AZ_UPG_ROCKET_AMMO_20:
    case AZ_UPG_ROCKET_AMMO_21:
    case AZ_UPG_ROCKET_AMMO_22:
    case AZ_UPG_ROCKET_AMMO_23:
    case AZ_UPG_ROCKET_AMMO_24:
    case AZ_UPG_ROCKET_AMMO_25:
    case AZ_UPG_ROCKET_AMMO_26:
    case AZ_UPG_ROCKET_AMMO_27:
    case AZ_UPG_ROCKET_AMMO_28:
    case AZ_UPG_ROCKET_AMMO_29:
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
    case AZ_UPG_BOMB_AMMO_10:
    case AZ_UPG_BOMB_AMMO_11:
    case AZ_UPG_BOMB_AMMO_12:
    case AZ_UPG_BOMB_AMMO_13:
    case AZ_UPG_BOMB_AMMO_14:
    case AZ_UPG_BOMB_AMMO_15:
    case AZ_UPG_BOMB_AMMO_16:
    case AZ_UPG_BOMB_AMMO_17:
    case AZ_UPG_BOMB_AMMO_18:
    case AZ_UPG_BOMB_AMMO_19:
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
    case AZ_UPG_CAPACITOR_00:
    case AZ_UPG_CAPACITOR_01:
    case AZ_UPG_CAPACITOR_02:
    case AZ_UPG_CAPACITOR_03:
    case AZ_UPG_CAPACITOR_04:
    case AZ_UPG_CAPACITOR_05:
    case AZ_UPG_CAPACITOR_06:
    case AZ_UPG_CAPACITOR_07:
    case AZ_UPG_CAPACITOR_08:
    case AZ_UPG_CAPACITOR_09:
    case AZ_UPG_CAPACITOR_10:
    case AZ_UPG_CAPACITOR_11:
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

static void draw_node_internal(const az_node_t *node, az_clock_t clock) {
  switch (node->kind) {
    case AZ_NODE_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_NODE_CONSOLE:
      draw_console(node, clock);
      break;
    case AZ_NODE_TRACTOR:
      draw_tractor_node(node->status, clock);
      break;
    case AZ_NODE_UPGRADE:
      draw_upgrade_icon(node->subkind.upgrade, clock);
      break;
    case AZ_NODE_DOODAD_FG:
    case AZ_NODE_DOODAD_BG:
      az_draw_doodad(node->subkind.doodad, clock);
      break;
    case AZ_NODE_FAKE_WALL_FG:
    case AZ_NODE_FAKE_WALL_BG:
      az_draw_wall_data(node->subkind.fake_wall, clock);
      break;
    case AZ_NODE_MARKER:
      glColor3f(0, 1, 1);
      glBegin(GL_LINES); {
        glVertex2f(-20, 0); glVertex2f(20, 0);
        glVertex2f(0, -20); glVertex2f(0, 20);
      } glEnd();
      glPushMatrix(); {
        glScalef(1, -1, 1);
        az_draw_printf(8, AZ_ALIGN_LEFT, 3, -10, "%d", node->subkind.marker);
      } glPopMatrix();
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
    if (node->kind == AZ_NODE_DOODAD_BG ||
        node->kind == AZ_NODE_FAKE_WALL_BG) {
      az_draw_node(node, state->clock);
    }
  }
}

void az_draw_console_and_upgrade_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_CONSOLE || node->kind == AZ_NODE_UPGRADE) {
      az_draw_node(node, state->clock);
    }
  }
}

void az_draw_tractor_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_TRACTOR) {
      az_draw_node(node, state->clock);
    }
  }
}

void az_draw_foreground_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_DOODAD_FG ||
        node->kind == AZ_NODE_FAKE_WALL_FG) {
      az_draw_node(node, state->clock);
    }
  }
}

/*===========================================================================*/
