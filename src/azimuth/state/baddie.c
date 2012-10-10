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

#include "azimuth/state/baddie.h"

#include <assert.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static const az_baddie_data_t baddie_data[] = {
  [AZ_BAD_LUMP] = {
    .bounding_radius = 20.0,
    .max_health = 10.0
  },
  [AZ_BAD_TURRET] = {
    .bounding_radius = 35.0,
    .max_health = 15.0
  }
};

void az_init_baddie(az_baddie_t *baddie, az_baddie_kind_t kind,
                    az_vector_t position, double angle) {
  assert(kind != AZ_BAD_NOTHING);
  baddie->kind = kind;
  const int data_index = (int)kind;
  assert(0 <= data_index && data_index < AZ_ARRAY_SIZE(baddie_data));
  baddie->data = &baddie_data[data_index];
  baddie->position = position;
  baddie->velocity = AZ_VZERO;
  baddie->angle = angle;
  baddie->health = baddie->data->max_health;
  baddie->cooldown = 0.0;
}

/*===========================================================================*/
