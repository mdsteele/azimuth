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
#ifndef AZIMUTH_UTIL_H_
#define AZIMUTH_UTIL_H_

/*===========================================================================*/

// Get the length of a statically-sized array, as a compile-time constant.
#define AZ_ARRAY_SIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / !(sizeof(a) % sizeof(*(a))))

// Loop over a statically-sized array.
#define AZ_ARRAY_LOOP(var_name, array) \
  for (__typeof__(&*(array)) var_name = (array); \
       var_name != (array) + AZ_ARRAY_SIZE(array); ++var_name)

// Return a number that cycles from zero up to (modulus - 1), with the number
// advancing by one for every `slowdown` ticks of the clock.
unsigned long az_clock_mod(unsigned int modulus, unsigned int slowdown,
                           unsigned long clock);

// Return a number that cycles from zero up to (modulus - 1) and back down
// again, with the number advancing by one every `slowdown` ticks of the clock.
unsigned long az_clock_zigzag(unsigned int modulus, unsigned int slowdown,
                              unsigned long clock);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_H_
