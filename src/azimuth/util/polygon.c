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

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h> // for NULL

#include "azimuth/util/vector.h"

/*===========================================================================*/

static bool solve_quadratic(double a, double b, double c,
                            double *t1, double *t2) {
  const double discriminant = b * b - 4.0 * a * c;
  if (discriminant < 0.0) return false;
  const double root = sqrt(discriminant);
  *t1 = (-b + root) / (2.0 * a);
  *t2 = (-b - root) / (2.0 * a);
  return true;
}

/*===========================================================================*/

bool az_polygon_contains(az_polygon_t polygon, az_vector_t point) {
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

/*===========================================================================*/

bool az_circle_touches_line(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t center) {
  assert(radius >= 0.0);
  // Calculate the shortest vector from center to any point on the line.
  const az_vector_t to_line =
    az_vflatten(az_vsub(p1, center), az_vsub(p1, p2));
  // Determine if that vector is no longer than radius.
  return (az_vdot(to_line, to_line) <= radius * radius);
}

static bool circle_touches_line_segment_internal(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t center) {
  assert(radius >= 0.0);
  if (!az_circle_touches_line(p1, p2, radius, center)) return false;
  const az_vector_t seg = az_vsub(p2, p1);
  return (az_vdot(seg, az_vsub(center, p1)) >= 0.0 &&
          az_vdot(seg, az_vsub(center, p2)) <= 0.0);
}

bool az_circle_touches_line_segment(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t center) {
  return (az_vwithin(center, p1, radius) ||
          az_vwithin(center, p2, radius) ||
          circle_touches_line_segment_internal(p1, p2, radius, center));
}

bool az_circle_touches_polygon(
    az_polygon_t polygon, double radius, az_vector_t center) {
  for (int i = 0; i < polygon.num_vertices; ++i) {
    if (az_vwithin(center, polygon.vertices[i], radius)) return true;
  }
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    if (circle_touches_line_segment_internal(
            polygon.vertices[i], polygon.vertices[j],
            radius, center)) return true;
  }
  return az_polygon_contains(polygon, center);
}

bool az_circle_touches_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double radius, az_vector_t center) {
  return az_circle_touches_polygon(polygon, radius,
      az_vrotate(az_vsub(center, polygon_position), -polygon_angle));
}

/*===========================================================================*/

bool az_ray_hits_bounding_circle(az_vector_t start, az_vector_t delta,
                                 az_vector_t center, double radius) {
  return az_circle_hits_point(center, radius, start, delta, NULL, NULL);
}

bool az_ray_hits_circle(
    double radius, az_vector_t center, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  az_vector_t point;
  if (!az_circle_hits_point(center, radius, start, delta,
                            &point, NULL)) return false;
  if (point_out != NULL) *point_out = point;
  if (normal_out != NULL) *normal_out = az_vsub(point, center);
  return true;
}

bool az_ray_hits_line_segment(
    az_vector_t p1, az_vector_t p2, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  const az_vector_t edelta = az_vsub(p2, p1);
  const double denom = az_vcross(delta, edelta);
  // Make sure the line segment isn't parallel to the ray.
  if (denom == 0.0) return false;
  const az_vector_t rel = az_vsub(p1, start);
  // Make sure that the ray hits the line between the two vertices.
  const double u = az_vcross(rel, delta) / denom;
  assert(isfinite(u));
  if (u < 0.0 || u >= 1.0) return false;
  // Make sure that the ray hits the line segment within the bounds of the ray.
  const double t = az_vcross(rel, edelta) / denom;
  assert(isfinite(t));
  if (t < 0.0 || t > 1.0) return false;
  if (point_out != NULL) {
    *point_out = az_vadd(start, az_vmul(delta, t));
  }
  if (normal_out != NULL) {
    *normal_out = az_vsub(az_vproj(delta, edelta), delta);
  }
  return true;
}

