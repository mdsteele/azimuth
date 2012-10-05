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

#include "azimuth/player.h"
#include "azimuth/screen.h"
#include "azimuth/space.h"
#include "azimuth/vector.h"
#include "azimuth/view/space.h"

#define VIDEO_DEPTH 32
#define VIDEO_FLAGS (SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL)

static void event_loop(void) {
  az_controls_t controls = {false};
  az_space_state_t state;
  az_player_t player = {.shields = 55, .max_shields = 400, .energy = 98,
                        .max_energy = 100, .gun1 = AZ_GUN_HOMING,
                        .gun2 = AZ_GUN_BURST,
                        .rockets = 63, .max_rockets = 75,
                        .bombs = 15, .max_bombs = 15,
                        .ordnance = AZ_ORDN_BOMBS};
  state.clock = 0;
  state.ship.position = (az_vector_t){50, 150};
  state.ship.velocity = (az_vector_t){-20, 10};
  state.ship.angle = 2.9;
  state.ship.player = &player;
  state.camera = AZ_VZERO;

  while (true) {
    // Tick the state:
    az_tick_space_state(&state, &controls, 1.0/60.0);
    // Draw the screen:
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    az_space_draw_screen(&state);
    glFlush(); // Are the flush and finish at all necessary?  I'm not sure.
    glFinish();
    SDL_GL_SwapBuffers();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        return;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_q:
          if (event.key.keysym.mod & (KMOD_CTRL | KMOD_META)) {
            return;
          }
          break;
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
