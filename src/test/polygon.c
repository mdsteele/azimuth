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

#include <math.h>
#include <stddef.h> // for NULL

#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

#define NULL_POLYGON ((az_polygon_t){ .num_vertices = 0 })

static const az_vector_t triangle_vertices[3] = {{-3, -3}, {2, 0}, {1, 4}};

static const az_vector_t concave_hexagon_vertices[6] = {
  {2, -3}, {2, 2}, {-2, 3}, {-2, -3}, {0, -1}, {1, -1}
};

static const az_vector_t square_vertices[5] =
  {{1, 1}, {0, 1}, {-1, 1}, {-1, -1}, {1, -1}};

static const az_vector_t nix = {99999, 99999};

/*===========================================================================*/

void test_polygon_contains(void) {
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);
  // Simple case: a convex polygon.
  EXPECT_TRUE(az_polygon_contains(triangle, (az_vector_t){0, 1}));
  EXPECT_TRUE(az_polygon_contains(triangle, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(triangle, (az_vector_t){1.5, 0.5}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){-2, 3}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){5, 1}));
  EXPECT_FALSE(az_polygon_contains(triangle, (az_vector_t){-5, 10}));

  // Trickier: a concave polygon.
  const az_polygon_t hexagon = AZ_INIT_POLYGON(concave_hexagon_vertices);
  EXPECT_TRUE(az_polygon_contains(hexagon, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1, 2.5}));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1.5, -2}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){0, -2}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){-3, -2}));
  EXPECT_TRUE(az_polygon_contains(hexagon, (az_vector_t){-1, -1}));
  EXPECT_FALSE(az_polygon_contains(hexagon, (az_vector_t){-5, -1}));

  // Let do some tests with colinear edges:
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);
  EXPECT_TRUE(az_polygon_contains(square, AZ_VZERO));
  EXPECT_TRUE(az_polygon_contains(square, (az_vector_t){0.5, 0.5}));
  EXPECT_FALSE(az_polygon_contains(square, (az_vector_t){-5, 1}));
  EXPECT_FALSE(az_polygon_contains(square, (az_vector_t){-5, -1}));

  // The null polygon shouldn't contain any point.
  EXPECT_FALSE(az_polygon_contains(NULL_POLYGON, AZ_VZERO));
}

void test_convex_polygon_contains(void) {
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);
  EXPECT_TRUE(az_convex_polygon_contains(triangle, (az_vector_t){0, 1}));
  EXPECT_TRUE(az_convex_polygon_contains(triangle, AZ_VZERO));
  EXPECT_TRUE(az_convex_polygon_contains(triangle, (az_vector_t){1.5, 0.5}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){-2, 3}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){5, 1}));
  EXPECT_FALSE(az_convex_polygon_contains(triangle, (az_vector_t){-5, 10}));

  // Let do some tests with colinear edges:
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);
  EXPECT_TRUE(az_convex_polygon_contains(square, AZ_VZERO));
  EXPECT_TRUE(az_convex_polygon_contains(square, (az_vector_t){0.5, 0.5}));
  EXPECT_FALSE(az_convex_polygon_contains(square, (az_vector_t){-5, 1}));
  EXPECT_FALSE(az_convex_polygon_contains(square, (az_vector_t){-5, -1}));

  // The null polygon shouldn't contain any point.
  EXPECT_FALSE(az_convex_polygon_contains(NULL_POLYGON, AZ_VZERO));
}

/*===========================================================================*/

void test_circle_touches_line(void) {
  EXPECT_TRUE(az_circle_touches_line(
      (az_vector_t){2, 4}, (az_vector_t){3, 3}, 1.5, (az_vector_t){1, 7}));
  EXPECT_FALSE(az_circle_touches_line(
      (az_vector_t){2, 4}, (az_vector_t){3, 3}, 1.4, (az_vector_t){3, 5}));
}

void test_circle_touches_line_segment(void) {
  EXPECT_TRUE(az_circle_touches_line_segment(
      (az_vector_t){1, 4}, (az_vector_t){3, 4}, 0.2, (az_vector_t){2, 4.1}));
  EXPECT_TRUE(az_circle_touches_line_segment(
      (az_vector_t){1, 4}, (az_vector_t){3, 4}, 0.3, (az_vector_t){3.1, 4.1}));
  EXPECT_FALSE(az_circle_touches_line_segment(
      (az_vector_t){1, 4}, (az_vector_t){3, 4}, 0.2, (az_vector_t){2, 4.3}));
  EXPECT_FALSE(az_circle_touches_line_segment(
      (az_vector_t){1, 4}, (az_vector_t){3, 4}, 0.2, (az_vector_t){4, 4.1}));
}

