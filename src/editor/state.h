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
#ifndef EDITOR_STATE_H_
#define EDITOR_STATE_H_

#include <stdbool.h>

#include "azimuth/state/room.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_TOOL_SELECT = 0,
  AZ_TOOL_MOVE,
  AZ_TOOL_ROTATE
} az_editor_tool_t;

typedef struct {
  az_vector_t camera;
  bool spin_camera;
  struct {
    bool up, down, left, right;
  } controls;
  az_editor_tool_t tool;
  az_room_t *room;
  az_wall_t *selected_wall;
} az_editor_state_t;

void az_tick_editor_state(az_editor_state_t *state);

/*===========================================================================*/

#endif // EDITOR_STATE_H_
