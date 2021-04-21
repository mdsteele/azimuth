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

#include <assert.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "azimuth/constants.h"
#include "azimuth/gui/audio.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

#ifdef __APPLE__
// The UTF-8 sequence for the Apple command key symbol (U+2318):
#define CMD_KEY_NAME "\xE2\x8C\x98"
#else
#define CMD_KEY_NAME "Ctrl-"
#endif

static bool sdl_initialized = false;
static bool display_initialized = false;
static bool currently_fullscreen = false;
static int num_gl_init_funcs = 0;
static az_init_func_t gl_init_funcs[8];
static SDL_Window* window = NULL;
static SDL_GLContext context = NULL;
static int current_screen_width = AZ_SCREEN_WIDTH;
static int current_screen_height = AZ_SCREEN_HEIGHT;
static float current_screen_scale = 1.0f;
static float current_screen_xoffset = 0;
static float current_screen_yoffset = 0;
static double nanoseconds_per_count = 1000000000;

// Get the current time in nanoseconds, as measured from some unspecified zero
// point.  Not guaranteed to be monotonic.
static uint64_t az_current_time_nanos(void) {
  return SDL_GetPerformanceCounter() * nanoseconds_per_count;
}

// Sleep until az_current_time_nanos() would return the given time value, and
// then return the time at which the sleep ended.  Return the current time
// immediately without sleeping if it is already past the requested time.
static uint64_t az_sleep_until(uint64_t time) {
  const uint64_t now = az_current_time_nanos();
  if (time <= now) return now;
  const Uint32 milliseconds = (time - now) / 1000000;
  if (milliseconds > 0) SDL_Delay(milliseconds);
  return az_current_time_nanos();
}

void az_register_gl_init_func(az_init_func_t func) {
  assert(!sdl_initialized);
  if (num_gl_init_funcs >= AZ_ARRAY_SIZE(gl_init_funcs)) {
    AZ_FATAL("gl_init_funcs array is full.\n");
  } else {
    gl_init_funcs[num_gl_init_funcs++] = func;
  }
}