void test_circle_touches_polygon(void) {
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);
  // Case where circle touches vertex:
  EXPECT_TRUE(az_circle_touches_polygon(
      triangle, 0.15, (az_vector_t){2.1, 0}));
  // Case where circle touches edge:
  EXPECT_TRUE(az_circle_touches_polygon(
      triangle, 0.15, (az_vector_t){1.6, 2}));
  // Case where circle is completely inside polygon:
  EXPECT_TRUE(az_circle_touches_polygon(
      triangle, 0.1, (az_vector_t){0, 0}));
  // Case where circle is doesn't touch polygon:
  EXPECT_FALSE(az_circle_touches_polygon(
      triangle, 1.0, (az_vector_t){3, -4}));
}

void test_circle_touches_polygon_trans(void) {
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);
  EXPECT_TRUE(az_circle_touches_polygon_trans(
      square, (az_vector_t){-10, 1}, AZ_DEG2RAD(45),
      0.1, (az_vector_t){-10, 2.5}));
  EXPECT_FALSE(az_circle_touches_polygon_trans(
      square, (az_vector_t){-10, 1}, AZ_DEG2RAD(45),
      0.1, (az_vector_t){-11, 2}));
}

/*===========================================================================*/

void test_ray_hits_bounding_circle(void) {
  // Ray passes from ouside of circle to inside:
  EXPECT_TRUE(az_ray_hits_bounding_circle(
      (az_vector_t){2, 2}, (az_vector_t){-2, -2}, (az_vector_t){0, 0}, 2));
  // Simple ray misses circle:
  EXPECT_FALSE(az_ray_hits_bounding_circle(
      (az_vector_t){2, 2}, (az_vector_t){-1, -1}, (az_vector_t){0, 4}, 2));
  // Ray completely inside circle:
  EXPECT_TRUE(az_ray_hits_bounding_circle(
      (az_vector_t){-4, 4}, (az_vector_t){2, 0}, (az_vector_t){-3, 4}, 2));
  // Ray passes from one side of circle to the other:
  EXPECT_TRUE(az_ray_hits_bounding_circle(
      (az_vector_t){-8, 4}, (az_vector_t){16, 1}, (az_vector_t){-3, 4}, 2));
  // Ray barely misses circle:
  EXPECT_FALSE(az_ray_hits_bounding_circle(
      (az_vector_t){-.5, .5}, (az_vector_t){1, -1}, (az_vector_t){-1, -1}, 1));
  // Ray pointed away from circle:
  EXPECT_FALSE(az_ray_hits_bounding_circle(
      (az_vector_t){-1, 1}, (az_vector_t){0, 10}, (az_vector_t){-1, -1}, 1));
  // Ray pointed towards circle, but stops just short:
  EXPECT_FALSE(az_ray_hits_bounding_circle(
      (az_vector_t){-1, 2}, (az_vector_t){0, -1}, (az_vector_t){-1, -1}, 1));
}

void test_ray_hits_circle(void) {
  az_vector_t intersect = nix, normal = nix;

  // Check az_ray_hits_circle works with NULLs for point_out and normal_out:
  EXPECT_TRUE(az_ray_hits_circle(
      2.0, (az_vector_t){0, 0}, (az_vector_t){3, 0}, (az_vector_t){-2, 0},
      NULL, NULL));

  // Ray passes from ouside of circle to inside:
  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_circle(
      2.0, (az_vector_t){0, 0}, (az_vector_t){3, 0}, (az_vector_t){-2, 0},
      &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){2, 0}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 0}), az_vunit(normal));
}

void test_ray_hits_line_segment(void) {
  az_vector_t intersect = nix, normal = nix;

  // Check az_ray_hits_line_segment works with NULLs:
  EXPECT_TRUE(az_ray_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, (az_vector_t){6, -5},
      (az_vector_t){0, 10}, NULL, NULL));

  // Check case where ray hits line segment dead-on.
  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, (az_vector_t){6, -5},
      (az_vector_t){0, 10}, &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){6, 1}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){0, -1}), az_vunit(normal));

  // Check case where ray would hit infinite line, but misses line segment.
  intersect = normal = nix;
  EXPECT_FALSE(az_ray_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, (az_vector_t){16, -5},
      (az_vector_t){0, 10}, &intersect, &normal));
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Check case where ray would hit line segment, but stops short.
  intersect = normal = nix;
  EXPECT_FALSE(az_ray_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, (az_vector_t){6, -5},
      (az_vector_t){0, 5}, &intersect, &normal));
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);
}