static bool ray_hits_polygon_internal(
    az_polygon_t polygon, az_vector_t start, az_vector_t delta,
    double *time_out, az_vector_t *normal_out) {
  assert(time_out != NULL);
  const az_vector_t *vertices = polygon.vertices;
  // We're going to iterate through the edges of the polygon, testing each one
  // for an intersection with the ray.  We keep track of the smallest "time"
  // (from 0 to 1) for which the ray hits something.  We also keep track of how
  // many edges we hit at all (at any nonnegative "time"), because if we hit an
  // odd number of edges that means we started inside the polygon, and should
  // actually hit at time zero.
  double best_time = INFINITY;
  az_vector_t best_edge = AZ_VZERO;
  int hits = 0;
  // Iterate over all edges in the polygon.  On each iteration, i is the index
  // of the "primary" vertex, and j is the index of the vertex that comes just
  // after it in the list (wrapping around at the end).
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    const az_vector_t edelta = az_vsub(vertices[j], vertices[i]);
    const double denom = az_vcross(delta, edelta);
    // Make sure the edge isn't parallel to the ray.
    if (denom == 0.0) continue;
    const az_vector_t rel = az_vsub(vertices[i], start);
    // Make sure that the ray hits the edge line between the two vertices.
    const double u = az_vcross(rel, delta) / denom;
    assert(isfinite(u));
    if (u < 0.0 || u >= 1.0) continue;
    // Make sure that the ray hits the edge within the bounds of the ray.
    const double t = az_vcross(rel, edelta) / denom;
    assert(isfinite(t));
    if (t < 0.0) continue;
    ++hits;
    if (t > 1.0) continue;
    if (t < best_time) {
      best_time = t;
      best_edge = edelta;
    }
  }
  // If we started inside the polygon, then we actually hit at time zero.
  if (hits % 2 != 0) {
    best_time = 0.0;
    best_edge = AZ_VZERO;
  }
  // If the ray hits the polygon, store the collision point in point_out, and
  // the normal vector in normal_out.
  const bool did_hit = best_time != INFINITY;
  if (did_hit) {
    assert(hits > 0);
    assert(isfinite(best_time));
    assert(0.0 <= best_time && best_time <= 1.0);
    *time_out = best_time;
    if (normal_out != NULL) {
      if (best_edge.x == 0.0 && best_edge.y == 0.0) {
        *normal_out = az_vneg(delta);
      } else {
        *normal_out = az_vsub(az_vproj(delta, best_edge), delta);
      }
    }
  }
  return did_hit;
}

bool az_ray_hits_polygon(
    az_polygon_t polygon, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  double time = INFINITY;
  if (ray_hits_polygon_internal(polygon, start, delta, &time, normal_out)) {
    assert(isfinite(time));
    if (point_out != NULL) {
      *point_out = az_vadd(start, az_vmul(delta, time));
    }
    return true;
  }
  return false;
}

bool az_ray_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  if (az_ray_hits_polygon(polygon,
          az_vrotate(az_vsub(start, polygon_position), -polygon_angle),
          az_vrotate(delta, -polygon_angle), point_out, normal_out)) {
    if (point_out != NULL) {
      *point_out = az_vadd(az_vrotate(*point_out, polygon_angle),
                           polygon_position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, polygon_angle);
    }
    return true;
  }
  return false;
}

/*===========================================================================*/

bool az_circle_hits_point(
    az_vector_t point, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *normal_out) {
  assert(radius >= 0.0);
  // If the point is already inside the circle, we're immediately done.
  if (az_vwithin(point, start, radius)) {
    if (pos_out != NULL) *pos_out = start;
    if (normal_out != NULL) *normal_out = az_vsub(start, point);
    return true;
  }
  // Set up a quadratic equation modeling when the circle will hit the point
  // (assuming it travels t*delta from start at time t).  If there is no
  // solution, the circle misses the point; otherwise, we still need to check
  // if the solution is in range.
  const az_vector_t sdelta = az_vsub(start, point);
  double t1, t2;
  if (!solve_quadratic(az_vdot(delta, delta), 2.0 * az_vdot(sdelta, delta),
                       az_vdot(sdelta, sdelta) - radius * radius,
                       &t1, &t2)) return false;
  const double t = (0.0 <= t1 && (t1 <= t2 || t2 < 0.0) ? t1 : t2);
  if (0.0 <= t && t <= 1.0) {
    const az_vector_t pos = az_vadd(start, az_vmul(delta, t));
    if (pos_out != NULL) *pos_out = pos;
    if (normal_out != NULL) *normal_out = az_vsub(pos, point);
    return true;
  } else return false;
}

bool az_circle_hits_circle(
    double sradius, az_vector_t center, double mradius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *normal_out) {
  assert(sradius >= 0.0);
  assert(mradius >= 0.0);
  return az_circle_hits_point(center, sradius + mradius, start, delta,
                              pos_out, normal_out);
}

