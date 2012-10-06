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

#include "test/polygon.h"

#include "azimuth/util/misc.h" // for AZ_ARRAY_SIZE
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

#define MAKE_POLYGON(v) \
  ((az_polygon_t){.num_vertices = AZ_ARRAY_SIZE(v), .vertices = v})

static az_vector_t triangle_vertices[3] = {{-3, -3}, {2, 0}, {1, 4}};

static az_vector_t concave_hexagon_vertices[6] = {
  {2, -3}, {2, 2}, {-2, 3}, {-2, -3}, {0, -1}, {1, -1}
};

static az_vector_t square_vertices[5] =
  {{1, 1}, {0, 1}, {-1, 1}, {-1, -1}, {1, -1}};

void test_polygon_contains(void) {
  az_polygon_t triangle = MAKE_POLYGON(triangle_vertices);
  // Simple case: a convex polygon.
  EXPECT_TRUE(az_polygon_contains(triangle, (az_vector_t){0, 1}));
  EXPECT_TRUE(az_polygon_contains(triangle, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(triangle, (az_vector_t){1.5, 0.5}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){-2, 3}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){5, 1}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){-5, 10}));

  // Trickier: a concave polygon.
  az_polygon_t hexagon = MAKE_POLYGON(concave_hexagon_vertices);
  EXPECT_TRUE(az_polygon_contains(hexagon, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1, 2.5}));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1.5, -2}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){0, -2}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){-3, -2}));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1, -1}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){-5, -1}));

  // Let do some tests with colinear edges:
  az_polygon_t square = MAKE_POLYGON(square_vertices);
  EXPECT_TRUE(az_polygon_contains(square, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(square, (az_vector_t){0.5, 0.5}));
  EXPECT_FALSE(az_polygon_contains(square, (az_vector_t){-5, 1}));
  EXPECT_FALSE(az_polygon_contains(square, (az_vector_t){-5, -1}));
}

void test_convex_polygon_contains(void) {
  az_polygon_t triangle = MAKE_POLYGON(triangle_vertices);
  EXPECT_TRUE(az_convex_polygon_contains(triangle, (az_vector_t){0, 1}));
  EXPECT_TRUE(az_convex_polygon_contains(triangle, AZ_VZERO));
  EXPECT_TRUE(az_convex_polygon_contains(triangle, (az_vector_t){1.5, 0.5}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){-2, 3}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){5, 1}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){-5, 10}));

  // Let do some tests with colinear edges:
  az_polygon_t square = MAKE_POLYGON(square_vertices);
  EXPECT_TRUE(az_convex_polygon_contains(square, AZ_VZERO));
  EXPECT_TRUE(az_convex_polygon_contains(square, (az_vector_t){0.5, 0.5}));
  EXPECT_FALSE(az_convex_polygon_contains(square, (az_vector_t){-5, 1}));
  EXPECT_FALSE(az_convex_polygon_contains(square, (az_vector_t){-5, -1}));
}

/*===========================================================================*/
