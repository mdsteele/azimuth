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

#include "azimuth/state/wall.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h> // for NULL

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define INIT_POLYGON(v) { .num_vertices=AZ_ARRAY_SIZE(v), .vertices=(v) }

static az_vector_t wall_vertices_0[] = {
  {50, 50}, {-50, 50}, {-50, -50}, {50, -50}
};

static const az_wall_data_t wall_datas[] = {
  [0] = {
    .bounding_radius = 75.0,
    .color = {255, 255, 0, 255},
    .elasticity = 0.4,
    .polygon = INIT_POLYGON(wall_vertices_0)
  }
};

const az_wall_data_t *az_get_wall_data(int index) {
  if (index < 0 || index >= AZ_ARRAY_SIZE(wall_datas)) return NULL;
  return &wall_datas[index];
}

int az_wall_data_index(const az_wall_data_t *data) {
  if (data < wall_datas || data >= wall_datas + AZ_ARRAY_SIZE(wall_datas)) {
    return -1;
  }
  return data - wall_datas;
}

bool az_point_hits_wall(const az_wall_t *wall, az_vector_t point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return az_vwithin(point, wall->position, wall->data->bounding_radius) &&
    az_polygon_contains(wall->data->polygon, az_vrelative(
      point, wall->position, wall->angle));
}

bool az_ray_hits_wall(const az_wall_t *wall, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_circle(start, delta, wall->position,
                             wall->data->bounding_radius) &&
          az_ray_hits_polygon_trans(wall->data->polygon, wall->position,
                                    wall->angle, start, delta,
                                    point_out, normal_out));
}

/*===========================================================================*/