void test_ray_hits_polygon(void) {
  az_vector_t intersect = nix, normal = nix;
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);

  // Check az_ray_hits_polygon works with NULLs for point_out and normal_out:
  EXPECT_TRUE(az_ray_hits_polygon(
      triangle, (az_vector_t){2, 4}, (az_vector_t){-1, -4}, NULL, NULL));

  // Check case where ray hits an edge:
  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_polygon(
      triangle, (az_vector_t){2, 4}, (az_vector_t){-1, -4},
      &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){1.5, 2}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){4, 1}), az_vunit(normal));

  // Check case where ray hits several edges:
  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_polygon(
      triangle, (az_vector_t){2, 4}, (az_vector_t){-5, -20},
      &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){1.5, 2}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){4, 1}), az_vunit(normal));

  // Check case where ray is entirely inside polygon (should hit at the start
  // point, with the normal being -delta):
  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_polygon(
      triangle, (az_vector_t){0.5, 0}, (az_vector_t){1, 0},
      &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){0.5, 0}), intersect);
  EXPECT_VAPPROX(((az_vector_t){-1, 0}), az_vunit(normal));

  // Check case where ray misses (by stopping short of polygon):
  intersect = normal = nix;
  EXPECT_FALSE(az_ray_hits_polygon(
      triangle, (az_vector_t){-5, 0}, (az_vector_t){1, 0},
      &intersect, &normal));
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Check case where ray misses (by missing completely):
  intersect = normal = nix;
  EXPECT_FALSE(az_ray_hits_polygon(
      triangle, (az_vector_t){5, 4}, (az_vector_t){1, -5},
      &intersect, &normal));
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // We should never hit the null polygon.
  intersect = normal = nix;
  EXPECT_FALSE(az_ray_hits_polygon(
      NULL_POLYGON, (az_vector_t){-1, -1}, (az_vector_t){1, 1},
      &intersect, &normal));
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);
}

void test_ray_hits_polygon_trans(void) {
  az_vector_t intersect = nix, normal = nix;
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);

  intersect = normal = nix;
  EXPECT_TRUE(az_ray_hits_polygon_trans(
      triangle, (az_vector_t){3, 0.059714999709336247}, 1.3258176636680323,
      (az_vector_t){3, 5}, (az_vector_t){0, -5}, &intersect, &normal));
  EXPECT_VAPPROX(((az_vector_t){3, 2}), intersect);
  EXPECT_VAPPROX(((az_vector_t){0, 1}), az_vunit(normal));
}

/*===========================================================================*/

void test_circle_hits_point(void) {
  az_vector_t pos = nix, impact = nix;

  // Check az_circle_hits_point works with NULLs for pos_out and impact_out:
  EXPECT_TRUE(az_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1}, (az_vector_t){10, 0},
      NULL, NULL));

  // Check case where circle hits point dead-on.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1}, (az_vector_t){10, 0},
      &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-1, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);

  // Check case where point starts inside circle.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_point(
      (az_vector_t){-5, 7}, 2.0, (az_vector_t){-6, 6}, (az_vector_t){9, -15},
      &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-6, 6}), pos);
  EXPECT_VAPPROX(((az_vector_t){-5, 7}), impact);

  // Check case where circle misses point.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1}, (az_vector_t){10, 10},
      &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // Check case where circle stops short of point.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1}, (az_vector_t){3, 0},
      &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
}

void test_circle_hits_circle(void) {
  az_vector_t pos = nix, impact = nix;

  // Check az_circle_hits_point works with NULLs for pos_out and impact_out:
  EXPECT_TRUE(az_circle_hits_circle(
      1.0, (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1},
      (az_vector_t){10, 0}, NULL, NULL));

  // Check simple case:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_circle(
      1.0, (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1},
      (az_vector_t){10, 0}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-2, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){0, 1}), impact);

  // Check case where radius of stationary circle is zero:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_circle(
      0.0, (az_vector_t){1, 1}, 2.0, (az_vector_t){-6, 1},
      (az_vector_t){10, 0}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-1, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);

  // Check case where radius of moving circle is zero:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_circle(
      1.0, (az_vector_t){1, 1}, 0.0, (az_vector_t){-6, 1},
      (az_vector_t){10, 0}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){0, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){0, 1}), impact);
}

