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
#include "azimuth/state/wall.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"
#include "editor/list.h"

/*===========================================================================*/

typedef enum {
  AZ_TOOL_MOVE = 0,
  AZ_TOOL_ROTATE,
  AZ_TOOL_CAMERA,
  AZ_TOOL_BADDIE,
  AZ_TOOL_DOOR,
  AZ_TOOL_NODE,
  AZ_TOOL_WALL
} az_editor_tool_t;

typedef enum {
  AZ_ETA_NOTHING = 0,
  AZ_ETA_SET_CURRENT_ROOM,
  AZ_ETA_SET_DOOR_DEST
} az_editor_text_action_t;

typedef struct {
  bool selected;
  az_baddie_spec_t spec;
} az_editor_baddie_t;

typedef struct {
  bool selected;
  az_door_spec_t spec;
} az_editor_door_t;

typedef struct {
  bool selected;
  az_node_spec_t spec;
} az_editor_node_t;

typedef struct {
  bool selected;
  az_wall_t spec;
} az_editor_wall_t;

typedef struct {
  az_camera_bounds_t camera_bounds;
  AZ_LIST_DECLARE(az_editor_baddie_t, baddies);
  AZ_LIST_DECLARE(az_editor_door_t, doors);
  AZ_LIST_DECLARE(az_editor_node_t, nodes);
  AZ_LIST_DECLARE(az_editor_wall_t, walls);
} az_editor_room_t;

typedef struct {
  az_clock_t clock;
  bool unsaved; // true if we currently have unsaved changes
  bool spin_camera;
  az_vector_t camera;
  struct {
    bool up, down, left, right;
  } controls;
  struct {
    az_editor_text_action_t action; // if AZ_ETA_NOTHING, text box inactive
    int length;
    char buffer[100];
  } text;
  az_editor_tool_t tool;
  struct {
    az_baddie_kind_t baddie_kind;
    az_door_kind_t door_kind;
    az_node_kind_t node_kind;
    az_upgrade_t upgrade_kind;
    int wall_data_index;
  } brush;
  az_room_key_t current_room;
  struct {
    az_room_key_t start_room;
    az_vector_t start_position;
    double start_angle;
    AZ_LIST_DECLARE(az_editor_room_t, rooms);
  } planet;
} az_editor_state_t;

void az_tick_editor_state(az_editor_state_t *state);

/*===========================================================================*/

#endif // EDITOR_STATE_H_
