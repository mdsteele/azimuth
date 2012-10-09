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
#ifndef AZIMUTH_STATE_SPACE_H_
#define AZIMUTH_STATE_SPACE_H_

#include <stdbool.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/node.h"
#include "azimuth/state/particle.h"
#include "azimuth/state/pickup.h"
#include "azimuth/state/player.h"
#include "azimuth/state/projectile.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/uid.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef struct {
  double active_for; // negative if timer is inactive
  double time_remaining; // seconds
} az_timer_t;

typedef struct {
  unsigned long clock;
  az_vector_t camera;
  az_ship_t ship;
  az_timer_t timer;
  az_baddie_t baddies[100];
  az_node_t nodes[50];
  az_particle_t particles[500];
  az_pickup_t pickups[100];
  az_projectile_t projectiles[250];
} az_space_state_t;

bool az_insert_baddie(az_space_state_t *state, az_baddie_t **baddie_out);

bool az_lookup_node(az_space_state_t *state, az_uid_t uid,
                    az_node_t **node_out);
bool az_insert_node(az_space_state_t *state, az_node_t **node_out);

bool az_insert_particle(az_space_state_t *state, az_particle_t **particle_out);

bool az_insert_projectile(az_space_state_t *state, az_projectile_t **proj_out);

void az_try_add_pickup(az_space_state_t *state, az_pickup_kind_t kind,
                       az_vector_t position);

/*===========================================================================*/

#endif // AZIMUTH_STATE_SPACE_H_