void test_circle_hits_line(void) {
  az_vector_t pos = nix, impact = nix;

  // Check az_circle_hits_line works with NULLs for pos_out and impact_out:
  EXPECT_TRUE(az_circle_hits_line(
      (az_vector_t){1, 1}, (az_vector_t){2, 1}, 2.0, (az_vector_t){15, -5},
      (az_vector_t){0, 10}, NULL, NULL));

  // Check case where circle hits line dead-on.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_line(
      (az_vector_t){1, 1}, (az_vector_t){2, 1}, 2.0, (az_vector_t){15, -5},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){15, -1}), pos);
  EXPECT_VAPPROX(((az_vector_t){15, 1}), impact);

  // Check case where circle is already intersecting line.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_line(
      (az_vector_t){1, 1}, (az_vector_t){2, 1}, 2.0, (az_vector_t){12, 2},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){12, 2}), pos);
  EXPECT_VAPPROX(((az_vector_t){12, 1}), impact);

  // Check case where circle goes wrong direction.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_line(
      (az_vector_t){1, 1}, (az_vector_t){2, 1}, 2.0, (az_vector_t){15, -5},
      (az_vector_t){0, -10}, &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // Check case where circle stops short of line.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_line(
      (az_vector_t){1, 1}, (az_vector_t){2, 1}, 2.0, (az_vector_t){15, -5},
      (az_vector_t){0, 3}, &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
}

void test_circle_hits_line_segment(void) {
  az_vector_t pos = nix, impact = nix;

  // Check az_circle_hits_line_segment works with NULLs:
  EXPECT_TRUE(az_circle_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, 2.0, (az_vector_t){6, -5},
      (az_vector_t){0, 10}, NULL, NULL));

  // Check case where circle hits line segment dead-on.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, 2.0, (az_vector_t){6, -5},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){6, -1}), pos);
  EXPECT_VAPPROX(((az_vector_t){6, 1}), impact);

  // Check case where circle grazes endpoint of line segment.
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_line_segment(
      (az_vector_t){-3, 2}, (az_vector_t){4, 2},
      sqrt(2), (az_vector_t){5, -4},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){5, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){4, 2}), impact);

  // Check case where circle would hit infinite line, but misses line segment.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, 2.0, (az_vector_t){16, -5},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // Check case where circle would hit infinite line, but misses line segment
  // on the other side.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_line_segment(
      (az_vector_t){5.5, 1}, (az_vector_t){6.5, 1}, 2.0, (az_vector_t){0, -5},
      (az_vector_t){0, 10}, &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
}

void test_circle_hits_polygon(void) {
  az_vector_t pos = nix, impact = nix;
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_circle_hits_polygon works with NULLs for pos_out and impact_out:
  EXPECT_TRUE(az_circle_hits_polygon(
      triangle, 2.0, (az_vector_t){3.940285000290664, 5.4850712500726659},
      (az_vector_t){-4, -10}, NULL, NULL));

  // Check case where circle hits an edge of the triangle:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_polygon(
      triangle, 2.0, (az_vector_t){5.4402850002906638, 7.4850712500726662},
      (az_vector_t){-4, -10}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){3.4402850002906638, 2.4850712500726662}), pos);
  EXPECT_VAPPROX(((az_vector_t){1.5, 2}), impact);

  // Check case where circle hits a corner of the triangle:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_polygon(
      triangle, 2.0, (az_vector_t){4, 7}, (az_vector_t){-10, -10},
      &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){2.4142135623730949, 5.4142135623730949}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 4}), impact);

  // Check case where circle hits an edge of the square:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_polygon(
      square, 0.3, (az_vector_t){-5, 0}, (az_vector_t){20, 0}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-1.3, 0}), pos);
  EXPECT_VAPPROX(((az_vector_t){-1, 0}), impact);

  // Check case where circle hits a corner of the square:
  pos = impact = nix;
  EXPECT_TRUE(az_circle_hits_polygon(
      square, sqrt(2), (az_vector_t){-5, 2}, (az_vector_t){20, 0},
      &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){-2, 2}), pos);
  EXPECT_VAPPROX(((az_vector_t){-1, 1}), impact);

  // Check case where circle misses the square:
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_polygon(
      square, 0.2, (az_vector_t){-5, 1.21}, (az_vector_t){20, 0},
      &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // We should never hit the null polygon.
  pos = impact = nix;
  EXPECT_FALSE(az_circle_hits_polygon(
      NULL_POLYGON, 0.5, (az_vector_t){-1, -1}, (az_vector_t){1, 1},
      &pos, &impact));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
}