bool az_circle_hits_line(az_vector_t p1, az_vector_t p2, double radius,
                         az_vector_t start, az_vector_t delta,
                         az_vector_t *pos_out, az_vector_t *normal_out) {
  assert(radius >= 0.0);
  // Calculate the vector from start to the nearest point on the line.
  const az_vector_t to_line =
    az_vflatten(az_vsub(p1, start), az_vsub(p1, p2));
  // If the circle is already intersecting the line, then we're done.
  if (az_vdot(to_line, to_line) <= radius * radius) {
    if (pos_out != NULL) *pos_out = start;
    if (normal_out != NULL) *normal_out = az_vneg(to_line);
    return true;
  }
  // Else, if the circle is moving away from the line, it doesn't hit it.
  if (az_vdot(to_line, delta) <= 0.0) return false;
  // Else, calculate how long it takes the circle to hit the line.
  const double dist = az_vnorm(to_line);
  const double a = az_vnorm(az_vproj(delta, to_line));
  assert(a != 0.0);
  const double b = dist - radius;
  const double t = b / a;
  if (0.0 <= t && t <= 1.0) {
    if (pos_out != NULL) *pos_out = az_vadd(start, az_vmul(delta, t));
    if (normal_out != NULL) *normal_out = az_vneg(to_line);
    return true;
  }
  return false;
}

static bool circle_hits_line_segment_internal(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *normal_out) {
  az_vector_t pos, normal;
  if (!az_circle_hits_line(p1, p2, radius, start, delta, &pos, &normal)) {
    return false;
  }
  const az_vector_t seg = az_vsub(p2, p1);
  if (az_vdot(seg, az_vsub(pos, p1)) < 0.0 ||
      az_vdot(seg, az_vsub(pos, p2)) > 0.0) return false;
  if (pos_out != NULL) *pos_out = pos;
  if (normal_out != NULL) *normal_out = normal;
  return true;
}

bool az_circle_hits_line_segment(
    az_vector_t p1, az_vector_t p2, double radius, az_vector_t start,
    az_vector_t delta, az_vector_t *pos_out, az_vector_t *normal_out) {
  bool hit = false;
  az_vector_t hit_at;
  if (az_circle_hits_point(p1, radius, start, delta, &hit_at, normal_out)) {
    hit = true;
    delta = az_vsub(hit_at, start);
  }
  if (az_circle_hits_point(p2, radius, start, delta, &hit_at, normal_out)) {
    hit = true;
    delta = az_vsub(hit_at, start);
  }
  if (circle_hits_line_segment_internal(p1, p2, radius, start, delta,
                                        &hit_at, normal_out)) {
    hit = true;
  }
  if (hit && pos_out != NULL) *pos_out = hit_at;
  return hit;
}

bool az_circle_hits_polygon(
    az_polygon_t polygon, double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *normal_out) {
  // If the circle starts within the polygon, count that as an immediate
  // impact and use the position as the normal (so that the normal points away
  // from the origin).
  if (az_polygon_contains(polygon, start)) {
    if (pos_out != NULL) *pos_out = start;
    if (normal_out != NULL) *normal_out = start;
    return true;
  }
  bool hit = false;
  az_vector_t pos;
  // Check if the circle hits any corners of the polygon.
  for (int i = 0; i < polygon.num_vertices; ++i) {
    if (az_circle_hits_point(polygon.vertices[i], radius, start, delta,
                             &pos, normal_out)) {
      hit = true;
      delta = az_vsub(pos, start);
    }
  }
  // Check if the circle hits any edges of the polygon.
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    if (circle_hits_line_segment_internal(
            polygon.vertices[i], polygon.vertices[j], radius, start, delta,
            &pos, normal_out)) {
      hit = true;
      delta = az_vsub(pos, start);
    }
  }
  // Return the final answer.
  if (hit && pos_out != NULL) {
    *pos_out = pos;
  }
  return hit;
}

bool az_circle_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double radius, az_vector_t start, az_vector_t delta,
    az_vector_t *pos_out, az_vector_t *normal_out) {
  if (az_circle_hits_polygon(
          polygon, radius,
          az_vrotate(az_vsub(start, polygon_position), -polygon_angle),
          az_vrotate(delta, -polygon_angle),
          pos_out, normal_out)) {
    if (pos_out != NULL) {
      *pos_out = az_vadd(az_vrotate(*pos_out, polygon_angle),
                         polygon_position);
    }
    if (normal_out != NULL) {
      *normal_out = az_vrotate(*normal_out, polygon_angle);
    }
    return true;
  }
  return false;
}

