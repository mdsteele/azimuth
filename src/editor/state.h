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

#include "azimuth/state/dialog.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/room.h"
#include "azimuth/state/wall.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/vector.h"
#include "editor/list.h"

/*===========================================================================*/

typedef enum {
  AZ_TOOL_MOVE = 0,
  AZ_TOOL_MASS_MOVE,
  AZ_TOOL_ROTATE,
  AZ_TOOL_MASS_ROTATE,
  AZ_TOOL_CAMERA,
  AZ_TOOL_BADDIE,
  AZ_TOOL_DOOR,
  AZ_TOOL_GRAVFIELD,
  AZ_TOOL_NODE,
  AZ_TOOL_WALL
} az_editor_tool_t;

typedef enum {
  AZ_ETA_NOTHING = 0,
  AZ_ETA_EDIT_GRAVFIELD,
  AZ_ETA_EDIT_SCRIPT,
  AZ_ETA_SET_CURRENT_ROOM,
  AZ_ETA_SET_DOOR_DEST,
  AZ_ETA_SET_UUID_SLOT
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
  az_gravfield_spec_t spec;
} az_editor_gravfield_t;

typedef struct {
  bool selected;
  az_node_spec_t spec;
} az_editor_node_t;

typedef struct {
  bool selected;
  az_wall_spec_t spec;
} az_editor_wall_t;

typedef struct {
  bool selected;
  int zone_index;
  az_camera_bounds_t camera_bounds;
  az_script_t *on_start;
  AZ_LIST_DECLARE(az_editor_baddie_t, baddies);
  AZ_LIST_DECLARE(az_editor_door_t, doors);
  AZ_LIST_DECLARE(az_editor_gravfield_t, gravfields);
  AZ_LIST_DECLARE(az_editor_node_t, nodes);
  AZ_LIST_DECLARE(az_editor_wall_t, walls);
} az_editor_room_t;

typedef struct {
  az_clock_t clock;
  double total_time;
  bool unsaved; // true if we currently have unsaved changes
  bool spin_camera;
  az_vector_t camera;
  double zoom_level;
  struct {
    bool up, down, left, right;
  } controls;
  struct {
    az_editor_text_action_t action; // if AZ_ETA_NOTHING, text box inactive
    int cursor;
    int length;
    char buffer[100];
  } text;
  az_editor_tool_t tool;
  struct {
    double angle;
    az_baddie_kind_t baddie_kind;
    az_door_kind_t door_kind;
    az_gravfield_kind_t gravfield_kind;
    az_node_kind_t node_kind;
    az_doodad_kind_t doodad_kind;
    az_upgrade_t upgrade_kind;
    az_wall_kind_t wall_kind;
    int wall_data_index;
    int zone_index;
  } brush;
  az_room_key_t current_room;
  struct {
    az_room_key_t start_room;
    az_vector_t start_position;
    double start_angle;
    AZ_LIST_DECLARE(az_text_t, texts);
    AZ_LIST_DECLARE(az_zone_t, zones);
    AZ_LIST_DECLARE(az_editor_room_t, rooms);
  } planet;
} az_editor_state_t;

/*===========================================================================*/

// Load and initialize the editor state from disk.  Return false on failure.
bool az_load_editor_state(az_editor_state_t *state);

// Save the editor state to disk.  Return false on failure, otherwise set
// state->unsaved to false and return true.
bool az_save_editor_state(az_editor_state_t *state);

void az_tick_editor_state(az_editor_state_t *state, double time);

void az_init_editor_text(
    az_editor_state_t *state, az_editor_text_action_t action,
    const char *format, ...) __attribute__((__format__(__printf__,3,4)));

bool az_editor_is_in_minimap_mode(const az_editor_state_t *state);

void az_center_editor_camera_on_current_room(az_editor_state_t *state);

// Delete the data arrays owned by the editor state (but not the state object
// itself).
void az_destroy_editor_state(az_editor_state_t *state);

/*===========================================================================*/

#endif // EDITOR_STATE_H_
