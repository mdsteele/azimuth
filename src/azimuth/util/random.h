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
#ifndef AZIMUTH_UTIL_RANDOM_H_
#define AZIMUTH_UTIL_RANDOM_H_

#include <stdint.h>

/*===========================================================================*/

typedef struct {
  uint32_t z, w;
} az_random_seed_t;

// Generates 32 random bits, based only on the seed value, and then updates the
// seed.  By always starting with the same seed value, this can generate a
// repeatable sequence of pseudorandom numbers.
uint32_t az_rand_uint32(az_random_seed_t *seed);

// Generates a random double from 0 to 1, based only on the seed value, and
// then updates the seed.  By always starting with the same seed value, this
// can generate a repeatable sequence of pseudorandom numbers.
double az_rand_udouble(az_random_seed_t *seed);

// Generates a random double from -1 to 1, based only on the seed value, and
// then updates the seed.  By always starting with the same seed value, this
// can generate a repeatable sequence of pseudorandom numbers.
double az_rand_sdouble(az_random_seed_t *seed);

/*===========================================================================*/

// Returns a random double from min (inclusive) to max (exclusive), using the
// global random seed.  Both min and max must be finite, and min must not be
// greater than max (for convenience, if min == max, min is returned; otherwise
// the result will be strictly less than max).
double az_random(double min, double max);

// Returns a random integer in the (inclusive) range [min, max], using the
// global random seed.  min must not be greater than max.
int az_randint(int min, int max);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_RANDOM_H_