/*===========================================================================*/

bool az_arc_ray_might_hit_bounding_circle(
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    az_vector_t circle_center, double circle_radius) {
  // For now, just give a perfect answer.  If this turns out to be too
  // expensive, we can always rewrite this function to be more heuristic (at
  // the cost of some false positives).
  return az_arc_ray_hits_circle(circle_radius, circle_center, start,
                                spin_center, spin_angle, NULL, NULL, NULL);
}

bool az_arc_ray_hits_circle(
    double circle_radius, az_vector_t circle_center,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out) {
  // If the point is already inside the circle, we're immediately done.
  if (az_vwithin(start, circle_center, circle_radius)) {
    if (angle_out != NULL) *angle_out = 0.0;
    if (point_out != NULL) *point_out = start;
    if (normal_out != NULL) *normal_out = az_vsub(start, circle_center);
    return true;
  }
  // Otherwise, if the ray isn't going to move at all, we're done.
  else if (spin_angle == 0.0) return false;

  // Calculate our starting positions relative to spin_center.
  const az_vector_t rel_center = az_vsub(circle_center, spin_center);
  const double circle_dist = az_vnorm(rel_center);
  const az_vector_t rel_start = az_vsub(start, spin_center);
  const double spin_radius = az_vnorm(rel_start);

  // If the ray wouldn't hit the circle at any angle, we're done.
  if (fabs(circle_dist - spin_radius) > circle_radius) return false;
  const double start_theta = az_vtheta(rel_start);

  // Next we need to determine the earliest angle at which the ray will impact
  // the circle.  Derivation:
  //   Let cr = circle_radius
  //       sr = spin_radius
  //       (rx, ry) = rel_center
  //       t = start_theta + angle
  //   Our goal is to solve for `angle`.
  //   We set the distance between the ray and the circle center equal to the
  //   circle radius, and start solving for t:
  //     cr^2 = (sr*cos(t) - rx)^2 + (sr*sin(t) - ry)^2
  //     cr^2 = sr^2*cos^2(t) - 2*rx*sr*cos(t) + rx^2 +
  //            sr^2*sin^2(t) - 2*ry*sr*sin(t) + ry^2
  //   Factor and apply identity cos^2(t) + sin^2(t) = 1 to get:
  //     cr^2 = sr^2 + rx^2 + ry^2 - 2*sr*(rx*cos(t) + ry*sin(t))
  //     rx*cos(t) + ry*sin(t) = (sr^2 + rx^2 + ry^2 - cr^2) / (2 * sr)
  //   Next, pause for a moment and let s = atan2(rx, ry) (n.b. note the
  //   flipped arguments to atan2!) and cd = circle_dist, yielding:
  //     rx*cos(t) + ry*sin(t) = cd * (sin(s)*cos(t) + cos(s)*sin(t))
  //   Apply identity sin(s)*cos(t) + cos(s)*sin(t) = sin(s + t) to get:
  //     rx*cos(t) + ry*sin(t) = cd * sin(s + t)
  //   Substitute that in above and get:
  //     cd * sin(s + t) = (sr^2 + rx^2 + ry^2 - cr^2) / (2 * sr)
  //     sin(s + t) = (sr^2 + cd^2 - cr^2) / (2 * sr * cd)
  //     angle = asin((sr^2 + cd^2 - cr^2) / (2 * sr * cd)) - s - start_theta
  //   Done.  Now we just need to check both possible values of the arcsine.
  const double s = atan2(rel_center.x, rel_center.y); // n.b. x before y here
  // We know from checking above that 1) we didn't start within the circle, but
  // 2) we will hit the circle at some point (though possibly not within
  // spin_angle).  These facts imply that neither start nor circle_center can
  // be equal to spin_center, and therefore that spin_radius and circle_dist
  // are both nonzero.  This is good, because we'll be dividing by them soon.
  assert(spin_radius > 0.0);
  assert(circle_dist > 0.0);
  // Calculate the arcsine from the above equation, sanity-checking the range
  // of the sine first (it's guaranteed to be within range, but that fact may
  // not be obvious, hence the assertion).
  const double sine =
    (spin_radius*spin_radius + circle_dist*circle_dist -
     circle_radius*circle_radius) / (2.0 * spin_radius * circle_dist);
  assert(-1.0 <= sine && sine <= 1.0);
  const double arcsine = asin(sine);
  // The asin function will give us one possible value, but really there are
  // two possible values (since we can hit either of two sides of the circle).
  // So calculate the two values for `angle` (modulo multiples of 2pi):
  const double angle1 = arcsine - s - start_theta;
  const double angle2 = AZ_PI - arcsine - s - start_theta;
  // Pick the first angle at which we hit the circle (which depends on the sign
  // of spin_angle):
  const double angle =
    (spin_angle > 0.0 ?
     fmin(az_mod2pi_nonneg(angle1), az_mod2pi_nonneg(angle2)) :
     fmax(az_mod2pi_nonpos(angle1), az_mod2pi_nonpos(angle2)));
  // If the angle is within spin_angle (note that the two will have the same
  // sign), then we hit the circle, otherwise we don't.
  if (fabs(angle) > fabs(spin_angle)) return false;
  if (angle_out != NULL) *angle_out = angle;
  if (point_out != NULL || normal_out != NULL) {
    const az_vector_t point =
      az_vadd(spin_center, az_vpolar(spin_radius, start_theta + angle));
    if (point_out != NULL) *point_out = point;
    if (normal_out != NULL) *normal_out = az_vsub(point, circle_center);
  }
  return true;
}

