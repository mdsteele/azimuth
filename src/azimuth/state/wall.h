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
#ifndef AZIMUTH_STATE_WALL_H_
#define AZIMUTH_STATE_WALL_H_

#include <stdbool.h>

#include "azimuth/util/color.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_WALL_NOTHING = 0,
  AZ_WALL_NORMAL
} az_wall_kind_t;

typedef struct {
  double elasticity; // the coefficiant of restitution when ship hits the wall
  double impact_damage_coeff;
  double bounding_radius;
  az_color_t color;
  az_polygon_t polygon;
} az_wall_data_t;

typedef struct {
  az_wall_kind_t kind; // if AZ_WALL_NOTHING, this wall is not present
  const az_wall_data_t *data;
  az_vector_t position;
  double angle;
} az_wall_t;

// The number of different wall data structs available.
extern const int AZ_NUM_WALL_DATAS;

// Call this at program startup to initialize all wall data.  In particular,
// this must be called before any calls to az_get_wall_data.
void az_init_wall_datas(void);

// Get the static wall data struct for a particular index.  The index must be
// between 0 (inclusive) and AZ_NUM_WALL_DATAS (exclusive).
const az_wall_data_t *az_get_wall_data(int index);

// Return the index for a wall data struct.  The pointer must be one that was
// returned from az_get_wall_data.
int az_wall_data_index(const az_wall_data_t *data);

// Return true if the given point intersects the wall.
bool az_point_hits_wall(const az_wall_t *wall, az_vector_t point);

// Determine if a ray, travelling delta from start, will hit the wall.  If it
// does, stores the intersection point in *point_out (if point_out is non-NULL)
// and the normal vector in *normal_out (if normal_out is non-NULL).
bool az_ray_hits_wall(const az_wall_t *wall, az_vector_t start,
                      az_vector_t delta, az_vector_t *point_out,
                      az_vector_t *normal_out);

/*===========================================================================*/

#endif // AZIMUTH_STATE_WALL_H_
