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

#include "azimuth/util/vector.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

/*===========================================================================*/

const az_vector_t AZ_VZERO = {.x = 0, .y = 0};

static bool vfinite(az_vector_t v) {
  return isfinite(v.x) && isfinite(v.y);
}

az_vector_t az_vpolar(double magnitude, double theta) {
  assert(isfinite(magnitude));
  assert(isfinite(theta));
  return (az_vector_t){.x = magnitude * cos(theta),
                       .y = magnitude * sin(theta)};
}

az_vector_t az_vadd(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x + v2.x, .y = v1.y + v2.y};
}

az_vector_t az_vsub(az_vector_t v1, az_vector_t v2) {
  return (az_vector_t){.x = v1.x - v2.x, .y = v1.y - v2.y};
}

az_vector_t az_vneg(az_vector_t v) {
  return (az_vector_t){.x = -v.x, .y = -v.y};
}

az_vector_t az_vmul(az_vector_t v, double f) {
  return (az_vector_t){.x = v.x * f, .y = v.y * f};
}

az_vector_t az_vdiv(az_vector_t v, double f) {
  assert(f != 0.0);
  return (az_vector_t){.x = v.x / f, .y = v.y / f};
}

double az_vdot(az_vector_t v1, az_vector_t v2) {
  return v1.x * v2.x + v1.y * v2.y;
}

double az_vcross(az_vector_t v1, az_vector_t v2) {
  return v1.x * v2.y - v1.y * v2.x;
}

az_vector_t az_vproj(az_vector_t v1, az_vector_t v2) {
  assert(vfinite(v1));
  assert(vfinite(v2));
  const double sqnorm = az_vdot(v2, v2);
  assert(sqnorm > 0.0);
  return az_vmul(v2, az_vdot(v1, v2) / sqnorm);
}

az_vector_t az_vflatten(az_vector_t v1, az_vector_t v2) {
  return az_vsub(v1, az_vproj(v1, v2));
}

az_vector_t az_vrotate(az_vector_t v, double radians) {
  assert(vfinite(v));
  assert(isfinite(radians));
  const double c = cos(radians);
  const double s = sin(radians);
  return (az_vector_t){.x = v.x * c - v.y * s, .y = v.y * c + v.x * s};
}

az_vector_t az_vrot90ccw(az_vector_t v) {
  return (az_vector_t){.x = -v.y, .y = v.x};
}

double az_vnorm(az_vector_t v) {
  assert(vfinite(v));
  return hypot(v.x, v.y);
}

az_vector_t az_vunit(az_vector_t v) {
  assert(vfinite(v));
  const double norm = az_vnorm(v);
  assert(norm > 0.0);
  return az_vdiv(v, norm);
}

az_vector_t az_vwithlen(az_vector_t v, double length) {
  assert(vfinite(v));
  const double len = az_vnorm(v);
  assert(len >= 0.0);
  if (len > 0.0) return az_vmul(v, length / len);
  else return (az_vector_t){length, 0.0};
}

az_vector_t az_vcaplen(az_vector_t v, double max_length) {
  assert(vfinite(v));
  const double length = az_vnorm(v);
  return (length <= max_length ? v : az_vmul(v, max_length / length));
}

double az_vtheta(az_vector_t v) {
  assert(vfinite(v));
  return atan2(v.y, v.x);
}

double az_vdist(az_vector_t v1, az_vector_t v2) {
  return az_vnorm(az_vsub(v1, v2));
}

bool az_vwithin(az_vector_t v1, az_vector_t v2, double dist) {
  assert(isfinite(dist));
  return ((v1.x - v2.x) * (v1.x - v2.x) +
          (v1.y - v2.y) * (v1.y - v2.y) <= dist * dist);
}

/*===========================================================================*/

int az_modulo(int a, int b) {
  assert(b != 0);
  const int remainder = a % b;
  if (remainder == 0) return 0;
  return ((a < 0) ^ (b < 0) ? remainder + b : remainder);
}

double az_mod2pi(double theta) {
  assert(isfinite(theta));
  return theta - AZ_TWO_PI * floor((theta + AZ_PI) / AZ_TWO_PI);
}

double az_angle_towards(double theta, double delta, double goal) {
  const double difference = az_mod2pi(theta - goal);
  return az_mod2pi(difference < 0.0 ?
                   (-difference <= delta ? goal : theta + delta) :
                   (difference <= delta ? goal : theta - delta));
}

int az_imin(int a, int b) {
  return a <= b ? a : b;
}

int az_imax(int a, int b) {
  return a > b ? a : b;
}

#define EPSILON 0.00000001

bool az_dapprox(double a, double b) {
  assert(isfinite(a));
  assert(isfinite(b));
  const double d = a - b;
  return (d < EPSILON && d > -EPSILON);
}

/*===========================================================================*/
