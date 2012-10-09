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

#include <OpenGL/gl.h>

#include "azimuth/state/node.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static void draw_node(const az_node_t *node) {
  switch (node->kind) {
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
    // TODO: draw other kinds of nodes
    default: assert(false);
  }
}

void az_draw_nodes(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(node, state->nodes) {
    if (node->kind == AZ_NODE_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(node->position.x, node->position.y, 0);
      glRotated(AZ_RAD2DEG(node->angle), 0, 0, 1);
      draw_node(node);
    } glPopMatrix();
  }
}

/*===========================================================================*/
