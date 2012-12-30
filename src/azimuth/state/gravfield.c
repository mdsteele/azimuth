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
#include <stdbool.h>

#include "azimuth/util/polygon.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

double az_sector_gravfield_interior_angle(const az_gravfield_t *gravfield) {
  assert(gravfield->kind == AZ_GRAV_SECTOR_SPIN ||
         gravfield->kind == AZ_GRAV_SECTOR_PULL);
  const double angle =
    az_mod2pi_nonneg(AZ_DEG2RAD(gravfield->size.sector.sweep_degrees));
  return (angle == 0.0 ? AZ_TWO_PI : angle);
}

bool az_point_within_gravfield(const az_gravfield_t *gravfield,
                               az_vector_t point) {
  assert(gravfield->kind != AZ_GRAV_NOTHING);
  if (gravfield->kind == AZ_GRAV_TRAPEZOID) {
    const double semilength = gravfield->size.trapezoid.semilength;
    const double front_offset = gravfield->size.trapezoid.front_offset;
    const double front_semiwidth = gravfield->size.trapezoid.front_semiwidth;
    const double rear_semiwidth = gravfield->size.trapezoid.rear_semiwidth;
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
    assert(gravfield->kind == AZ_GRAV_SECTOR_SPIN ||
           gravfield->kind == AZ_GRAV_SECTOR_PULL);
    const double thickness = gravfield->size.sector.thickness;
    assert(thickness > 0.0);
    const double inner_radius = gravfield->size.sector.inner_radius;
    assert(inner_radius >= 0.0);
    const double outer_radius = inner_radius + thickness;
    return (az_vwithin(point, gravfield->position, outer_radius) &&
            !az_vwithin(point, gravfield->position, inner_radius) &&
            az_mod2pi_nonneg(az_vtheta(az_vsub(point, gravfield->position)) -
                             gravfield->angle) <=
            az_sector_gravfield_interior_angle(gravfield));
  }
}

/*===========================================================================*/
