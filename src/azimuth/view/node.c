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
#include "azimuth/util/misc.h"

/*===========================================================================*/

static void draw_upgrade_icon(az_upgrade_t upgrade, az_clock_t clock) {
  const int frame = az_clock_mod(4, 10, clock);
  switch (upgrade) {
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
        glColor3f(0, 1, 0);
        glVertex2d(2, 11); glVertex2d(-2, 11);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(-2, -12); glVertex2d(2, -12);
        glColor3f(0, 1, 0);
        glVertex2d(12, 9); glVertex2d(8, 9);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(2, -12); glVertex2d(5, -12);
        glColor3f(0, 1, 0);
        glVertex2d(-8, 9); glVertex2d(-12, 9);
        glColor4f(0, 1, 0, 0.125);
        glVertex2d(-5, -11); glVertex2d(-2, -11);
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
}

static void draw_node_internal(const az_node_t *node, az_clock_t clock) {
  switch (node->kind) {
    case AZ_NODE_NOTHING: assert(false); break;
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
      glColor3f(1, az_clock_mod(2, 10, clock), 1);
      glBegin(GL_LINE_LOOP); {
        glVertex2d(14, 14);
        glVertex2d(-14, 14);
        glVertex2d(-14, -14);
        glVertex2d(14, -14);
      } glEnd();
      draw_upgrade_icon(node->upgrade, clock);
      break;
    // TODO: draw other kinds of nodes
    default:
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_LOOP); {
        glVertex2d(10, 0);
        glVertex2d(-10, 10);
        glVertex2d(-10, -10);
      } glEnd();
      break;
  }
}

void az_draw_node(const az_node_t *node, az_clock_t clock) {
  glPushMatrix(); {
    glTranslated(node->position.x, node->position.y, 0);
    glRotated(AZ_RAD2DEG(node->angle), 0, 0, 1);
    draw_node_internal(node, clock);
  } glPopMatrix();
}

void az_draw_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_NOTHING) continue;
    az_draw_node(node, state->clock);
  }
}

/*===========================================================================*/
