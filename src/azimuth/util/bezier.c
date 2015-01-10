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

#include "azimuth/util/bezier.h"

#include <assert.h>
#include <math.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

az_vector_t az_cubic_bezier_point(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    double param) {
  assert(param >= 0.0 && param <= 1.0);
  const double t = param;
  const double s = 1.0 - param;
  return az_vadd(
      az_vadd(az_vmul(start, s*s*s),
              az_vmul(ctrl1, 3*s*s*t)),
      az_vadd(az_vmul(ctrl2, 3*s*t*t),
              az_vmul(end, t*t*t)));
}

double az_cubic_bezier_angle(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    double param) {
  assert(param >= 0.0 && param <= 1.0);
  const double t = param;
  const double s = 1.0 - param;
  return az_vtheta(az_vadd(
      az_vadd(az_vmul(start, -3*s*s),
              az_vmul(ctrl1, 3*s*s - 6*s*t)),
      az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
              az_vmul(end, 3*t*t))));
}

double az_cubic_bezier_arc_length(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    int num_steps, double param_start, double param_end) {
  assert(num_steps > 0);
  assert(0.0 <= param_start && param_start <= param_end && param_end <= 1.0);
  const double step = (param_end - param_start) / num_steps;
  double arc_length = 0.0;
  for (int i = 0; i < num_steps; ++i) {
    const double t = param_start + (i + 0.5) * step;
    const double s = 1.0 - t;
    arc_length += step * az_vnorm(az_vadd(
        az_vadd(az_vmul(start, -3*s*s),
                az_vmul(ctrl1, 3*s*s - 6*s*t)),
        az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
                az_vmul(end, 3*t*t))));
  }
  return arc_length;
}

double az_cubic_bezier_arc_param(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    int num_steps, double param_start, double arc_length) {
  assert(num_steps > 0);
  assert(0.0 <= param_start && param_start <= 1.0);
  const double step = (1.0 - param_start) / num_steps;
  double prev_length = 0.0;
  for (int i = 0; i < num_steps; ++i) {
    const double t = param_start + (i + 0.5) * step;
    const double s = 1.0 - t;
    const double next_length = prev_length + step * az_vnorm(az_vadd(
        az_vadd(az_vmul(start, -3*s*s),
                az_vmul(ctrl1, 3*s*s - 6*s*t)),
        az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
                az_vmul(end, 3*t*t))));
    if (fabs(arc_length - next_length) >
        fabs(arc_length - prev_length)) {
      return param_start + i * step;
    }
    prev_length = next_length;
  }
  return 1.0;
}

/*===========================================================================*/
