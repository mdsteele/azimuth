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

#include "azimuth/state/ship.h"

#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static const az_vector_t ship_vertices[] = {
  {20, 0}, {15, 4}, {6, 12}, {-10, 12}, {-11, 7}, {-14, 4},
  {-14, -4}, {-11, -7}, {-10, -12}, {6, -12}, {15, -4}
};

static const az_polygon_t ship_polygon = {
  .num_vertices = AZ_ARRAY_SIZE(ship_vertices),
  .vertices = ship_vertices
};

// Return true if the given point should count as intersecting the ship's
// shield perimeter.
bool az_point_hits_ship(const az_ship_t *ship, az_vector_t point) {
  return az_convex_polygon_contains(ship_polygon, az_vrelative(
      point, ship->position, ship->angle));
}

/*===========================================================================*/
