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

#include <SDL.h>

#include "azimuth/gui/audio.h"
#include "azimuth/gui/screen.h"
#include "azimuth/util/misc.h" // for AZ_ASSERT_UNREACHABLE

/*===========================================================================*/

// On Macs, we use the Command key (which SDL recognizes as the Meta key); on
// other platforms we use the Control key.
#ifdef __APPLE__
#define AZ_KMOD_CMD KMOD_GUI
#else
#define AZ_KMOD_CMD KMOD_CTRL
#endif

static SDL_Keycode az_key_to_sdl_key(az_key_id_t key) {
  assert(key != AZ_KEY_UNKNOWN);
  switch (key) {
    case AZ_KEY_UNKNOWN: AZ_ASSERT_UNREACHABLE();
    case AZ_KEY_ESCAPE: return SDLK_ESCAPE;
    case AZ_KEY_BACKTICK: return SDLK_BACKQUOTE;
    case AZ_KEY_HYPHEN: return SDLK_MINUS;
    case AZ_KEY_EQUALS: return SDLK_EQUALS;
    case AZ_KEY_BACKSPACE: return SDLK_BACKSPACE;
    case AZ_KEY_TAB: return SDLK_TAB;
    case AZ_KEY_LEFT_BRACKET: return SDLK_LEFTBRACKET;
    case AZ_KEY_RIGHT_BRACKET: return SDLK_RIGHTBRACKET;
    case AZ_KEY_BACKSLASH: return SDLK_BACKSLASH;
    case AZ_KEY_SEMICOLON: return SDLK_SEMICOLON;
    case AZ_KEY_QUOTE: return SDLK_QUOTE;
    case AZ_KEY_RETURN: return SDLK_RETURN;
    case AZ_KEY_COMMA: return SDLK_COMMA;
    case AZ_KEY_PERIOD: return SDLK_PERIOD;
    case AZ_KEY_SLASH: return SDLK_SLASH;
    case AZ_KEY_SPACE: return SDLK_SPACE;
    case AZ_KEY_UP_ARROW: return SDLK_UP;
    case AZ_KEY_DOWN_ARROW: return SDLK_DOWN;
    case AZ_KEY_LEFT_ARROW: return SDLK_LEFT;
    case AZ_KEY_RIGHT_ARROW: return SDLK_RIGHT;
    case AZ_KEY_0: return SDLK_0;
    case AZ_KEY_1: return SDLK_1;
    case AZ_KEY_2: return SDLK_2;
    case AZ_KEY_3: return SDLK_3;
    case AZ_KEY_4: return SDLK_4;
    case AZ_KEY_5: return SDLK_5;
    case AZ_KEY_6: return SDLK_6;
    case AZ_KEY_7: return SDLK_7;
    case AZ_KEY_8: return SDLK_8;
    case AZ_KEY_9: return SDLK_9;
    case AZ_KEY_A: return SDLK_a;
    case AZ_KEY_B: return SDLK_b;
    case AZ_KEY_C: return SDLK_c;
    case AZ_KEY_D: return SDLK_d;
    case AZ_KEY_E: return SDLK_e;
    case AZ_KEY_F: return SDLK_f;
    case AZ_KEY_G: return SDLK_g;
    case AZ_KEY_H: return SDLK_h;
    case AZ_KEY_I: return SDLK_i;
    case AZ_KEY_J: return SDLK_j;
    case AZ_KEY_K: return SDLK_k;
    case AZ_KEY_L: return SDLK_l;
    case AZ_KEY_M: return SDLK_m;
    case AZ_KEY_N: return SDLK_n;
    case AZ_KEY_O: return SDLK_o;
    case AZ_KEY_P: return SDLK_p;
    case AZ_KEY_Q: return SDLK_q;
    case AZ_KEY_R: return SDLK_r;
    case AZ_KEY_S: return SDLK_s;
    case AZ_KEY_T: return SDLK_t;
    case AZ_KEY_U: return SDLK_u;
    case AZ_KEY_V: return SDLK_v;
    case AZ_KEY_W: return SDLK_w;
    case AZ_KEY_X: return SDLK_x;
    case AZ_KEY_Y: return SDLK_y;
    case AZ_KEY_Z: return SDLK_z;
    case AZ_KEY_INSERT: return SDLK_INSERT;
    case AZ_KEY_DELETE: return SDLK_DELETE;
    case AZ_KEY_LEFT_ALT: return SDLK_LALT;
    case AZ_KEY_RIGHT_ALT: return SDLK_RALT;
    case AZ_KEY_LEFT_CONTROL: return SDLK_LCTRL;
    case AZ_KEY_RIGHT_CONTROL: return SDLK_RCTRL;
    case AZ_KEY_LEFT_SHIFT: return SDLK_LSHIFT;
    case AZ_KEY_RIGHT_SHIFT: return SDLK_RSHIFT;
    case AZ_KEY_LEFT_GUI: return SDLK_LGUI;
    case AZ_KEY_RIGHT_GUI: return SDLK_RGUI;
    case AZ_KEY_PRINT_SCREEN: return SDLK_PRINTSCREEN;
    case AZ_KEY_SCROLL_LOCK: return SDLK_SCROLLLOCK;
    case AZ_KEY_HOME: return SDLK_HOME;
    case AZ_KEY_END: return SDLK_END;
    case AZ_KEY_PAGE_UP: return SDLK_PAGEUP;
    case AZ_KEY_PAGE_DOWN: return SDLK_PAGEDOWN;
    case AZ_KEY_PAUSE: return SDLK_PAUSE;
    case AZ_NUM_ALLOWED_KEYS: AZ_ASSERT_UNREACHABLE();
  }
  AZ_ASSERT_UNREACHABLE();
}

