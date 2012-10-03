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

#include <stdbool.h>
#include <stdio.h>

#include <OpenGL/gl.h>
#include <SDL/SDL.h>

#include "azimuth/screen.h"
#include "azimuth/space.h"
#include "azimuth/vector.h"

#define VIDEO_DEPTH 32
#define VIDEO_FLAGS (SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL)

static void draw_camera_view(az_space_state_t* state) {
  // center:
  glPushMatrix(); {
    //glTranslated(0, 0, 0);
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
}

static void draw_screen(az_space_state_t* state) {
  glClear(GL_COLOR_BUFFER_BIT);

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

  glFlush(); // Are the flush and finish at all necessary?  I'm not sure.
  glFinish();
  SDL_GL_SwapBuffers();
}

static void event_loop(void) {
  az_controls_t controls = {false};
  az_space_state_t state;
  state.clock = 0;
  state.ship.position = (az_vector_t){50, 150};
  state.ship.velocity = (az_vector_t){-20, 10};
  state.ship.angle = 2.9;
  state.camera = AZ_VZERO;

  while (true) {
    az_tick_space_state(&state, &controls, 1.0/60.0);
    draw_screen(&state);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        return;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_UP: controls.up = true; break;
        case SDLK_DOWN: controls.down = true; break;
        case SDLK_LEFT: controls.left = true; break;
        case SDLK_RIGHT: controls.right = true; break;
        case SDLK_x: controls.util = true; break;
        default: break;
        }
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_UP: controls.up = false; break;
        case SDLK_DOWN: controls.down = false; break;
        case SDLK_LEFT: controls.left = false; break;
        case SDLK_RIGHT: controls.right = false; break;
        case SDLK_x: controls.util = false; break;
        default: break;
        }
        break;
      default: break;
      }
    }
  }
}

int main(int argc, char **argv) {
  // Start up SDL (quit if we fail):
  if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

  // Enable OpenGL double-buffering:
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  // Enable vsync:
#if SDL_VERSION_ATLEAST(1,3,0)
  SDL_GL_SetSwapInterval(1);
#else
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
#endif
  // Enable antialiasing:
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

  // Init the display (quit if we fail):
  if (!SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
                        VIDEO_DEPTH, VIDEO_FLAGS)) {
    SDL_Quit();
    return 1;
  }

  // Turn off the depth buffer:
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  // Enable antialiasing:
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  // Set the view:
  glClearColor(0, 0, 0, 0);
  glClearDepth(1.0f);
  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Run the game:
  event_loop();

  // Exit cleanly:
  SDL_Quit();
  return 0;
}
