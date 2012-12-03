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

#include "azimuth/view/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static void draw_baddie_internal(const az_baddie_t *baddie, az_clock_t clock) {
  const double flare = baddie->armor_flare;
  const double frozen = (baddie->frozen <= 0.0 ? 0.0 :
                         baddie->frozen >= 0.2 ? 0.5 + 0.5 * baddie->frozen :
                         az_clock_mod(3, 2, clock) < 2 ? 0.6 : 0.0);
  if (baddie->frozen > 0.0) clock = 0;
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_LUMP:
      glColor3f(1 - frozen, 0, 1 - 0.75 * flare); // magenta
      glBegin(GL_POLYGON); {
        for (int i = 0; i < baddie->data->polygon.num_vertices; ++i) {
          glVertex2d(baddie->data->polygon.vertices[i].x,
                     baddie->data->polygon.vertices[i].y);
        }
      } glEnd();
      break;
    case AZ_BAD_TURRET:
      glPushMatrix(); {
        glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
        glBegin(GL_QUAD_STRIP); {
          glColor3f(0.25 + 0.25 * flare, 0.25,
                    0.25 + 0.25 * frozen); // dark gray
          glVertex2d( 0,  5);
          glVertex2d(30,  5);
          glColor3f(0.75 + 0.25 * flare, 0.75,
                    0.75 + 0.25 * frozen); // light gray
          glVertex2d( 0,  0);
          glVertex2d(30,  0);
          glColor3f(0.25 + 0.25 * flare, 0.25,
                    0.25 + 0.25 * frozen); // dark gray
          glVertex2d( 0, -5);
          glVertex2d(30, -5);
        } glEnd();
      } glPopMatrix();
      glColor3f(0.5 + 0.5 * flare, 0.5, 0.5 + 0.5 * frozen); // gray
      glBegin(GL_POLYGON); {
        const double radius = 20;
        for (int i = 0; i <= 6; ++i) {
          glVertex2d(radius * cos(i * AZ_PI / 3.0),
                     radius * sin(i * AZ_PI / 3.0));
        }
      } glEnd();
      break;
    case AZ_BAD_ZIPPER:
      glColor3f(flare, 1 - fmax(flare, 0.5 * frozen),
                0.25 + 0.75 * frozen); // green
      glBegin(GL_POLYGON); {
        for (int i = 0; i < baddie->data->polygon.num_vertices; ++i) {
          glVertex2d(baddie->data->polygon.vertices[i].x,
                     baddie->data->polygon.vertices[i].y);
        }
      } glEnd();
      break;
    case AZ_BAD_BOUNCER:
      glBegin(GL_TRIANGLE_FAN); {
        const unsigned int zig = az_clock_zigzag(15, 1, clock);
        glColor3f(1 - 0.75 * frozen, 0.25 + 0.01 * zig + 0.5 * flare,
                  0.25 + 0.75 * frozen); // red
        glVertex2d(0, 0);
        glColor3f(0.25 + 0.02 * zig - 0.25 * frozen, 0.5 * flare,
                  0.25 * frozen); // dark red
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5, 0.25, 1);
        glVertex2d(0, 7);
        glColor4f(0.5, 0.25, 1, 0);
        glVertex2d(3, 2); glVertex2d(3, 12); glVertex2d(-3, 12);
        glVertex2d(-3, 2); glVertex2d(3, 2);
      } glEnd();
      break;
  }
}

void az_draw_baddie(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  glPushMatrix(); {
    glTranslated(baddie->position.x, baddie->position.y, 0);
    glRotated(AZ_RAD2DEG(baddie->angle), 0, 0, 1);
    draw_baddie_internal(baddie, clock);
  } glPopMatrix();
}

void az_draw_baddies(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    az_draw_baddie(baddie, state->clock);
  }
}

/*===========================================================================*/