static az_key_id_t sdl_key_to_az_key(SDL_Keycode key) {
  switch (key) {
    case SDLK_ESCAPE: return AZ_KEY_ESCAPE;
    case SDLK_BACKQUOTE: return AZ_KEY_BACKTICK;
    case SDLK_MINUS: return AZ_KEY_HYPHEN;
    case SDLK_EQUALS: return AZ_KEY_EQUALS;
    case SDLK_BACKSPACE: return AZ_KEY_BACKSPACE;
    case SDLK_TAB: return AZ_KEY_TAB;
    case SDLK_LEFTBRACKET: return AZ_KEY_LEFT_BRACKET;
    case SDLK_RIGHTBRACKET: return AZ_KEY_RIGHT_BRACKET;
    case SDLK_BACKSLASH: return AZ_KEY_BACKSLASH;
    case SDLK_SEMICOLON: return AZ_KEY_SEMICOLON;
    case SDLK_QUOTE: return AZ_KEY_QUOTE;
    case SDLK_RETURN: return AZ_KEY_RETURN;
    case SDLK_COMMA: return AZ_KEY_COMMA;
    case SDLK_PERIOD: return AZ_KEY_PERIOD;
    case SDLK_SLASH: return AZ_KEY_SLASH;
    case SDLK_SPACE: return AZ_KEY_SPACE;
    case SDLK_UP: return AZ_KEY_UP_ARROW;
    case SDLK_DOWN: return AZ_KEY_DOWN_ARROW;
    case SDLK_LEFT: return AZ_KEY_LEFT_ARROW;
    case SDLK_RIGHT: return AZ_KEY_RIGHT_ARROW;
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
    case SDLK_INSERT: return AZ_KEY_INSERT;
    case SDLK_DELETE: return AZ_KEY_DELETE;
    case SDLK_LALT: return AZ_KEY_LEFT_ALT;
    case SDLK_RALT: return AZ_KEY_RIGHT_ALT;
    case SDLK_LCTRL: return AZ_KEY_LEFT_CONTROL;
    case SDLK_RCTRL: return AZ_KEY_RIGHT_CONTROL;
    case SDLK_LSHIFT: return AZ_KEY_LEFT_SHIFT;
    case SDLK_RSHIFT: return AZ_KEY_RIGHT_SHIFT;
    case SDLK_LGUI: return AZ_KEY_LEFT_GUI;
    case SDLK_RGUI: return AZ_KEY_RIGHT_GUI;
    case SDLK_PRINTSCREEN: return AZ_KEY_PRINT_SCREEN;
    case SDLK_SCROLLLOCK: return AZ_KEY_SCROLL_LOCK;
    case SDLK_HOME: return AZ_KEY_HOME;
    case SDLK_END: return AZ_KEY_END;
    case SDLK_PAGEUP: return AZ_KEY_PAGE_UP;
    case SDLK_PAGEDOWN: return AZ_KEY_PAGE_DOWN;
    case SDLK_PAUSE: return AZ_KEY_PAUSE;
    default: return AZ_KEY_UNKNOWN;
  }
}

static void pause_until_refocus(void) {
  az_pause_all_audio();
  SDL_Event sdl_event;
  while (true) {
    while (SDL_PollEvent(&sdl_event)) {
      switch (sdl_event.type) {
        case SDL_WINDOWEVENT:
          if (sdl_event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            az_unpause_all_audio();
            return;
          }
          continue;
        case SDL_QUIT:
          exit(EXIT_SUCCESS);
        default: continue;
      }
    }
    SDL_Delay(100);
  }
}

