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

#include "azimuth/state/gravfield.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

bool az_trapezoidal(az_gravfield_kind_t kind) {
  assert(kind != AZ_GRAV_NOTHING);
  switch (kind) {
    case AZ_GRAV_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_GRAV_TRAPEZOID:
    case AZ_GRAV_WATER:
      return true;
    case AZ_GRAV_SECTOR_PULL:
    case AZ_GRAV_SECTOR_SPIN:
      return false;
  }
  AZ_ASSERT_UNREACHABLE();
}

double az_sector_interior_angle(const az_gravfield_size_t *size) {
  const double radians =
    az_mod2pi_nonneg(AZ_DEG2RAD(size->sector.sweep_degrees));
  return (radians == 0.0 ? AZ_TWO_PI : radians);
}

bool az_point_within_gravfield(const az_gravfield_t *gravfield,
                               az_vector_t point) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  const az_gravfield_size_t *size = &gravfield->size;
  if (az_trapezoidal(gravfield->kind)) {
    const double semilength = size->trapezoid.semilength;
    const double front_offset = size->trapezoid.front_offset;
    const double front_semiwidth = size->trapezoid.front_semiwidth;
    const double rear_semiwidth = size->trapezoid.rear_semiwidth;
    const az_vector_t vertices[4] = {
      {semilength, front_offset - front_semiwidth},
      {semilength, front_offset + front_semiwidth},
      {-semilength, rear_semiwidth},
      {-semilength, -rear_semiwidth}
    };
    const az_polygon_t trapezoid = AZ_INIT_POLYGON(vertices);
    return az_polygon_contains(
        trapezoid, az_vrotate(az_vsub(point, gravfield->position),
                              -gravfield->angle));
  } else {
    const double thickness = size->sector.thickness;
    assert(thickness > 0.0);
    const double inner_radius = size->sector.inner_radius;
    assert(inner_radius >= 0.0);
    const double outer_radius = inner_radius + thickness;
    return (az_vwithin(point, gravfield->position, outer_radius) &&
            !az_vwithin(point, gravfield->position, inner_radius) &&
            az_mod2pi_nonneg(az_vtheta(az_vsub(point, gravfield->position)) -
                             gravfield->angle) <=
            az_sector_interior_angle(size));
  }
}

bool az_ray_hits_water_surface(
    const az_gravfield_t *gravfield, az_vector_t start, az_vector_t delta,
    az_vector_t *point_out, az_vector_t *normal_out) {
  assert(gravfield->kind == AZ_GRAV_WATER);
  const double semilength = gravfield->size.trapezoid.semilength;
  const double front_offset = gravfield->size.trapezoid.front_offset;
  const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
  const double position_norm = az_vnorm(gravfield->position);
  const double arc_radius =
    hypot(fmax(front_offset + front_semiwidth, front_offset - front_semiwidth),
          position_norm + semilength);
  const az_vector_t arc_center =
    az_vsub(gravfield->position, az_vpolar(position_norm, gravfield->angle));
  const double theta1 = atan2(front_offset - front_semiwidth,
                              position_norm + semilength);
  const double theta2 = atan2(front_offset + front_semiwidth,
                              position_norm + semilength);
  const double min_theta = az_mod2pi(theta1 + gravfield->angle);
  const double theta_span = az_mod2pi_nonneg(theta2 - theta1);
  return az_ray_hits_arc(arc_radius, arc_center, min_theta, theta_span,
                         start, delta, point_out, normal_out);
}

/*===========================================================================*/
