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

/*===========================================================================*/

typedef enum {
  AZ_EVENT_QUIT,
  AZ_EVENT_KEY_DOWN,
  AZ_EVENT_KEY_UP,
  AZ_EVENT_MOUSE_DOWN,
  AZ_EVENT_MOUSE_UP,
  AZ_EVENT_MOUSE_MOVE
} az_event_kind_t;

typedef enum {
  AZ_KEY_UNKNOWN = 0,
  AZ_KEY_C = 'C',
  AZ_KEY_V = 'V',
  AZ_KEY_X = 'X',
  AZ_KEY_Z = 'Z',
  AZ_KEY_UP_ARROW = 0x80,
  AZ_KEY_DOWN_ARROW,
  AZ_KEY_LEFT_ARROW,
  AZ_KEY_RIGHT_ARROW
} az_key_name_t;

typedef union {
  az_event_kind_t kind;
  struct {
    az_event_kind_t kind;
    az_key_name_t name;
  } key;
  struct {
    az_event_kind_t kind;
    int x, y;
  } mouse;
} az_event_t;

// Get the next event in the queue and return true, or return false if the
// event queue is empty.
bool az_poll_event(az_event_t *event);

/*===========================================================================*/
