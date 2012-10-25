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

#include "azimuth/state/door.h"

#include <stdbool.h>

#include "azimuth/util/misc.h" // for AZ_ARRAY_SIZE
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

const double AZ_DOOR_BOUNDING_RADIUS = 58.31;

static const az_vector_t closed_door_vertices[] = {
  {30, 50}, {-30, 50}, {-30, -50}, {30, -50}, {40, -35}, {40, 35}
};

static const az_polygon_t closed_door_polygon = {
  .num_vertices = AZ_ARRAY_SIZE(closed_door_vertices),
  .vertices = closed_door_vertices
};

bool az_ray_hits_door(const az_door_t *door, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out) {
  if (!az_ray_hits_circle(start, delta, door->position,
                          AZ_DOOR_BOUNDING_RADIUS)) return false;
  if (door->is_open) return false; // TODO what about sides of door?
  return az_ray_hits_polygon_trans(closed_door_polygon, door->position,
                                   door->angle, start, delta, point_out,
                                   normal_out);
}

/*===========================================================================*/
