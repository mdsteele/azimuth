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

#include "azimuth/view/space.h"

#include <assert.h>
#include <math.h>

#include <OpenGL/gl.h>

#include "azimuth/screen.h"
#include "azimuth/state/baddie.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util.h"
#include "azimuth/vector.h"
#include "azimuth/view/hud.h"
#include "azimuth/view/ship.h"

/*===========================================================================*/

static void draw_pickup(az_pickup_kind_t kind, unsigned long clock) {
  // TODO: draw different kinds of pickups differently
  double radius = 0.5 * (double)(clock % 16);
  switch (kind) {
  case AZ_PUP_ROCKETS:
    glColor3f(1, 0, 0);
    glBegin(GL_LINES); {
      glVertex2d(-2.5,  2.5);
      glVertex2d( 2.5,  2.5);
      glVertex2d(-2.5, -0.5);
      glVertex2d( 2.5, -0.5);
    } glEnd();
    glBegin(GL_LINE_STRIP); {
      glVertex2d(-2.5, -1.5);
      glVertex2d(-4.5, -4.5);
      glVertex2d(-1.5, -3.5);
    } glEnd();
    glBegin(GL_LINE_STRIP); {
      glVertex2d( 2.5, -1.5);
      glVertex2d( 4.5, -4.5);
      glVertex2d( 1.5, -3.5);
    } glEnd();
    glColor3f(0, (clock % 8 < 4 ? 0 : 1), 1);
    glBegin(GL_LINE_LOOP); {
      glVertex2d( 1.5,  4.5);
      glVertex2d( 2.5,  2.5);
      glVertex2d( 2.5, -1.5);
      glVertex2d( 1.5, -3.5);
      glVertex2d(-1.5, -3.5);
      glVertex2d(-2.5, -1.5);
      glVertex2d(-2.5,  2.5);
      glVertex2d(-1.5,  4.5);
    } glEnd();
    break;
  case AZ_PUP_BOMBS:
    glColor3f(1, (clock % 8 < 4 ? 0 : 1), 0);
    glBegin(GL_POLYGON); {
      glVertex2d( 1.5,  2.5);
      glVertex2d( 2.5,  1.5);
      glVertex2d( 2.5, -1.5);
      glVertex2d( 1.5, -2.5);
      glVertex2d(-1.5, -2.5);
      glVertex2d(-2.5, -1.5);
      glVertex2d(-2.5,  1.5);
      glVertex2d(-1.5,  2.5);
    } glEnd();
    glColor3f(0, 0, 1);
    glBegin(GL_LINE_LOOP); {
      glVertex2d( 2.5,  4.5);
      glVertex2d( 4.5,  2.5);
      glVertex2d( 4.5, -2.5);
      glVertex2d( 2.5, -4.5);
      glVertex2d(-2.5, -4.5);
      glVertex2d(-4.5, -2.5);
      glVertex2d(-4.5,  2.5);
      glVertex2d(-2.5,  4.5);
    } glEnd();
    break;
  case AZ_PUP_LARGE_SHIELDS:
    radius *= 1.3; // fallthrough
  case AZ_PUP_MEDIUM_SHIELDS:
    radius *= 1.3; // fallthrough
  case AZ_PUP_SMALL_SHIELDS:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0, 1, 1, 0);
      glVertex2d(0, 0);
      glColor4f(0, 1, 1, 1);
      for (int i = 0; i <= 16; ++i) {
        glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                   radius * sin(i * AZ_PI_EIGHTHS));
      }
    } glEnd();
    break;
  default: assert(false);
  }
}

static void draw_pickups(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(pickup, state->pickups) {
    if (pickup->kind == AZ_PUP_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(pickup->position.x, pickup->position.y, 0);
      glRotated(AZ_RAD2DEG(-az_vtheta(pickup->position)), 0, 0, 1);
      draw_pickup(pickup->kind, state->clock);
    } glPopMatrix();
  }
}

static void draw_camera_view(const az_space_state_t *state) {
  // center:
  glPushMatrix(); {
    glColor4f(1, 0, 0, 1); // red
    glBegin(GL_LINE_LOOP); {
      glVertex2d( 50,  50);
      glVertex2d(-50,  50);
      glVertex2d(-50, -50);
      glVertex2d( 50, -50);
    } glEnd();
    glBegin(GL_LINE_LOOP); {
      glVertex2d( 250,  250);
      glVertex2d(   0,  330);
      glVertex2d(-250,  250);
      glVertex2d(-330,    0);
      glVertex2d(-250, -250);
      glVertex2d(   0, -330);
      glVertex2d( 250, -250);
      glVertex2d( 330,    0);
    } glEnd();
  } glPopMatrix();

  draw_pickups(state);

  // Draw baddies:
  // TODO: draw different kinds of baddies differently
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(baddie->position.x, baddie->position.y, 0);
      glRotated(AZ_RAD2DEG(baddie->angle), 0, 0, 1);
      glColor3f(1, 0, 1); // magenta
      glBegin(GL_LINE_LOOP); {
        glVertex2d(20, 0);
        glVertex2d(15, 15);
        glVertex2d(-15, 15);
        glVertex2d(-15, -15);
        glVertex2d(15, -15);
      } glEnd();
    } glPopMatrix();
  }

  az_draw_ship(state);

  // Draw projectiles:
  // TODO: draw different kinds of projectiles differently
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind != AZ_PROJ_NOTHING) {
      glColor3f(0, 1, 0); // green
      glBegin(GL_POINTS); {
        glVertex2d(proj->position.x, proj->position.y);
      } glEnd();
    }
  }
}

void az_space_draw_screen(const az_space_state_t *state) {
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(SCREEN_WIDTH/2, -SCREEN_HEIGHT/2, 0);
    // Move the screen to the camera pose.
    glTranslated(0, -az_vnorm(state->camera), 0);
    glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera)), 0, 0, 1);
    // Draw what the camera sees.
    draw_camera_view(state);
  } glPopMatrix();

  az_draw_hud(state);
}

/*===========================================================================*/
