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
#include "editor/list.h"

/*===========================================================================*/

typedef enum {
  AZ_TOOL_MOVE = 0,
  AZ_TOOL_ROTATE,
  AZ_TOOL_BADDIE,
  AZ_TOOL_WALL
} az_editor_tool_t;

typedef struct {
  bool selected;
  az_baddie_spec_t spec;
} az_editor_baddie_t;

typedef struct {
  bool selected;
  az_wall_t spec;
} az_editor_wall_t;

typedef struct {
  bool unsaved;
  bool spin_camera;
  az_vector_t camera;
  struct {
    bool up, down, left, right;
  } controls;
  az_editor_tool_t tool;
  struct {
    int wall_data_index;
    az_baddie_kind_t baddie_kind;
  } brush;
  AZ_LIST_DECLARE(az_editor_baddie_t, baddies);
  AZ_LIST_DECLARE(az_editor_wall_t, walls);
} az_editor_state_t;

void az_tick_editor_state(az_editor_state_t *state);

/*===========================================================================*/

#endif // EDITOR_STATE_H_
