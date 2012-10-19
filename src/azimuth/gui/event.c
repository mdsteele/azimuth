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

#include "azimuth/gui/event.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h> // for exit and EXIT_SUCCESS

#include <SDL/SDL.h>

#include "azimuth/gui/screen.h"
#include "azimuth/system/misc.h"

/*===========================================================================*/

// On Macs, we use the Command key (which SDL recognizes as the Meta key); on
// other platforms we use the Control key.
#ifdef __APPLE__
#define AZ_KMOD_CMD KMOD_META
#else
#define AZ_KMOD_CMD KMOD_CTRL
#endif

static az_key_name_t sdl_key_to_az_key(SDLKey key) {
  switch (key) {
    case SDLK_c: return AZ_KEY_C;
    case SDLK_v: return AZ_KEY_V;
    case SDLK_x: return AZ_KEY_X;
    case SDLK_z: return AZ_KEY_Z;
    case SDLK_UP: return AZ_KEY_UP_ARROW;
    case SDLK_DOWN: return AZ_KEY_DOWN_ARROW;
    case SDLK_LEFT: return AZ_KEY_LEFT_ARROW;
    case SDLK_RIGHT: return AZ_KEY_RIGHT_ARROW;
    default: return AZ_KEY_UNKNOWN;
  }
}

static void pause_until_refocus(void) {
  SDL_Event sdl_event;
  while (true) {
    while (SDL_PollEvent(&sdl_event)) {
      switch (sdl_event.type) {
        case SDL_ACTIVEEVENT:
          if (sdl_event.active.gain == 1 &&
              (sdl_event.active.state & SDL_APPINPUTFOCUS)) {
            return;
          }
          continue;
        case SDL_QUIT:
          exit(EXIT_SUCCESS);
        default: continue;
      }
    }
    az_sleep_millis(100);
  }
}

bool az_poll_event(az_event_t *event) {
  SDL_Event sdl_event;
  while (SDL_PollEvent(&sdl_event)) {
    switch (sdl_event.type) {
      case SDL_ACTIVEEVENT:
        if (sdl_event.active.gain == 0 &&
            (sdl_event.active.state & SDL_APPINPUTFOCUS)) {
          pause_until_refocus();
        }
        continue;
      case SDL_KEYDOWN:
        if (sdl_event.key.keysym.mod & AZ_KMOD_CMD) {
          // For Command/Ctrl-M, toggle fullscreen, then get the next event.
          if (sdl_event.key.keysym.sym == SDLK_m) {
            az_set_fullscreen(!az_is_fullscreen());
            continue;
          }
          // For Command/Ctrl-Q, quit the game.
          else if (sdl_event.key.keysym.sym == SDLK_q) {
            exit(EXIT_SUCCESS);
          }
        }
        event->kind = AZ_EVENT_KEY_DOWN;
        event->key.name = sdl_key_to_az_key(sdl_event.key.keysym.sym);
        return true;
      case SDL_KEYUP:
        event->kind = AZ_EVENT_KEY_UP;
        event->key.name = sdl_key_to_az_key(sdl_event.key.keysym.sym);
        return true;
      case SDL_MOUSEBUTTONDOWN:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_DOWN;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        return true;
      case SDL_MOUSEBUTTONUP:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_UP;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        return true;
      case SDL_MOUSEMOTION:
        event->kind = AZ_EVENT_MOUSE_MOVE;
        event->mouse.x = sdl_event.motion.x;
        event->mouse.y = sdl_event.motion.y;
        return true;
      case SDL_QUIT:
        exit(EXIT_SUCCESS);
      default: continue;
    }
    assert(false); // unreachable
  }
  return false;
}

/*===========================================================================*/
