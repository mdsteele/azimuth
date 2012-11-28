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

#include "azimuth/view/ship.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/node.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_draw_ship(az_space_state_t* state) {
  const az_ship_t *ship = &state->ship;
  if (!az_ship_is_present(ship)) return;
  const az_controls_t *controls = &ship->controls;

  // Draw the tractor beam:
  if (ship->tractor_beam.active) {
    az_node_t *node;
    if (az_lookup_node(state, ship->tractor_beam.node_uid, &node)) {
      const az_vector_t start = node->position;
      const az_vector_t delta = az_vsub(ship->position, start);
      const double dist = az_vnorm(delta);
      const double thick = 4;
      const float green = az_clock_mod(2, 2, state->clock) == 0 ? 1.0 : 0.0;
      glPushMatrix(); {
        glTranslated(start.x, start.y, 0);
        glRotated(AZ_RAD2DEG(az_vtheta(delta)), 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(1, green, 1, 0.5);
          glVertex2d(0, 0);
          glColor4f(1, green, 1, 0);
          for (int i = 4; i <= 12; ++i) {
            glVertex2d(thick * cos(i * AZ_PI_EIGHTHS),
                       thick * sin(i * AZ_PI_EIGHTHS));
          }
        } glEnd();
        glBegin(GL_QUAD_STRIP); {
          glColor4f(1, green, 1, 0);
          glVertex2d(0, thick);
          glVertex2d(dist, thick);
          glColor4f(1, green, 1, 0.5);
          glVertex2d(0, 0);
          glVertex2d(dist, 0);
          glColor4f(1, green, 1, 0);
          glVertex2d(0, -thick);
          glVertex2d(dist, -thick);
        } glEnd();
      } glPopMatrix();
    }
  }

  // Draw the ship itself:
  glPushMatrix(); {
    glTranslated(ship->position.x, ship->position.y, 0);
    glRotated(AZ_RAD2DEG(ship->angle), 0, 0, 1);
    // Exhaust:
    if (controls->up && !controls->down) {
      double zig = az_clock_zigzag(10, 1, state->clock);
      // For forward thrusters:
      if (!controls->burn) {
        // From port engine:
        glBegin(GL_TRIANGLE_STRIP); {
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(-10, 12);
          glColor4f(1, 0.75, 0, 0.9); // orange
          glVertex2d(-10, 9);
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(-20 - zig, 9);
          glVertex2d(-10, 7);
        } glEnd();
        // From starboard engine:
        glBegin(GL_TRIANGLE_STRIP); {
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(-10, -12);
          glColor4f(1, 0.75, 0, 0.9); // orange
          glVertex2d(-10, -9);
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(-20 - zig, -9);
          glVertex2d(-10, -7);
        } glEnd();
      }
      // For reverse thrusters:
      else {
        // From port engine:
        glBegin(GL_TRIANGLE_STRIP); {
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(6, 12);
          glColor4f(1, 0.75, 0, 0.9); // orange
          glVertex2d(6, 9);
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(16 + zig, 9);
          glVertex2d(6, 7);
        } glEnd();
        // From starboard engine:
        glBegin(GL_TRIANGLE_STRIP); {
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(6, -12);
          glColor4f(1, 0.75, 0, 0.9); // orange
          glVertex2d(6, -9);
          glColor4f(1, 0.5, 0, 0); // transparent orange
          glVertex2d(16 + zig, -9);
          glVertex2d(6, -7);
        } glEnd();
      }
    }
    // Gun charge:
    if (ship->gun_charge > 0.0) {
      assert(ship->gun_charge <= 1.0);
      glPushMatrix(); {
        glTranslated(20, 0, 0);
        const double radius = ship->gun_charge *
          (7.0 + 0.3 * az_clock_zigzag(10, 1, state->clock));
        const int offset = 6 * az_clock_mod(60, 1, state->clock);
        for (int n = 0; n < 2; ++n) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor4f(1, 1, 0.5, 0.4);
            glVertex2d(0, 0);
            glColor4f(1, 1, 1, 0.0);
            for (int i = 0; i <= 360; i += 60) {
              const double degrees = (n == 0 ? i + offset : i - offset);
              glVertex2d(radius * cos(AZ_DEG2RAD(degrees)),
                         radius * sin(AZ_DEG2RAD(degrees)));
            }
          } glEnd();
        }
      } glPopMatrix();
    }
    // Engines:
    glBegin(GL_QUADS); {
      // Struts:
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( 1,  9);
      glVertex2d(-7,  9);
      glVertex2d(-7, -9);
      glVertex2d( 1, -9);
      // Port engine:
      glVertex2d(-10,  12);
      glVertex2d(  6,  12);
      glColor3f(0.75, 0.75, 0.75); // light gray
      glVertex2d(  8,   7);
      glVertex2d(-11,   7);
      // Starboard engine:
      glVertex2d(  8,  -7);
      glVertex2d(-11,  -7);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d(-10, -12);
      glVertex2d(  6, -12);
    } glEnd();
    // Main body:
    glBegin(GL_QUAD_STRIP); {
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( 15,  4);
      glVertex2d(-14,  4);
      glColor3f(0.75, 0.75, 0.75); // light gray
      glVertex2d( 20,  0);
      glVertex2d(-14,  0);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2d( 15, -4);
      glVertex2d(-14, -4);
    } glEnd();
    // Windshield:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0, 0.5, 0.5); // dim cyan
      glVertex2d(14,  2);
      glColor3f(0, 1, 1); // cyan
      glVertex2d(18,  0);
      glVertex2d(12,  0);
      glColor3f(0, 0.5, 0.5); // dim cyan
      glVertex2d(15, -2);
    } glEnd();

    // Shield flare:
    if (ship->shield_flare > 0.0) {
      assert(ship->shield_flare <= 1.0);
      glBegin(GL_QUAD_STRIP); {
        const GLfloat alpha = 0.6 * ship->shield_flare;
        const double scale = 1.0 + 0.7 * ship->shield_flare;
        for (int i = AZ_SHIP_POLYGON.num_vertices - 1, j = 0;
             j <= AZ_SHIP_POLYGON.num_vertices; i = j++) {
          glColor4f(0, 1, 1, alpha);
          const az_vector_t v1 = AZ_SHIP_POLYGON.vertices[i];
          glVertex2d(v1.x, v1.y);
          glColor4f(0, 1, 1, 0);
          const az_vector_t v2 = az_vmul(v1, scale);
          glVertex2d(v2.x, v2.y);
        }
      } glEnd();
    }
  } glPopMatrix();
}

/*===========================================================================*/
