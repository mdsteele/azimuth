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

#include "azimuth/util/random.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

/*===========================================================================*/

// A simple random number generator, based on the MWC algorithm from George
// Marsaglia's post to sci.stat.math on 12 Jan 1999, which can be found here:
//   https://groups.google.com/forum/#!topic/sci.stat.math/5yb0jwf1stw
uint32_t az_rand_uint32(az_random_seed_t *seed) {
  seed->z = 36969u * (seed->z & 0xffff) + (seed->z >> 16);
  seed->w = 18000u * (seed->w & 0xffff) + (seed->w >> 16);
  return (seed->z << 16) | (seed->w & 0xffff);
}

double az_rand_udouble(az_random_seed_t *seed) {
  return az_rand_uint32(seed) * 2.3283064365386963e-10;
}

double az_rand_sdouble(az_random_seed_t *seed) {
  return az_rand_uint32(seed) * 4.656612873077393e-10 - 1.0;
}

/*===========================================================================*/

static az_random_seed_t global_seed = {1, 1};

double az_random(double min, double max) {
  assert(isfinite(min));
  assert(isfinite(max));
  assert(min <= max);
  if (min == max) return min;
  return min + (max - min) * az_rand_udouble(&global_seed);
}

int az_randint(int min, int max) {
  assert(min <= max);
  if (min == max) return min;
  // This will result in a nonuniform distribution when (1 + max - min) is not
  // a power of two, but it's good enough for our purposes.
  return min + az_rand_uint32(&global_seed) % (uint32_t)(1 + max - min);
}

/*===========================================================================*/
