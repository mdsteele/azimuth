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

/*===========================================================================*/

// A 2d vector:
typedef struct {
  double x, y;
} az_vector_t;

// The zero vector:
extern const az_vector_t AZ_VZERO;

// Return false for the zero vector, true otherwise.
bool az_vnonzero(az_vector_t v);

// Create a vector from polar coordinates.
az_vector_t az_vpolar(double magnitude, double theta);

// Add two vectors.
az_vector_t az_vadd(az_vector_t v1, az_vector_t v2);
// Subtract the second vector from the first.
az_vector_t az_vsub(az_vector_t v1, az_vector_t v2);
// Negate a vector.
az_vector_t az_vneg(az_vector_t v);
// Multiply a vector by a scalar.
az_vector_t az_vmul(az_vector_t v, double f);
// Divide a vector by a scalar.  The scalar must be nonzero.
az_vector_t az_vdiv(az_vector_t v, double f);

// Compute the dot product of the two vectors.
double az_vdot(az_vector_t v1, az_vector_t v2);
// Compute the magnitude of the cross product of the two vectors.
double az_vcross(az_vector_t v1, az_vector_t v2);

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

// Get the length of the vector.
double az_vnorm(az_vector_t v);
// Return a unit vector with the same direction as the given vector.  If the
// vector is zero, returns a unit vector along the x-axis (so that az_vtheta
// returns zero for both vectors).
az_vector_t az_vunit(az_vector_t v);
// Return a vector with the same direction as the given vector, but with the
// given new length.  The input vector is permitted to be zero, but in that
// case no guarantees are made about the direction of the output vector.
az_vector_t az_vwithlen(az_vector_t v, double length);
// Return a vector with the same direction as the given input vector, but with
// length equal to the min of `max_length` and the input vector's length.
az_vector_t az_vcaplen(az_vector_t v, double max_length);

// Get the polar theta angle of the vector.  Returns zero for the zero vector.
double az_vtheta(az_vector_t v);

// Get the distance between two vectors.
double az_vdist(az_vector_t v1, az_vector_t v2);
// Determine if two points are within the given distance of each other.
bool az_vwithin(az_vector_t v1, az_vector_t v2, double dist);

/*===========================================================================*/

// Compute a mod b; b must be nonzero.  Note that this is _not_ the same as the
// % operator; in C99, the return value of the % operator is specified to have
// the sign of the dividend (i.e. it is a remainder operation), whereas this
// function uses the sign of the divisor (i.e. it is a modulo operation).
int az_modulo(int a, int b);

// Compute a mod b, with the result having the same sign as the sign argument.
// This contrasts to fmod(a, b), which uses the sign of a, and az_modulo, which
// uses the sign of b (and operates on ints rather than doubles).  The b
// argument must be nonzero.
double az_signmod(double a, double b, double sign);

// Add or subtract a multiple of 2pi from the given number so that the result
// is between -pi and pi.
double az_mod2pi(double theta);
// Like az_mod2pi, but make the result be between 0 and 2pi.
double az_mod2pi_nonneg(double theta);
// Like az_mod2pi, but make the result be between -2pi and 0.
double az_mod2pi_nonpos(double theta);

// Add or subtract `delta` from the absolute angle `theta` to move it towards
// the absolute angle `goal`, but don't overshoot if the two angles are within
// `delta` of each other.
double az_angle_towards(double theta, double delta, double goal);

// Min and max functions for ints:
int az_imin(int a, int b);
int az_imax(int a, int b);

// Test if two (finite) doubles are approximately equal.
bool az_dapprox(double a, double b);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_VECTOR_H_