void az_init_gui(bool fullscreen, bool enable_audio) {
  SDL_DisplayMode display_mode = {0};
  assert(!sdl_initialized);
  if (SDL_Init(SDL_INIT_VIDEO | (enable_audio ? SDL_INIT_AUDIO : 0)) != 0) {
    AZ_FATAL("SDL_Init failed: %s\n", SDL_GetError());
  }
  atexit(SDL_Quit);
  nanoseconds_per_count = 1000000000 / (double)SDL_GetPerformanceFrequency();
  if (enable_audio) {
    az_init_audio();
  }
  // Enable OpenGL double-buffering:
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  // Enable antialiasing:
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
  SDL_GetDesktopDisplayMode(0, &display_mode);
  window = SDL_CreateWindow(
    "Azimuth",
    SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0),
    display_mode.w, display_mode.h,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
  if (NULL == window) {
    AZ_FATAL("SDL_CreateWindow failed: %s\n", SDL_GetError());
  }
  // TODO: set window title on maximize/minimize in event.c somehow?
  SDL_SetWindowTitle(window, "Azimuth (press " CMD_KEY_NAME "F to run full-screen)");
  context = SDL_GL_CreateContext(window);
  if (!context) {
      AZ_FATAL("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
  }

  sdl_initialized = true;
  az_set_fullscreen(fullscreen);
  SDL_ShowCursor(SDL_DISABLE);

#ifndef NDEBUG
  fprintf(stderr, "Using GL implementation:\n"
          "    Vendor: %s\n"
          "  Renderer: %s\n"
          "   Version: %s\n",
          glGetString(GL_VENDOR), glGetString(GL_RENDERER),
          glGetString(GL_VERSION));
#endif
}

void az_deinit_gui(void) {
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

bool az_is_fullscreen(void) {
  assert(sdl_initialized);
  assert(display_initialized);
  return currently_fullscreen;
}

void az_set_fullscreen(bool fullscreen) {
  int display_index = 0;
  SDL_DisplayMode display_mode = {0};
  assert(sdl_initialized);
  if (display_initialized && fullscreen == currently_fullscreen) return;
  currently_fullscreen = fullscreen;
  az_pause_all_audio();

  // Init the display:
  int x = 0, y = 0;
  if (display_initialized) {
    SDL_GetMouseState(&x, &y);
  }

  display_index = SDL_GetWindowDisplayIndex(window);
  SDL_GetDesktopDisplayMode(display_index, &display_mode);
  SDL_SetWindowDisplayMode(window, &display_mode);
  if(0 != SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0)) {
      AZ_FATAL("SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
  }

  // Enable vsync:
  const int vsync_result = SDL_GL_SetSwapInterval(1);
  if (vsync_result != 0) {
    AZ_WARNING_ALWAYS("Failed to enable vsync: %s\n", SDL_GetError());
  }
  if (display_initialized) {
    SDL_WarpMouseInWindow(window, x, y);
  }

  // Turn off the depth buffer:
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  // Enable alpha blending:
  glEnable(GL_BLEND);
#ifndef WIN32
  glBlendEquation(GL_FUNC_ADD);
#endif
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // Set antialiasing:
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glDisable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  // Set the view:
  glClearColor(0, 0, 0, 0);
  glClearDepth(1.0f);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT, 0, 1, -1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Run GL init functions:
  for (int i = 0; i < num_gl_init_funcs; ++i) {
    gl_init_funcs[i]();
  }

  display_initialized = true;
  az_unpause_all_audio();
}

bool az_has_mousefocus(void) {
  assert(sdl_initialized);
  return (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS);
}

void az_start_screen_redraw(void) {
  assert(sdl_initialized);
  assert(display_initialized);
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  // Calculate letterboxing scaling factor & offsets
  SDL_GL_GetDrawableSize(window, &current_screen_width, &current_screen_height);
  if (current_screen_width / (float)AZ_SCREEN_WIDTH >
      current_screen_height / (float)AZ_SCREEN_HEIGHT) {
	current_screen_scale = current_screen_height / (float)AZ_SCREEN_HEIGHT;
  } else {
	current_screen_scale = current_screen_width / (float)AZ_SCREEN_WIDTH;
  }
  current_screen_xoffset = (current_screen_width - (AZ_SCREEN_WIDTH * current_screen_scale)) / 2.0f;
  current_screen_yoffset = (current_screen_height - (AZ_SCREEN_HEIGHT * current_screen_scale)) / 2.0f;

  glLineWidth(current_screen_scale);
  glViewport(
    current_screen_xoffset,
    current_screen_yoffset,
    current_screen_scale * AZ_SCREEN_WIDTH,
    current_screen_scale * AZ_SCREEN_HEIGHT);
}

void az_finish_screen_redraw(void) {
  assert(sdl_initialized);
  assert(display_initialized);
  SDL_GL_SwapWindow(window);
  // Synchronize, in case vsync fails to lock us to 60Hz:
  static uint64_t sync_time = 0;
  sync_time = az_sleep_until(sync_time) + AZ_FRAME_TIME_NANOS;
}

void az_gl_scissor(int x, int y, int width, int height) {
  glScissor(
    (current_screen_scale * x) + current_screen_xoffset,
    (current_screen_scale * y) + current_screen_yoffset,
    current_screen_scale * width,
    current_screen_scale * height);
}

void az_map_mouse_coords(int x_in, int y_in, int *x_out, int *y_out) {
  if(x_out) {
    *x_out = (x_in - current_screen_xoffset) / current_screen_scale;
  }
  if(y_out) {
    *y_out = (y_in - current_screen_yoffset) / current_screen_scale;
  }
}

/*===========================================================================*/
