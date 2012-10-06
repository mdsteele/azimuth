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
#ifndef AZIMUTH_VECTOR_H_
#define AZIMUTH_VECTOR_H_

#include <stdbool.h>

/*===========================================================================*/

// Constants:
#define AZ_PI 3.1415926535897931
#define AZ_TWO_PI 6.2831853071795862
#define AZ_HALF_PI 1.5707963267948966
#define AZ_PI_EIGHTHS 0.39269908169872414

// Convert between radians and degrees:
#define AZ_RAD2DEG(radians) ((double)(radians) * 57.295779513082323)
#define AZ_DEG2RAD(degrees) ((double)(degrees) * 0.017453292519943295)

// A 2d vector:
typedef struct {
  double x, y;
} az_vector_t;

// The zero vector:
extern const az_vector_t AZ_VZERO;

// Create a vector from polar coordinates.
az_vector_t az_vpolar(double magnitude, double theta);

// Add two vectors.
az_vector_t az_vadd(az_vector_t v1, az_vector_t v2);
// Subtract the second vector from the first.
az_vector_t az_vsub(az_vector_t v1, az_vector_t v2);
// Multiply a vector by a scalar.
az_vector_t az_vmul(az_vector_t v, double f);
// Divide a vector by a scalar.
az_vector_t az_vdiv(az_vector_t v, double f);

// Get the length of the vector.
double az_vnorm(az_vector_t v);

// Get the polar theta angle of the vector.
double az_vtheta(az_vector_t v);

// Determine if two points are within the given distance of each other.
bool az_vwithin(az_vector_t v1, az_vector_t v2, double dist);

// Add or subtract a multiple of 2pi from the given number so that the result
// is between -pi and pi.
double az_mod2pi(double theta);

// Min and max functions for ints and doubles:
int az_imin(int a, int b);
int az_imax(int a, int b);
double az_dmin(double a, double b);
double az_dmax(double a, double b);

/*===========================================================================*/

#endif // AZIMUTH_VECTOR_H_
