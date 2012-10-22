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

#include "azimuth/gui/screen.h"

#include <stdbool.h>

#include <GL/gl.h>
#include <SDL/SDL.h>

#include "azimuth/constants.h"
#include "azimuth/util/misc.h" // for AZ_FATAL

/*===========================================================================*/

#ifdef __APPLE__
// The UTF-8 sequence for the Apple command key symbol (U+2318):
#define CMD_KEY_NAME "\xE2\x8C\x98"
#else
#define CMD_KEY_NAME "Ctrl"
#endif

#define VIDEO_DEPTH 32
#define VIDEO_FLAGS (SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL)

static bool currently_fullscreen;

void az_init_gui(bool fullscreen) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    AZ_FATAL("SDL_Init failed.\n");
  }
  atexit(SDL_Quit);
  SDL_WM_SetCaption("Azimuth (press " CMD_KEY_NAME "-M to run full-screen)",
                    "Azimuth");
  currently_fullscreen = !fullscreen;
  az_set_fullscreen(fullscreen);
}

bool az_is_fullscreen(void) {
  return currently_fullscreen;
}

void az_set_fullscreen(bool fullscreen) {
  if (fullscreen == currently_fullscreen) return;
  currently_fullscreen = fullscreen;
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

  // Init the display:
  if (!SDL_SetVideoMode(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT, VIDEO_DEPTH,
                        VIDEO_FLAGS | (fullscreen ? SDL_FULLSCREEN : 0))) {
    AZ_FATAL("SDL_SetVideoMode failed.\n");
  }

  // Turn off the depth buffer:
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  // Enable alpha blending:
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
  glViewport(0, 0, AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void az_start_screen_redraw(void) {
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
}

void az_finish_screen_redraw(void) {
  glFlush(); // Are the flush and finish at all necessary?  I'm not sure.
  glFinish();
  SDL_GL_SwapBuffers();
}

/*===========================================================================*/
