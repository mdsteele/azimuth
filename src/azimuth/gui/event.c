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
    case SDLK_RETURN: return AZ_KEY_RETURN;
    case SDLK_SPACE: return AZ_KEY_SPACE;
    case SDLK_0: return AZ_KEY_0;
    case SDLK_1: return AZ_KEY_1;
    case SDLK_2: return AZ_KEY_2;
    case SDLK_3: return AZ_KEY_3;
    case SDLK_4: return AZ_KEY_4;
    case SDLK_5: return AZ_KEY_5;
    case SDLK_6: return AZ_KEY_6;
    case SDLK_7: return AZ_KEY_7;
    case SDLK_8: return AZ_KEY_8;
    case SDLK_9: return AZ_KEY_9;
    case SDLK_a: return AZ_KEY_A;
    case SDLK_b: return AZ_KEY_B;
    case SDLK_c: return AZ_KEY_C;
    case SDLK_d: return AZ_KEY_D;
    case SDLK_e: return AZ_KEY_E;
    case SDLK_f: return AZ_KEY_F;
    case SDLK_g: return AZ_KEY_G;
    case SDLK_h: return AZ_KEY_H;
    case SDLK_i: return AZ_KEY_I;
    case SDLK_j: return AZ_KEY_J;
    case SDLK_k: return AZ_KEY_K;
    case SDLK_l: return AZ_KEY_L;
    case SDLK_m: return AZ_KEY_M;
    case SDLK_n: return AZ_KEY_N;
    case SDLK_o: return AZ_KEY_O;
    case SDLK_p: return AZ_KEY_P;
    case SDLK_q: return AZ_KEY_Q;
    case SDLK_r: return AZ_KEY_R;
    case SDLK_s: return AZ_KEY_S;
    case SDLK_t: return AZ_KEY_T;
    case SDLK_u: return AZ_KEY_U;
    case SDLK_v: return AZ_KEY_V;
    case SDLK_w: return AZ_KEY_W;
    case SDLK_x: return AZ_KEY_X;
    case SDLK_y: return AZ_KEY_Y;
    case SDLK_z: return AZ_KEY_Z;
    case SDLK_BACKSPACE: return AZ_KEY_BACKSPACE;
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
        event->key.command = (bool)(sdl_event.key.keysym.mod & AZ_KMOD_CMD);
        return true;
      case SDL_KEYUP:
        event->kind = AZ_EVENT_KEY_UP;
        event->key.name = sdl_key_to_az_key(sdl_event.key.keysym.sym);
        event->key.command = (bool)(sdl_event.key.keysym.mod & AZ_KMOD_CMD);
        return true;
      case SDL_MOUSEBUTTONDOWN:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_DOWN;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        event->mouse.dx = event->mouse.dy = 0;
        event->mouse.pressed = true;
        return true;
      case SDL_MOUSEBUTTONUP:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_UP;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        event->mouse.dx = event->mouse.dy = 0;
        event->mouse.pressed = false;
        return true;
      case SDL_MOUSEMOTION:
        event->kind = AZ_EVENT_MOUSE_MOVE;
        event->mouse.x = sdl_event.motion.x;
        event->mouse.y = sdl_event.motion.y;
        event->mouse.dx = sdl_event.motion.xrel;
        event->mouse.dy = sdl_event.motion.yrel;
        event->mouse.pressed = (bool)(sdl_event.motion.state &
                                      SDL_BUTTON(SDL_BUTTON_LEFT));
        return true;
      case SDL_QUIT:
        exit(EXIT_SUCCESS);
      default: continue;
    }
    assert(false); // unreachable
  }
  return false;
}

bool az_get_mouse_position(int *x, int *y) {
  if (!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) return false;
  SDL_GetMouseState(x, y);
  return true;
}

bool az_is_shift_key_held(void) {
  return (bool)(SDL_GetModState() & KMOD_SHIFT);
}

/*===========================================================================*/