bool az_arc_ray_hits_line(
    az_vector_t p1, az_vector_t p2,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out) {
  // Calculate start, and the line, relative to spin_center.  We will use
  // parametric form for the line; it is the locus of points p0 + t * delta for
  // all values of t.
  const az_vector_t p0 = az_vsub(p1, spin_center);
  const az_vector_t delta = az_vsub(p2, p1);
  const az_vector_t rel_start = az_vsub(start, spin_center);
  // Next we need to calculate the two points on the circle in which the ray
  // travels that intersect the line.  Derivation:
  //   Let sr = spin_radius (that is, vnorm(rel_start))
  //       (x0, y0) = p0
  //       (dx, dy) = delta
  //   Given a point on the line determined by t, we set the distance between
  //   that point and the origin (i.e. spin_center) equal to the spin_radius.
  //   Then we solve for t:
  //     sr^2 = (x0 + dx*t)^2 + (y0 + dy*t)^2
  //     sr^2 = x0^2 + 2*x0*dx*t + dx^2*t^2 + y0^2 + 2*y0*dy*t + dy^2*t^2
  //     0 = (dx^2 + dy^2)*t^2 + (2*x0*dx + 2*y0*dy)*t + x0^2 + y0^2 - sr^2
  //   This is a simple quadratic equation.  If there is no solution, that
  //   means that the ray can never hit the line, and we are done.
  double t1, t2;
  if (!solve_quadratic(az_vdot(delta, delta), 2.0 * az_vdot(p0, delta),
                       az_vdot(p0, p0) - az_vdot(rel_start, rel_start),
                       &t1, &t2)) return false;
  // We now know the two points on the line that intersect the ray's circle.
  // Next we find the ray angles that correspond to those two points:
  const double start_theta = az_vtheta(rel_start);
  const double angle1 =
    az_vtheta(az_vadd(p0, az_vmul(delta, t1))) - start_theta;
  const double angle2 =
    az_vtheta(az_vadd(p0, az_vmul(delta, t2))) - start_theta;
  // Pick the first angle at which we hit the circle (which depends on the sign
  // of spin_angle):
  const double angle =
    (spin_angle > 0.0 ?
     fmin(az_mod2pi_nonneg(angle1), az_mod2pi_nonneg(angle2)) :
     fmax(az_mod2pi_nonpos(angle1), az_mod2pi_nonpos(angle2)));
  // If the angle is within spin_angle (note that the two will have the same
  // sign), then we hit the circle, otherwise we don't.
  if (fabs(angle) > fabs(spin_angle)) return false;
  if (angle_out != NULL) *angle_out = angle;
  if (point_out != NULL) {
    *point_out = az_vadd(spin_center,
                         az_vrotate(az_vsub(start, spin_center), angle));
  }
  if (normal_out != NULL) {
    *normal_out = az_vflatten(az_vsub(start, p0), delta);
  }
  return true;
}

