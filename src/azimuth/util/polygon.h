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

#include "azimuth/util/misc.h" // for AZ_ARRAY_SIZE
#include "azimuth/util/vector.h"

/*===========================================================================*/

#define AZ_INIT_POLYGON(array) \
  { .num_vertices = AZ_ARRAY_SIZE(array), .vertices = (array) }

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
bool az_polygon_contains(az_polygon_t polygon, az_vector_t point);

// Test if the point is in the polygon.  The polygon must be convex, the
// vertices must come in counter-clockwise order.  This function is more
// efficient, but less general, than az_polygon_contains().
bool az_convex_polygon_contains(az_polygon_t polygon, az_vector_t point);

// Determine if a ray, travelling delta from start, will ever pass within the
// specified polygon.  If it does, stores in *point_out the first point on the
// polygon that the ray hits (if point_out is non-NULL) and in *normal_out a
// vector normal to the polygon at the point of collision (if normal_out is
// non-NULL).  No guarantees are made about the length of the normal vector,
// and in particular it may be zero.
bool az_ray_hits_polygon(az_polygon_t polygon, az_vector_t start,
                         az_vector_t delta, az_vector_t *point_out,
                         az_vector_t *normal_out);

// Like az_ray_hits_polygon, but translates and rotates the polygon as
// specified before calculating the intersection.
bool az_ray_hits_polygon_trans(az_polygon_t polygon,
                               az_vector_t polygon_position,
                               double polygon_angle, az_vector_t start,
                               az_vector_t delta, az_vector_t *point_out,
                               az_vector_t *normal_out);

// Determine if two polygons, one stationary and one moving, collide as the
// moving polygon travels along the m_delta vector.  If a collision does occur,
// stores the position the moving polygon is at when it collides in *pos_out
// (if pos_out is non-NULL), the point of contact between the two polygons in
// *impact_out (if impact_out is non-NULL), and a vector normal to the
// stationary polygon at the point of impact in *normal_out (if normal_out is
// non-NULL).
bool az_polygons_collide(az_polygon_t s_polygon, az_vector_t s_position,
                         double s_angle, az_polygon_t m_polygon,
                         az_vector_t m_position, double m_angle,
                         az_vector_t m_delta, az_vector_t *pos_out,
                         az_vector_t *impact_out, az_vector_t *normal_out);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_POLYGON_H_
