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

#include <OpenGL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/space.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/node.h"
#include "azimuth/view/hud.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/pickup.h"
#include "azimuth/view/projectile.h"
#include "azimuth/view/ship.h"

/*===========================================================================*/

static void draw_camera_view(az_space_state_t *state) {
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
  //az_draw_walls(state);
  az_draw_nodes(state);
  az_draw_pickups(state);
  az_draw_baddies(state);
  az_draw_projectiles(state);
  az_draw_ship(state);
  az_draw_particles(state);
}

void az_space_draw_screen(az_space_state_t *state) {
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    // Move the screen to the camera pose.
    glTranslated(0, -az_vnorm(state->camera), 0);
    glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera)), 0, 0, 1);
    // Draw what the camera sees.
    draw_camera_view(state);
  } glPopMatrix();

  az_draw_hud(state);
}

/*===========================================================================*/