void test_circle_hits_polygon_trans(void) {
  az_vector_t pos = nix, impact = nix;
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);

  EXPECT_TRUE(az_circle_hits_polygon_trans(
      triangle, (az_vector_t){3, 0.059714999709336247}, 1.3258176636680323,
      2.0, (az_vector_t){3, 5}, (az_vector_t){0, -5}, &pos, &impact));
  EXPECT_VAPPROX(((az_vector_t){3, 4}), pos);
  EXPECT_VAPPROX(((az_vector_t){3, 2}), impact);
}

/*===========================================================================*/

void test_arc_ray_hits_circle(void) {
  double angle = 99999;
  az_vector_t intersect = nix, normal = nix;

  // Check az_arc_ray_hits_circle works with NULLs:
  EXPECT_TRUE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){2, 4}, (az_vector_t){2, 1},
      AZ_DEG2RAD(-135), NULL, NULL, NULL));

  // Simple case, hitting (with negative spin_angle):
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){2, 4}, (az_vector_t){2, 1},
      AZ_DEG2RAD(-135), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(-90), angle);
  EXPECT_VAPPROX(((az_vector_t){5, 1}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){0, 1}), az_vunit(normal));

  // Simple case, stopping short (with negative spin_angle):
  angle = 99999; intersect = normal = nix;
  EXPECT_FALSE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){2, 4}, (az_vector_t){2, 1},
      AZ_DEG2RAD(-89), &angle, &intersect, &normal));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Simple case, hitting (with positive spin_angle):
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){2, 0}, (az_vector_t){2, -3},
      AZ_DEG2RAD(315), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(270), angle);
  EXPECT_VAPPROX(((az_vector_t){5, -3}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){0, -1}), az_vunit(normal));

  // Simple case, stopping short (with positive spin_angle):
  angle = 99999; intersect = normal = nix;
  EXPECT_FALSE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){2, 0}, (az_vector_t){2, -3},
      AZ_DEG2RAD(269), &angle, &intersect, &normal));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Ray goes completely around circle:
  angle = 99999; intersect = normal = nix;
  EXPECT_FALSE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){9, 5}, (az_vector_t){4, 0},
      AZ_DEG2RAD(400), &angle, &intersect, &normal));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Ray starts inside circle:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_circle(
      2.0, (az_vector_t){5, -1}, (az_vector_t){4, 0}, (az_vector_t){1, 1},
      AZ_DEG2RAD(400), &angle, &intersect, &normal));
  EXPECT_APPROX(0, angle);
  EXPECT_VAPPROX(((az_vector_t){4, 0}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){-1, 1}), az_vunit(normal));

  // Ray hits circle diagonally:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_circle(
      sqrt(2), (az_vector_t){1, 1}, (az_vector_t){4, 2}, (az_vector_t){3, 1},
      AZ_DEG2RAD(400), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){2, 2}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 1}), az_vunit(normal));
}

void test_arc_ray_hits_line(void) {
  double angle = 99999;
  az_vector_t intersect = nix, normal = nix;

  // Check az_arc_ray_hits_line works with NULLs:
  EXPECT_TRUE(az_arc_ray_hits_line(
      (az_vector_t){1, 3}, (az_vector_t){1, 4}, (az_vector_t){5, 1},
      (az_vector_t){1, 1}, AZ_DEG2RAD(170), NULL, NULL, NULL));

  // Ray hits line:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_line(
      (az_vector_t){1, 3}, (az_vector_t){1, 4}, (az_vector_t){5, 1},
      (az_vector_t){1, 1}, AZ_DEG2RAD(170), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, 5}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 0}), az_vunit(normal));

  // Ray hits line:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_line(
      (az_vector_t){1, 3}, (az_vector_t){1, 4}, (az_vector_t){3, 1},
      (az_vector_t){2, 0}, AZ_DEG2RAD(170), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 0}), az_vunit(normal));
}

void test_arc_ray_hits_line_segment(void) {
  double angle = 99999;
  az_vector_t intersect = nix, normal = nix;

  // Check az_arc_ray_hits_line_segment works with NULLs:
  EXPECT_TRUE(az_arc_ray_hits_line_segment(
      (az_vector_t){1, 6}, (az_vector_t){1, 4}, (az_vector_t){5, 1},
      (az_vector_t){1, 1}, AZ_DEG2RAD(170), NULL, NULL, NULL));

  // Ray hits line segment:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_line_segment(
      (az_vector_t){1, 6}, (az_vector_t){1, 4}, (az_vector_t){5, 1},
      (az_vector_t){1, 1}, AZ_DEG2RAD(170), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, 5}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 0}), az_vunit(normal));

  // Ray would hit infinite line, but misses line segment:
  angle = 99999; intersect = normal = nix;
  EXPECT_FALSE(az_arc_ray_hits_line_segment(
      (az_vector_t){1, 3}, (az_vector_t){1, 4}, (az_vector_t){3, 1},
      (az_vector_t){2, 0}, AZ_DEG2RAD(170), &angle, &intersect, &normal));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);
}

