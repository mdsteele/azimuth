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

/*===========================================================================*/

// Call this once, at startup, to initialize the random number generator.
void az_init_random(void);

// Return a random double from min (inclusive) to max (exclusive).  max must
// be greater than min, and both must be finite.
double az_random(double min, double max);

// Return a random integer in the (inclusive) range [min, max].  min must not
// be greater than max.
int az_randint(int min, int max);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_RANDOM_H_
