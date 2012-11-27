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
#include <math.h> // for fmax
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static az_vector_t wall_vertices_0[] = {
  {50, 25}, {-50, 25}, {-50, -25}, {50, -25}
};
static az_vector_t wall_vertices_1[] = {
  {50, 50}, {-50, 50}, {-50, -50}, {50, -50}
};
static az_vector_t wall_vertices_2[] = {
  {10, -25}, {60, -25}, {60, 25}, {-10.710678, 25},
  {-60.710678, -25}, {-25.355339, -60.355339}
};

static az_wall_data_t wall_datas[] = {
  [0] = {
    .color = {255, 255, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_0)
  },
  [1] = {
    .color = {0, 255, 255, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_1)
  },
  [2] = {
    .color = {255, 255, 0, 255},
    .elasticity = 0.4,
    .polygon = AZ_INIT_POLYGON(wall_vertices_2)
  }
};

/*===========================================================================*/

const int AZ_NUM_WALL_DATAS = AZ_ARRAY_SIZE(wall_datas);

static bool wall_data_initialized = false;

void az_init_wall_datas(void) {
  assert(!wall_data_initialized);
  AZ_ARRAY_LOOP(data, wall_datas) {
    double radius = 0.0;
    for (int i = 0; i < data->polygon.num_vertices; ++i) {
      radius = fmax(radius, az_vnorm(data->polygon.vertices[i]));
    }
    data->bounding_radius = radius + 0.01; // small safety margin
  }
  wall_data_initialized = true;
}

const az_wall_data_t *az_get_wall_data(int index) {
  assert(wall_data_initialized);
  assert(index >= 0);
  assert(index < AZ_NUM_WALL_DATAS);
  return &wall_datas[index];
}

int az_wall_data_index(const az_wall_data_t *data) {
  assert(data >= wall_datas);
  assert(data < wall_datas + AZ_NUM_WALL_DATAS);
  return data - wall_datas;
}

/*===========================================================================*/

bool az_point_hits_wall(const az_wall_t *wall, az_vector_t point) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return az_vwithin(point, wall->position, wall->data->bounding_radius) &&
    az_polygon_contains(wall->data->polygon,
        az_vrotate(az_vsub(point, wall->position), -wall->angle));
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

bool az_circle_hits_wall(
    const az_wall_t *wall, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(wall->kind != AZ_WALL_NOTHING);
  return (az_ray_hits_circle(start, delta, wall->position,
                             wall->data->bounding_radius + radius) &&
          az_circle_hits_polygon_trans(wall->data->polygon, wall->position,
                                       wall->angle, radius, start, delta,
                                       pos_out, impact_out));
}

/*===========================================================================*/