void test_arc_ray_hits_polygon(void) {
  double angle = 99999;
  az_vector_t intersect = nix, normal = nix;
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_arc_ray_hits_polygon works with NULLs:
  EXPECT_TRUE(az_arc_ray_hits_polygon(
      square, (az_vector_t){4, 4.5}, (az_vector_t){1, 3.5}, AZ_DEG2RAD(-170),
      NULL, NULL, NULL));

  // Ray hits polygon:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_polygon(
      square, (az_vector_t){4, 3.5}, (az_vector_t){1, 3.5}, AZ_DEG2RAD(-170),
      &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(-90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, 0.5}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 0}), az_vunit(normal));

  // Ray stops short of polygon:
  angle = 99999; intersect = normal = nix;
  EXPECT_FALSE(az_arc_ray_hits_polygon(
      square, (az_vector_t){4, 3.5}, (az_vector_t){1, 3.5}, AZ_DEG2RAD(-89),
      &angle, &intersect, &normal));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, intersect);
  EXPECT_VAPPROX(nix, normal);

  // Ray starts in polygon:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_polygon(
      square, (az_vector_t){0.5, -0.5}, (az_vector_t){1, 3.5},
      AZ_DEG2RAD(-170), &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(0), angle);
  EXPECT_VAPPROX(((az_vector_t){0.5, -0.5}), intersect);
  EXPECT_VAPPROX(AZ_VZERO, normal);
}

void test_arc_ray_hits_polygon_trans(void) {
  double angle = 99999;
  az_vector_t intersect = nix, normal = nix;
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_arc_ray_hits_polygon_trans works with NULLs:
  EXPECT_TRUE(az_arc_ray_hits_polygon_trans(
      square, (az_vector_t){-2, 1}, AZ_DEG2RAD(45),
      (az_vector_t){sqrt(2)/2, 3 + sqrt(2)/2},
      (az_vector_t){sqrt(2)/2, 1 + sqrt(2)/2}, AZ_DEG2RAD(100),
      NULL, NULL, NULL));

  // Ray hits polygon:
  angle = 99999; intersect = normal = nix;
  EXPECT_TRUE(az_arc_ray_hits_polygon_trans(
      square, (az_vector_t){-2, 1}, AZ_DEG2RAD(45),
      (az_vector_t){sqrt(2)/2, 3 + sqrt(2)/2},
      (az_vector_t){sqrt(2)/2, 1 + sqrt(2)/2}, AZ_DEG2RAD(100),
      &angle, &intersect, &normal));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){-2 + sqrt(2)/2, 1 + sqrt(2)/2}), intersect);
  EXPECT_VAPPROX(az_vunit((az_vector_t){1, 1}), az_vunit(normal));
}

/*===========================================================================*/

void test_arc_circle_hits_point(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;

  // Check az_arc_circle_hits_point works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){3, 5}, (az_vector_t){3, 3},
      AZ_DEG2RAD(-270), NULL, NULL, NULL));

  // Check case where circle hits point dead-on.
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_point(
      (az_vector_t){1, 1}, 2.0, (az_vector_t){3, 5}, (az_vector_t){3, 3},
      AZ_DEG2RAD(-270), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(-180), angle);
  EXPECT_VAPPROX(((az_vector_t){3, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);
}

void test_arc_circle_hits_circle(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;

  // Check az_arc_circle_hits_circle works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_circle(
      0.5, (az_vector_t){0.5, 1}, 2.0, (az_vector_t){3, 5},
      (az_vector_t){3, 3}, AZ_DEG2RAD(-270), NULL, NULL, NULL));

  // Check case where circle hits circle dead-on.
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_circle(
      0.5, (az_vector_t){0.5, 1}, 2.0, (az_vector_t){3, 5},
      (az_vector_t){3, 3}, AZ_DEG2RAD(-270), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(-180), angle);
  EXPECT_VAPPROX(((az_vector_t){3, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);
}

void test_arc_circle_hits_line(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;

  // Check az_arc_circle_hits_line works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_line(
      (az_vector_t){-2, 7}, (az_vector_t){-2, 9}, 3.0, (az_vector_t){3, -3},
      (az_vector_t){1, -3}, AZ_DEG2RAD(170), NULL, NULL, NULL));

  // Check case where circle hits line dead-on.
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_line(
      (az_vector_t){-2, 7}, (az_vector_t){-2, 9}, 3.0, (az_vector_t){3, -3},
      (az_vector_t){1, -3}, AZ_DEG2RAD(170), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, -1}), pos);
  EXPECT_VAPPROX(((az_vector_t){-2, -1}), impact);

  // Check case where circle hits line obliquely.
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_line(
      (az_vector_t){-2, 7}, (az_vector_t){-2, 9}, 3.0, (az_vector_t){11, -5},
      (az_vector_t){6, 0}, AZ_DEG2RAD(-170), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(-90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, -5}), pos);
  EXPECT_VAPPROX(((az_vector_t){-2, -5}), impact);

  // Check case where circle can't hit line.
  angle = 99999; pos = impact = nix;
  EXPECT_FALSE(az_arc_circle_hits_line(
      (az_vector_t){-2, 7}, (az_vector_t){-2, 9}, 3.0, (az_vector_t){15, -5},
      (az_vector_t){10, 0}, AZ_DEG2RAD(-400), &angle, &pos, &impact));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
}

