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
#ifndef AZIMUTH_STATE_NODE_H_
#define AZIMUTH_STATE_NODE_H_

#include "azimuth/state/script.h"
#include "azimuth/state/uid.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// The number of different node kinds there are, not counting AZ_NODE_NOTHING:
#define AZ_NUM_NODE_KINDS 7

typedef enum {
  AZ_NODE_NOTHING = 0,
  AZ_NODE_SAVE_POINT,
  AZ_NODE_TRACTOR,
  AZ_NODE_UPGRADE,
  AZ_NODE_REFILL,
  AZ_NODE_COMM,
  AZ_NODE_DOODAD_FG,
  AZ_NODE_DOODAD_BG
} az_node_kind_t;

// The number of different doodad kinds there are:
#define AZ_NUM_DOODAD_KINDS 4

typedef enum {
  AZ_DOOD_WARNING_LIGHT,
  AZ_DOOD_PIPE_STRAIGHT,
  AZ_DOOD_PIPE_CORNER,
  AZ_DOOD_PIPE_TEE
} az_doodad_kind_t;

typedef union {
  az_upgrade_t upgrade;
  az_doodad_kind_t doodad;
} az_node_subkind_t;

typedef struct {
  az_node_kind_t kind; // if AZ_NODE_NOTHING, this node is not present
  az_node_subkind_t subkind;
  az_uid_t uid;
  const az_script_t *on_use; // not owned; NULL if no script
  az_vector_t position;
  double angle;
} az_node_t;

#define AZ_NODE_BOUNDING_RADIUS 50.0

/*===========================================================================*/

#endif // AZIMUTH_STATE_NODE_H_
