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

#include "azimuth/vector.h"

#include <math.h>

const az_vector_t AZ_VZERO = {.x = 0, .y = 0};

az_vector_t az_vpolar(double magnitude, double theta) {
  return (az_vector_t){.x = magnitude * cos(theta),
                       .y = magnitude * sin(theta)};
}

az_vector_t az_vadd(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x + v2.x, .y = v1.y + v2.y};
}

az_vector_t az_vsub(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x - v2.x, .y = v1.y - v2.y};
}

az_vector_t az_vmul(az_vector_t v, double f) {
  return (az_vector_t){.x = v.x * f, .y = v.y * f};
}

az_vector_t az_vdiv(az_vector_t v, double f) {
  return (az_vector_t){.x = v.x / f, .y = v.y / f};
}

double az_vnorm(az_vector_t v) {
  return hypot(v.x, v.y);
}

double az_vtheta(az_vector_t v) {
  return atan2(v.y, v.x);
}

int az_imin(int a, int b) {
  return a <= b ? a : b;
}

int az_imax(int a, int b) {
  return a > b ? a : b;
}

double az_dmin(double a, double b) {
  return a <= b ? a : b;
}

double az_dmax(double a, double b) {
  return a > b ? a : b;
}

/*===========================================================================*/