void test_arc_circle_hits_line_segment(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;

  // Check az_arc_circle_hits_line_segment works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_line_segment(
      (az_vector_t){-2, 0}, (az_vector_t){-2, -2}, 3.0, (az_vector_t){3, -3},
      (az_vector_t){1, -3}, AZ_DEG2RAD(170), NULL, NULL, NULL));

  // Circle hits line segment in the middle:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_line_segment(
      (az_vector_t){-2, 0}, (az_vector_t){-2, -2}, 3.0, (az_vector_t){3, -3},
      (az_vector_t){1, -3}, AZ_DEG2RAD(170), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){1, -1}), pos);
  EXPECT_VAPPROX(((az_vector_t){-2, -1}), impact);

  // Circle would hit infinite line, but misses line segment:
  angle = 99999; pos = impact = nix;
  EXPECT_FALSE(az_arc_circle_hits_line_segment(
      (az_vector_t){-2, 70}, (az_vector_t){-2, 90}, 3.0, (az_vector_t){3, -3},
      (az_vector_t){1, -3}, AZ_DEG2RAD(170), &angle, &pos, &impact));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // Circle hits one end of the line segment:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_line_segment(
      (az_vector_t){1, 0}, (az_vector_t){2, 0}, sqrt(2), (az_vector_t){3, 3},
      (az_vector_t){2, 2}, AZ_DEG2RAD(-100), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(-90), angle);
  EXPECT_VAPPROX(((az_vector_t){3, 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){2, 0}), impact);
}

void test_arc_circle_hits_polygon(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_arc_circle_hits_polygon works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_polygon(
      square, 2.0, (az_vector_t){-5, -8}, (az_vector_t){-5, -3},
      AZ_DEG2RAD(100), NULL, NULL, NULL));

  // Circle hits polygon on side:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_polygon(
      square, 2.0, (az_vector_t){-5, -8}, (az_vector_t){-5, -3},
      AZ_DEG2RAD(100), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){0, -3}), pos);
  EXPECT_VAPPROX(((az_vector_t){0, -1}), impact);

  // Circle hits polygon on corner:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_polygon(
      square, sqrt(2), (az_vector_t){4, 2}, (az_vector_t){3, 1},
      AZ_DEG2RAD(100), &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){2, 2}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);

  // Circle stops short of polygon:
  angle = 99999; pos = impact = nix;
  EXPECT_FALSE(az_arc_circle_hits_polygon(
      square, sqrt(2), (az_vector_t){4, 2}, (az_vector_t){3, 1},
      AZ_DEG2RAD(89), &angle, &pos, &impact));
  EXPECT_APPROX(99999, angle);
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);

  // Circle starts overlapped with polygon:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_polygon(
      square, 1.5, (az_vector_t){2, 2}, (az_vector_t){3, 1},
      AZ_DEG2RAD(89), &angle, &pos, &impact));
  EXPECT_APPROX(0, angle);
  EXPECT_VAPPROX(((az_vector_t){2, 2}), pos);
  EXPECT_VAPPROX(((az_vector_t){1, 1}), impact);
}