bool az_arc_ray_hits_line_segment(
    az_vector_t p1, az_vector_t p2,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out) {
  double angle;
  az_vector_t point, normal;
  if (!az_arc_ray_hits_line(p1, p2, start, spin_center, spin_angle,
                            &angle, &point, &normal)) return false;
  const az_vector_t seg = az_vsub(p2, p1);
  if (az_vdot(seg, az_vsub(point, p1)) < 0.0 ||
      az_vdot(seg, az_vsub(point, p2)) > 0.0) return false;
  if (angle_out != NULL) *angle_out = angle;
  if (point_out != NULL) *point_out = point;
  if (normal_out != NULL) *normal_out = normal;
  return true;
}

bool az_arc_ray_hits_polygon(
    az_polygon_t polygon,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out) {
  // If the point is already inside the polygon, we're immediately done.
  if (az_polygon_contains(polygon, start)) {
    if (angle_out != NULL) *angle_out = 0.0;
    if (point_out != NULL) *point_out = start;
    if (normal_out != NULL) *normal_out = AZ_VZERO;
    return true;
  }
  // Check if the ray hits any edges of the polygon.
  bool hit = false;
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    if (az_arc_ray_hits_line_segment(
            polygon.vertices[i], polygon.vertices[j], start, spin_center,
            spin_angle, &spin_angle, point_out, normal_out)) {
      hit = true;
    }
  }
  // Return the final answer.
  if (hit && angle_out != NULL) *angle_out = spin_angle;
  return hit;
}

bool az_arc_ray_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    az_vector_t start, az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *point_out, az_vector_t *normal_out) {
  if (!az_arc_ray_hits_polygon(polygon,
          az_vrotate(az_vsub(start, polygon_position), -polygon_angle),
          az_vrotate(az_vsub(spin_center, polygon_position), -polygon_angle),
          spin_angle, angle_out, point_out, normal_out)) return false;
  if (point_out != NULL) {
      *point_out = az_vadd(az_vrotate(*point_out, polygon_angle),
                           polygon_position);
  }
  if (normal_out != NULL) {
    *normal_out = az_vrotate(*normal_out, polygon_angle);
  }
  return true;
}

//===========================================================================//

bool az_arc_circle_hits_point(
    az_vector_t point, double radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(radius >= 0.0);
  az_vector_t pos;
  if (!az_arc_ray_hits_circle(radius, point, start, spin_center, spin_angle,
                              angle_out, &pos, impact_out)) return false;
  if (pos_out != NULL) *pos_out = pos;
  if (impact_out != NULL) *impact_out = az_vsub(pos, *impact_out);
  return true;
}

bool az_arc_circle_hits_circle(
    double sradius, az_vector_t center, double mradius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(sradius >= 0.0);
  assert(mradius >= 0.0);
  az_vector_t pos;
  if (!az_arc_ray_hits_circle(
          sradius + mradius, center, start, spin_center, spin_angle,
          angle_out, &pos, impact_out)) return false;
  if (pos_out != NULL) *pos_out = pos;
  if (impact_out != NULL) {
    *impact_out = az_vsub(pos, az_vwithlen(*impact_out, mradius));
  }
  return true;
}

bool az_arc_circle_hits_line(
    az_vector_t p1, az_vector_t p2, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(circle_radius >= 0.0);
  // If the circle is already touching the line, we're immediately done.
  if (az_circle_touches_line(p1, p2, circle_radius, start)) {
    if (angle_out != NULL) *angle_out = 0.0;
    if (pos_out != NULL) *pos_out = start;
    if (impact_out != NULL) {
      *impact_out =
        az_vadd(start, az_vflatten(az_vsub(p1, start), az_vsub(p1, p2)));
    }
    return true;
  }
  // Otherwise, move the line towards the circle center by circle_radius, and
  // then pretend the circle is a ray to find the impact point.
  const az_vector_t normal =
    az_vwithlen(az_vflatten(az_vsub(start, p1),
                            az_vsub(p2, p1)), circle_radius);
  az_vector_t pos;
  if (!az_arc_ray_hits_line(
          az_vadd(normal, p1), az_vadd(normal, p2), start, spin_center,
          spin_angle, angle_out, &pos, NULL)) return false;
  if (pos_out != NULL) *pos_out = pos;
  if (impact_out != NULL) *impact_out = az_vsub(pos, normal);
  return true;
}

