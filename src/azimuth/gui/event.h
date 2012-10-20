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

#pragma once
#ifndef AZIMUTH_GUI_EVENT_H_
#define AZIMUTH_GUI_EVENT_H_

#include <stdbool.h>

/*===========================================================================*/

typedef enum {
  AZ_EVENT_KEY_DOWN,
  AZ_EVENT_KEY_UP,
  AZ_EVENT_MOUSE_DOWN,
  AZ_EVENT_MOUSE_UP,
  AZ_EVENT_MOUSE_MOVE
} az_event_kind_t;

typedef enum {
  AZ_KEY_UNKNOWN = 0,
  AZ_KEY_RETURN = '\n',
  AZ_KEY_SPACE = ' ',
  AZ_KEY_0 = '0',
  AZ_KEY_1 = '1',
  AZ_KEY_2 = '2',
  AZ_KEY_3 = '3',
  AZ_KEY_4 = '4',
  AZ_KEY_5 = '5',
  AZ_KEY_6 = '6',
  AZ_KEY_7 = '7',
  AZ_KEY_8 = '8',
  AZ_KEY_9 = '9',
  AZ_KEY_A = 'A',
  AZ_KEY_B = 'B',
  AZ_KEY_C = 'C',
  AZ_KEY_D = 'D',
  AZ_KEY_E = 'E',
  AZ_KEY_F = 'F',
  AZ_KEY_G = 'G',
  AZ_KEY_H = 'H',
  AZ_KEY_I = 'I',
  AZ_KEY_J = 'J',
  AZ_KEY_K = 'K',
  AZ_KEY_L = 'L',
  AZ_KEY_M = 'M',
  AZ_KEY_N = 'N',
  AZ_KEY_O = 'O',
  AZ_KEY_P = 'P',
  AZ_KEY_Q = 'Q',
  AZ_KEY_R = 'R',
  AZ_KEY_S = 'S',
  AZ_KEY_T = 'T',
  AZ_KEY_U = 'U',
  AZ_KEY_V = 'V',
  AZ_KEY_W = 'W',
  AZ_KEY_X = 'X',
  AZ_KEY_Y = 'Y',
  AZ_KEY_Z = 'Z',
  AZ_KEY_BACKSPACE = 0x7f,
  AZ_KEY_UP_ARROW,
  AZ_KEY_DOWN_ARROW,
  AZ_KEY_LEFT_ARROW,
  AZ_KEY_RIGHT_ARROW
} az_key_name_t;

typedef union {
  az_event_kind_t kind;
  struct {
    az_event_kind_t kind;
    az_key_name_t name;
    bool command; // true if Command/Ctrl (depending on OS) key is held
  } key;
  struct {
    az_event_kind_t kind;
    int x, y; // current mouse position
    int dx, dy; // change in mouse position (for MOUSE_MOVE only)
    bool pressed; // true if left mouse button is held
  } mouse;
} az_event_t;

// Get the next event in the queue and return true, or return false if the
// event queue is empty.
bool az_poll_event(az_event_t *event);

// Get the current position of the mouse in the window and return true, or
// return false if the mouse is not currently in the window.
bool az_get_mouse_position(int *x, int *y);

/*===========================================================================*/

#endif // AZIMUTH_GUI_EVENT_H_
