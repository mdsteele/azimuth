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

#include "azimuth/screen.h"
#include "azimuth/space.h"
#include "azimuth/vector.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

static void draw_camera_view(az_space_state_t* state) {
  // center:
  glPushMatrix(); {
    glColor4f(1, 0, 0, 1); // red
    glBegin(GL_LINE_LOOP);
    glVertex2d( 50,  50);
    glVertex2d(-50,  50);
    glVertex2d(-50, -50);
    glVertex2d( 50, -50);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex2d( 250,  250);
    glVertex2d(   0,  330);
    glVertex2d(-250,  250);
    glVertex2d(-330,    0);
    glVertex2d(-250, -250);
    glVertex2d(   0, -330);
    glVertex2d( 250, -250);
    glVertex2d( 330,    0);
    glEnd();
  } glPopMatrix();

  // ship:
  glPushMatrix(); {
    glTranslated(state->ship.position.x, state->ship.position.y, 0);
    glRotated(AZ_RAD2DEG(state->ship.angle), 0, 0, 1);
    glColor4f(0, 1, 1, 1); // cyan
    glBegin(GL_LINE_LOOP);
    glVertex2d( 20,   0);
    glVertex2d(-20,  10);
    glVertex2d(-20, -10);
    glEnd();
  } glPopMatrix();
}

static void draw_hud(az_space_state_t* state) {
  const double speed = az_vnorm(state->ship.velocity);

  glColor4ub(0, 0, 0, 128); // tinted-black
  glBegin(GL_QUADS);
  glVertex2i(5, 5);
  glVertex2i(5, 35);
  glVertex2i(415, 35);
  glVertex2i(415, 5);
  glEnd();

  glColor4f(1, 1, 1, 1); // white
  glBegin(GL_LINE_STRIP);
  glVertex2d(14.5, 8.5);
  glVertex2d(8.5, 8.5);
  glVertex2d(8.5, 21.5);
  glVertex2d(14.5, 21.5);
  glEnd();
  glBegin(GL_LINE_STRIP);
  glVertex2d(407.5, 8.5);
  glVertex2d(411.5, 8.5);
  glVertex2d(411.5, 21.5);
  glVertex2d(407.5, 21.5);
  glEnd();

  glColor4ub(0, 128, 0, 255); // green
  glBegin(GL_QUADS);
  glVertex2d(10, 10);
  glVertex2d(10 + speed, 10);
  glVertex2d(10 + speed, 20);
  glVertex2d(10, 20);
  glEnd();

  glColor4f(1, 1, 1, 1); // white
  az_draw_string((az_vector_t){10, 25}, 8, "HELLO: WORLD! 0123456789");
}

void az_space_draw_screen(az_space_state_t* state) {
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

  draw_hud(state);
}

/*===========================================================================*/
