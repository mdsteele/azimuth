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
#include <stdlib.h>
#include <time.h>

/*===========================================================================*/

void az_init_random(void) {
  srand(time(NULL));
}

double az_random(void) {
  // This is a terrible, terrible implementation of random numbers.
  // TODO: Replace this with something better.
  return ((double)rand()) / (1.0 + (double)RAND_MAX);
}

int az_randint(int min, int max) {
  assert(min <= max);
  if (min == max) return min;
  // This is a terrible, terrible implementation of random numbers (just for
  // starters, it's non-uniform), but it's good enough for us for now.
  // TODO: Replace this with something better.
  return min + rand() % (1 + max - min);
}

/*===========================================================================*/