bool az_poll_event(az_event_t *event) {
  SDL_Event sdl_event;
  while (SDL_PollEvent(&sdl_event)) {
    switch (sdl_event.type) {
      case SDL_WINDOWEVENT:
        if (sdl_event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          pause_until_refocus();
        }
        continue;
      case SDL_KEYDOWN:
        if (sdl_event.key.repeat) {
          continue;
        }
        if ((sdl_event.key.keysym.mod & AZ_KMOD_CMD) &&
            !(sdl_event.key.keysym.mod & KMOD_SHIFT)) {
          // For Command/Ctrl-F, toggle fullscreen, then get the next event.
          if (sdl_event.key.keysym.sym == SDLK_f) {
            az_set_fullscreen(!az_is_fullscreen());
            continue;
          }
          // For Command/Ctrl-Q, quit the game.
          else if (sdl_event.key.keysym.sym == SDLK_q) {
            exit(EXIT_SUCCESS);
          }
        }
        event->kind = AZ_EVENT_KEY_DOWN;
        event->key.id = sdl_key_to_az_key(sdl_event.key.keysym.sym);
        event->key.command = (bool)(sdl_event.key.keysym.mod & AZ_KMOD_CMD);
        event->key.shift = (bool)(sdl_event.key.keysym.mod & KMOD_SHIFT);
        event->key.character = 0;
        return true;
      case SDL_TEXTINPUT:
        // The key.character consumer in src/editor/main.c only
        // uses key.character so 'zero' out the other members
        event->kind = AZ_EVENT_KEY_DOWN;
        event->key.id = AZ_KEY_UNKNOWN;
        event->key.command = false;
        event->key.shift = false;
        // The key.character consumer in src/editor/main.c only
        // accepts printable low-ASCII so just send it the the
        // first UTF-8 character from the text array
        event->key.character = sdl_event.text.text[0];
        return true;
      case SDL_KEYUP:
        if (sdl_event.key.repeat) {
          continue;
        }
        event->kind = AZ_EVENT_KEY_UP;
        event->key.id = sdl_key_to_az_key(sdl_event.key.keysym.sym);
        event->key.command = (bool)(sdl_event.key.keysym.mod & AZ_KMOD_CMD);
        event->key.shift = (bool)(sdl_event.key.keysym.mod & KMOD_SHIFT);
        event->key.character = 0;
        return true;
      case SDL_MOUSEBUTTONDOWN:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_DOWN;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        az_map_mouse_coords(event->mouse.x, event->mouse.y, &event->mouse.x, &event->mouse.y);
        event->mouse.dx = event->mouse.dy = 0;
        event->mouse.pressed = true;
        return true;
      case SDL_MOUSEBUTTONUP:
        // Ignore all but the left mouse button.
        if (sdl_event.button.button != SDL_BUTTON_LEFT) continue;
        event->kind = AZ_EVENT_MOUSE_UP;
        event->mouse.x = sdl_event.button.x;
        event->mouse.y = sdl_event.button.y;
        az_map_mouse_coords(event->mouse.x, event->mouse.y, &event->mouse.x, &event->mouse.y);
        event->mouse.dx = event->mouse.dy = 0;
        event->mouse.pressed = false;
        return true;
      case SDL_MOUSEMOTION:
        event->kind = AZ_EVENT_MOUSE_MOVE;
        event->mouse.x = sdl_event.motion.x;
        event->mouse.y = sdl_event.motion.y;
        az_map_mouse_coords(event->mouse.x, event->mouse.y, &event->mouse.x, &event->mouse.y);
        event->mouse.dx = sdl_event.motion.xrel;
        event->mouse.dy = sdl_event.motion.yrel;
        event->mouse.pressed = (bool)(sdl_event.motion.state &
                                      SDL_BUTTON(SDL_BUTTON_LEFT));
        return true;
      case SDL_QUIT:
        exit(EXIT_SUCCESS);
      default: continue;
    }
    AZ_ASSERT_UNREACHABLE();
  }
  return false;
}

bool az_get_mouse_position(int *x, int *y) {
  if (!az_has_mousefocus()) return false;
  SDL_GetMouseState(x, y);
  az_map_mouse_coords(*x, *y, x, y);
  return true;
}

bool az_is_mouse_held(void) {
  return (bool)(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT));
}

bool az_is_key_held(az_key_id_t key) {
  assert(key != AZ_KEY_UNKNOWN);
  return (bool)SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(az_key_to_sdl_key(key))];
}

bool az_is_shift_key_held(void) {
  return (bool)(SDL_GetModState() & KMOD_SHIFT);
}

/*===========================================================================*/
