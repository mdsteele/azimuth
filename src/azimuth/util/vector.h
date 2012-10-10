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
#ifndef AZIMUTH_UTIL_VECTOR_H_
#define AZIMUTH_UTIL_VECTOR_H_

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
// Divide a vector by a scalar.  The scalar must be nonzero.
az_vector_t az_vdiv(az_vector_t v, double f);

// Compute the dot product of the two vectors.
double az_vdot(az_vector_t v1, az_vector_t v2);
// Project the first vector onto the second.  The second vector must be
// nonzero.
az_vector_t az_vproj(az_vector_t v1, az_vector_t v2);
// Flatten the first vector with respect to the second.  The second vector must
// be nonzero.
az_vector_t az_vflatten(az_vector_t v1, az_vector_t v2);

// Rotate a vector counterclockwise by the given angle.
az_vector_t az_vrotate(az_vector_t v, double radians);
// Rotate a vector 90 degrees counterclockwise.
az_vector_t az_vrot90ccw(az_vector_t v);
// Return the vector v, relative to an object at the given position and
// angle.  This is essentially a combination of az_vsub and az_vrotate.
az_vector_t az_vrelative(az_vector_t v, az_vector_t pos, double angle);

// Get the length of the vector.
double az_vnorm(az_vector_t v);
// Return a unit vector with the same direction as the given vector.  The
// vector must be nonzero.
az_vector_t az_vunit(az_vector_t v);

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

// Test if two (finite) doubles are approximately equal.
bool az_dapprox(double a, double b);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_VECTOR_H_
