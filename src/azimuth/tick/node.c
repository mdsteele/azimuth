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

#include "azimuth/tick/node.h"

#include <assert.h>

#include "azimuth/constants.h"
#include "azimuth/state/player.h"
#include "azimuth/state/node.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_tick_nodes(az_space_state_t *state, double time) {
  const bool has_tractor_beam = az_ship_is_present(&state->ship) &&
    az_has_upgrade(&state->ship.player, AZ_UPG_TRACTOR_BEAM);
  double best_tractor_dist = AZ_TRACTOR_BEAM_MAX_RANGE;
  az_node_t *closest_tractor_node = NULL;
  double best_console_dist = AZ_CONSOLE_RANGE;
  az_node_t *closest_console_node = NULL;
  AZ_ARRAY_LOOP(node, state->nodes) {
    switch (node->kind) {
      case AZ_NODE_TRACTOR:
        if (has_tractor_beam &&
            az_vwithin(state->ship.position, node->position,
                       AZ_TRACTOR_BEAM_MAX_RANGE)) {
          node->status = AZ_NS_NEAR;
          const double dist = az_vdist(state->ship.position, node->position);
          if (dist <= best_tractor_dist) {
            best_tractor_dist = dist;
            closest_tractor_node = node;
          }
        } else node->status = AZ_NS_FAR;
        break;
      case AZ_NODE_CONSOLE:
        if (az_vwithin(state->ship.position, node->position,
                       AZ_CONSOLE_RANGE)) {
          node->status = AZ_NS_NEAR;
          const double dist = az_vdist(state->ship.position, node->position);
          if (dist <= best_console_dist) {
            best_console_dist = dist;
            closest_console_node = node;
          }
        } else node->status = AZ_NS_FAR;
        break;
      default: break;
    }
  }
  if (closest_tractor_node != NULL) {
    closest_tractor_node->status = AZ_NS_READY;
  }
  if (state->ship.tractor_beam.active) {
    az_node_t *node;
    if (az_lookup_node(state, state->ship.tractor_beam.node_uid, &node)) {
      assert(node->kind == AZ_NODE_TRACTOR);
      node->status = AZ_NS_ACTIVE;
    }
  }
  if (closest_console_node != NULL) {
    closest_console_node->status = AZ_NS_READY;
  }
  if (state->mode == AZ_MODE_CONSOLE &&
      state->console_mode.step != AZ_CSS_ALIGN) {
    az_node_t *node;
    if (az_lookup_node(state, state->console_mode.node_uid, &node)) {
      assert(node->kind == AZ_NODE_CONSOLE);
      node->status = AZ_NS_ACTIVE;
    }
  }
}

/*===========================================================================*/