static bool arc_circle_hits_line_segment_internal(
    az_vector_t p1, az_vector_t p2, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  assert(circle_radius >= 0.0);
  double angle;
  az_vector_t pos, impact;
  if (!az_arc_circle_hits_line(
          p1, p2, circle_radius, start, spin_center, spin_angle,
          &angle, &pos, &impact)) return false;
  const az_vector_t seg = az_vsub(p2, p1);
  if (az_vdot(seg, az_vsub(pos, p1)) < 0.0 ||
      az_vdot(seg, az_vsub(pos, p2)) > 0.0) return false;
  if (angle_out != NULL) *angle_out = angle;
  if (pos_out != NULL) *pos_out = pos;
  if (impact_out != NULL) *impact_out = impact;
  return true;
}

bool az_arc_circle_hits_line_segment(
    az_vector_t p1, az_vector_t p2, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  return (arc_circle_hits_line_segment_internal(
              p1, p2, circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out) ||
          az_arc_circle_hits_point(
              p1, circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out) ||
          az_arc_circle_hits_point(
              p2, circle_radius, start, spin_center, spin_angle,
              angle_out, pos_out, impact_out));
}

bool az_arc_circle_hits_polygon(
    az_polygon_t polygon, double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  // If the circle is already touching the polygon, count that as an immediate
  // impact.
  if (az_circle_touches_polygon(polygon, circle_radius, start)) {
    if (angle_out != NULL) *angle_out = 0.0;
    if (pos_out != NULL) *pos_out = start;
    if (impact_out != NULL) {
      *impact_out = az_vsub(start, az_vwithlen(start, circle_radius));
    }
    return true;
  }
  bool hit = false;
  // Check if the circle hits any corners of the polygon.
  for (int i = 0; i < polygon.num_vertices; ++i) {
    if (az_arc_circle_hits_point(
            polygon.vertices[i], circle_radius, start, spin_center, spin_angle,
            &spin_angle, pos_out, impact_out)) {
      hit = true;
    }
  }
  // Check if the circle hits any edges of the polygon.
  for (int i = polygon.num_vertices - 1, j = 0; i >= 0; j = i--) {
    if (arc_circle_hits_line_segment_internal(
            polygon.vertices[i], polygon.vertices[j], circle_radius, start,
            spin_center, spin_angle, &spin_angle, pos_out, impact_out)) {
      hit = true;
    }
  }
  // Return the final answer.
  if (hit && angle_out != NULL) *angle_out = spin_angle;
  return hit;
}

bool az_arc_circle_hits_polygon_trans(
    az_polygon_t polygon, az_vector_t polygon_position, double polygon_angle,
    double circle_radius, az_vector_t start,
    az_vector_t spin_center, double spin_angle,
    double *angle_out, az_vector_t *pos_out, az_vector_t *impact_out) {
  if (!az_arc_circle_hits_polygon(
          polygon, circle_radius,
          az_vrotate(az_vsub(start, polygon_position), -polygon_angle),
          az_vrotate(az_vsub(spin_center, polygon_position), -polygon_angle),
          spin_angle, angle_out, pos_out, impact_out)) return false;
  if (pos_out != NULL) {
    *pos_out = az_vadd(az_vrotate(*pos_out, polygon_angle), polygon_position);
  }
  if (impact_out != NULL) {
    *impact_out =
      az_vadd(az_vrotate(*impact_out, polygon_angle), polygon_position);
  }
  return true;
}

/*===========================================================================*/

bool az_lead_target(az_vector_t rel_position, az_vector_t rel_velocity,
                    double proj_speed, az_vector_t *rel_impact_out) {
  assert(proj_speed > 0.0);
  double t1, t2;
  if (!solve_quadratic(
          az_vdot(rel_velocity, rel_velocity) - proj_speed * proj_speed,
          2.0 * az_vdot(rel_position, rel_velocity),
          az_vdot(rel_position, rel_position), &t1, &t2)) return false;
  if (t1 < 0.0 && t2 < 0.0) return false;
  if (rel_impact_out != NULL) {
    const double t = (0.0 <= t1 && (t1 <= t2 || t2 < 0.0) ? t1 : t2);
    *rel_impact_out = az_vadd(rel_position, az_vmul(rel_velocity, t));
  }
  return true;
}

/*===========================================================================*/
