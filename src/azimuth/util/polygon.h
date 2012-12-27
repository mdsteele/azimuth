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

/*===========================================================================*/

// Test if the point is in the polygon.  The polygon must be
// non-self-intersecting, but it need not be convex.
bool az_polygon_contains(az_polygon_t polygon, az_vector_t point);

// Test if the point is in the polygon.  The polygon must be convex, and the
// vertices must come in counter-clockwise order.  This function is more
// efficient, but less general, than az_polygon_contains().
bool az_convex_polygon_contains(az_polygon_t polygon, az_vector_t point);

/*===========================================================================*/

// Determine if the specified circle overlaps any part of the infinite line
// passing through p1 and p2.
bool az_circle_touches_line(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t center);

// Determine if the specified circle overlaps any part of the line segment
// from p1 to p2.
bool az_circle_touches_line_segment(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t center);

// Determine if the specified circle overlaps any part of the polygon.
bool az_circle_touches_polygon(
    az_polygon_t polygon, double radius, az_vector_t center);

// Like az_circle_touches_polygon, but translates and rotates the polygon as
// specified before deciding if the circle touches it.
bool az_circle_touches_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double radius, az_vector_t center);

/*===========================================================================*/

// Determine if a ray, travelling delta from start, will ever pass within the
// specified circle.  This is like az_ray_hits_circle, but doesn't bother to
// determine the impact point or normal -- just the yes/no return value.
bool az_ray_hits_bounding_circle(az_vector_t start, az_vector_t delta,
                                 az_vector_t center, double radius);

// The following functions each determine if a ray, travelling delta from
// start, will ever intersect a particular shape (depending on the function).
// If it does, the function stores in *point_out the point at which it touches
// the shape (if point_out is non-NULL) and a vector normal to the shape at the
// impact point in *normal_out (if normal_out is non-NULL).  No guarantees are
// made about the length of the normal vector, and in particular it may be zero
// (if there is no well-defined normal for the collision).

// Determine if the ray will hit the given circle.
bool az_ray_hits_circle(double radius, az_vector_t center,
                        az_vector_t start, az_vector_t delta,
                        az_vector_t *point_out, az_vector_t *normal_out);

// Determine if the ray will hit the given polygon.
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

/*===========================================================================*/

// The following functions each determine if a circle with the specified
// radius, travelling delta from start, will ever intersect a particular shape
// (depending on the function).  If it does, the function stores in *pos_out
// the first position of the circle at which it touches the shape (if pos_out
// is non-NULL) and in *impact_out the point of intersection (if impact_out is
// non-NULL).

// Determine if the circle will ever intersect the given point.
bool az_circle_hits_point(
    az_vector_t point, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will ever intersect the given circle.
bool az_circle_hits_circle(
    double sradius, az_vector_t center, double mradius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will ever intersect the infinite line passing
// through p1 and p2.
bool az_circle_hits_line(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will ever intersect the finite line segment between
// p1 and p2.
bool az_circle_hits_line_segment(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will ever intersect the polygon.
bool az_circle_hits_polygon(
    az_polygon_t polygon, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will ever intersect the transformed polygon.
bool az_circle_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *impact_out);

/*===========================================================================*/

// The following functions each determine if a circular ray, travelling from
// start around spin_center by spin_angle radians, will ever intersect a
// particular shape (depending on the function).  If it does, the function
// stores in *angle_out the angle travelled by the ray until impact (if
// angle_out is non-NULL), in *point_out the point at which it touches the
// shape (if point_out is non-NULL), and a vector normal to the shape at the
// impact point in *normal_out (if normal_out is non-NULL).  No guarantees are
// made about the length of the normal vector, and in particular it may be zero
// (if there is no well-defined normal for the collision).

// Determine if the circular ray will hit the given circle.
bool az_arc_ray_hits_circle(
    double circle_radius, az_vector_t circle_center, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out);

// Determine if the circular ray will hit the infinite line passing through p1
// and p2.
bool az_arc_ray_hits_line(
    az_vector_t p1, az_vector_t p2,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out);

// Determine if the circular ray will hit the finite line segment between p1
// and p2.
bool az_arc_ray_hits_line_segment(
    az_vector_t p1, az_vector_t p2,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out);

// Determine if the circular ray will hit the polygon.
bool az_arc_ray_hits_polygon(
    az_polygon_t polygon,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out);

// Determine if the circular ray will hit the transformed polygon.
bool az_arc_ray_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out);

/*===========================================================================*/

// The following functions each determine if a circle, travelling in a circular
// path from start around spin_center by spin_angle radians, will ever
// intersect a particular shape (depending on the function).  If it does, the
// function stores in *angle_out the angle travelled by the circle until impact
// (if angle_out is non-NULL), in *pos_out the first position of the circle at
// which it touches the shape (if pos_out is non-NULL), and in *impact_out the
// point of intersection (if impact_out is non-NULL).

// Determine if the circle will hit the given point.
bool az_arc_circle_hits_point(
    az_vector_t point, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will hit the given circle.
bool az_arc_circle_hits_circle(
    double sradius, az_vector_t center, double mradius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will hit the infinite line passing through p1 and
// p2.
bool az_arc_circle_hits_line(
    az_vector_t p1, az_vector_t p2, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will hit the finite line segment between p1 and p2.
bool az_arc_circle_hits_line_segment(
    az_vector_t p1, az_vector_t p2, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will hit the polygon.
bool az_arc_circle_hits_polygon(
    az_polygon_t polygon, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

// Determine if the circle will hit the transformed polygon.
bool az_arc_circle_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out);

/*===========================================================================*/

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
