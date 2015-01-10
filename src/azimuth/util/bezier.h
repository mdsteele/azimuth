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
#ifndef AZIMUTH_UTIL_BEZIER_H_
#define AZIMUTH_UTIL_BEZIER_H_

#include "azimuth/util/vector.h"

/*===========================================================================*/

az_vector_t az_cubic_bezier_point(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    double param);

double az_cubic_bezier_angle(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    double param);

// Calculate the arc length between two points on a cubic bezier curve.  This
// is done numerically, so the result is approximate; increase the num_steps
// argument to get a more accurate result.
double az_cubic_bezier_arc_length(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    int num_steps, double param_start, double param_end);

// Calculate the parameter (from 0 to 1) for the point on the given cubic
// bezier curve that lies arc_length away from the point for param_start.  This
// is done numerically, so the result is approximate; increase the num_steps
// argument to get a more accurate result.
double az_cubic_bezier_arc_param(
    az_vector_t start, az_vector_t ctrl1, az_vector_t ctrl2, az_vector_t end,
    int num_steps, double param_start, double arc_length);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_BEZIER_H_
