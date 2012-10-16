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
#ifndef AZIMUTH_UTIL_POLYGON_H_
#define AZIMUTH_UTIL_POLYGON_H_

#include <stdbool.h>

#include "azimuth/util/vector.h"

/*===========================================================================*/

// Represents a closed 2D polygon.  It is usually expected that the polygon is
// non-self-intersecting, and that the vertices come in counter-clockwise
// order.  The `vertices` field is generally not considered to own the array
// that it points to.
typedef struct {
  int num_vertices;
  const az_vector_t *vertices;
} az_polygon_t;

// Test if the point is in the polygon.  The polygon must be
// non-self-intersecting, but it need not be convex.
bool az_polygon_contains(const az_polygon_t polygon, az_vector_t point);

// Test if the point is in the polygon.  The polygon must be convex, the
// vertices must come in counter-clockwise order.  This function is more
// efficient, but less general, than az_polygon_contains().
bool az_convex_polygon_contains(const az_polygon_t polygon, az_vector_t point);

// Determine if a ray, travelling delta from start, will ever pass within the
// specified polygon.  If it does, stores in *point_out the first point on the
// polygon that the ray hits (if point_out is non-NULL) and in *normal_out a
// vector normal to the polygon at the point of collision (if normal_out is
// non-NULL).  No guarantees are made about the length of the normal vector,
// and in particular it may be zero.
bool az_ray_hits_polygon(const az_polygon_t polygon, az_vector_t start,
                         az_vector_t delta, az_vector_t *point_out,
                         az_vector_t *normal_out);

// Like az_ray_hits_polygon, but translates and rotates the polygon as
// specified before calculating the intersection.
bool az_ray_hits_polygon_trans(const az_polygon_t polygon,
                               az_vector_t polygon_position,
                               double polygon_angle, az_vector_t start,
                               az_vector_t delta, az_vector_t *point_out,
                               az_vector_t *normal_out);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_POLYGON_H_