void test_arc_circle_hits_polygon_trans(void) {
  double angle = 99999;
  az_vector_t pos = nix, impact = nix;
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_arc_circle_hits_polygon_trans works with NULLs:
  EXPECT_TRUE(az_arc_circle_hits_polygon_trans(
      square, (az_vector_t){-2, 1}, AZ_DEG2RAD(45),
      1.0, (az_vector_t){4 + sqrt(2), -4},
      (az_vector_t){-1 + sqrt(2), -4}, AZ_DEG2RAD(100),
      NULL, NULL, NULL));

  // Circle hits polygon:
  angle = 99999; pos = impact = nix;
  EXPECT_TRUE(az_arc_circle_hits_polygon_trans(
      square, (az_vector_t){-2, 1}, AZ_DEG2RAD(45),
      1.0, (az_vector_t){4 + sqrt(2), -4},
      (az_vector_t){-1 + sqrt(2), -4}, AZ_DEG2RAD(100),
      &angle, &pos, &impact));
  EXPECT_APPROX(AZ_DEG2RAD(90), angle);
  EXPECT_VAPPROX(((az_vector_t){-1 + sqrt(2), 1}), pos);
  EXPECT_VAPPROX(((az_vector_t){-2 + sqrt(2), 1}), impact);
}

/*===========================================================================*/

void test_polygons_collide(void) {
  az_vector_t pos = nix, impact = nix, normal = nix;
  const az_polygon_t triangle = AZ_INIT_POLYGON(triangle_vertices);
  const az_polygon_t square = AZ_INIT_POLYGON(square_vertices);

  // Check az_polygons_collide works with NULLs for out args:
  pos = impact = normal = nix;
  EXPECT_TRUE(az_polygons_collide(
      square, AZ_VZERO, 0, triangle, (az_vector_t){0, -5}, AZ_HALF_PI,
      (az_vector_t){0, 10}, NULL, NULL, NULL));

  // Check case where corner of moving polygon hits edge of stationary polygon:
  pos = impact = normal = nix;
  EXPECT_TRUE(az_polygons_collide(
      square, AZ_VZERO, 0, triangle, (az_vector_t){0, -5}, AZ_HALF_PI,
      (az_vector_t){0, 10}, &pos, &impact, &normal));
  EXPECT_VAPPROX(((az_vector_t){0, -3}), pos);
  EXPECT_VAPPROX(((az_vector_t){0, -1}), impact);
  EXPECT_VAPPROX(((az_vector_t){0, -1}), az_vunit(normal));

  // Check case where corner of stationary polygon hits edge of moving polygon:
  pos = impact = normal = nix;
  EXPECT_TRUE(az_polygons_collide(
      triangle, AZ_VZERO, 0, square, (az_vector_t){-5, -3}, AZ_PI,
      (az_vector_t){10, 0}, &pos, &impact, &normal));
  EXPECT_VAPPROX(((az_vector_t){-4, -3}), pos);
  EXPECT_VAPPROX(((az_vector_t){-3, -3}), impact);
  EXPECT_VAPPROX(((az_vector_t){-1, 0}), az_vunit(normal));

  // Check another case where corner of stationary polygon hits edge of moving
  // polygon:
  pos = impact = normal = nix;
  EXPECT_TRUE(az_polygons_collide(
      triangle, AZ_VZERO, 0, square, (az_vector_t){-6, -6}, AZ_PI/4,
      (az_vector_t){10, 10}, &pos, &impact, &normal));
  EXPECT_VAPPROX(((az_vector_t){-3.707106781186548, -3.707106781186548}), pos);
  EXPECT_VAPPROX(((az_vector_t){-3, -3}), impact);
  EXPECT_VAPPROX(az_vunit((az_vector_t){-1, -1}), az_vunit(normal));

  // Check case where polygons don't collide:
  pos = impact = normal = nix;
  EXPECT_FALSE(az_polygons_collide(
      square, AZ_VZERO, 0, triangle, (az_vector_t){5, -3}, 1.3258176636680323,
      (az_vector_t){-10, 0}, &pos, &impact, &normal));
  EXPECT_VAPPROX(nix, pos);
  EXPECT_VAPPROX(nix, impact);
  EXPECT_VAPPROX(nix, normal);

  // Check case where stationary polygon is not at origin:
  pos = impact = normal = nix;
  EXPECT_TRUE(az_polygons_collide(
      triangle, (az_vector_t){5, -2}, -AZ_HALF_PI,
      square, (az_vector_t){-1, 4}, AZ_PI / 4,
      (az_vector_t){10, -10}, &pos, &impact, &normal));
  EXPECT_VAPPROX(((az_vector_t){1.2928932188134525, 1.7071067811865475}), pos);
  EXPECT_VAPPROX(((az_vector_t){2, 1}), impact);
  EXPECT_VAPPROX(az_vunit((az_vector_t){-1, 1}), az_vunit(normal));
}

/*===========================================================================*/
