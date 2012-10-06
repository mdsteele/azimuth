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

#include "azimuth/util/polygon.h"

#include <stdbool.h>

#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_polygon_contains(const az_polygon_t polygon, az_vector_t point) {
  const az_vector_t *vertices = polygon.vertices;
  // We're going to do a simple ray-casting test, where we imagine casting a
  // ray from the point in the +X direction; if we intersect an even number of
  // edges, then we must be outside the polygon.  So we start by assuming that
  // we're outside (inside = false), and invert for each edge we intersect.
  bool inside = false;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    // Common case: if the edge is completely above or below the ray, then
    // skip it.
    if ((vertices[i].y > point.y && vertices[j].y > point.y) ||
        (vertices[i].y <= point.y && vertices[j].y <= point.y)) {
      continue;
    }
    // Okay, the edge straddles the X-axis passing through the point.  But if
    // both vertices are to the left of the point, then we can skip this edge.
    if (vertices[i].x < point.x && vertices[j].x < point.x) {
      continue;
    }
    // Conversely, if both vertices are to the right of the point, then we
    // definitely hit the edge.
    if (vertices[i].x > point.x && vertices[j].x > point.x) {
      inside = !inside;
      continue;
    }
    // Otherwise, we can't easily be sure.  Compute the intersection of the
    // edge with the X-axis passing through the point.  If the X-coordinate of
    // the intersection is to the right of the point, then this is an
    // edge-crossing.
    if (point.x <= vertices[i].x + (point.y - vertices[i].y) *
        (vertices[j].x - vertices[i].x) / (vertices[j].y - vertices[i].y)) {
      inside = !inside;
      continue;
    }
  }
  return inside;
}

bool az_convex_polygon_contains(const az_polygon_t polygon,
                                az_vector_t point) {
  const az_vector_t *vertices = polygon.vertices;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    // Use a cross-product to determine if the point is outside the given edge,
    // using the assumption that vertices come in counter-clockwise order.
    // Since this is a convex polygon, the point is outside the polygon iff it
    // is outside at least once edge.
    if ((point.x - vertices[i].x) * (vertices[j].y - vertices[i].y) -
        (point.y - vertices[i].y) * (vertices[j].x - vertices[i].x) >= 0.0) {
      return false;
    }
  }
  return true;
}

/*===========================================================================*/
