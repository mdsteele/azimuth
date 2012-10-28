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
#ifndef AZIMUTH_UTIL_CLOCK_H_
#define AZIMUTH_UTIL_CLOCK_H_

/*===========================================================================*/

typedef unsigned long az_clock_t;

// Return a number that cycles from zero up to (modulus - 1), with the number
// advancing by one for every `slowdown` ticks of the clock.
unsigned int az_clock_mod(unsigned int modulus, unsigned int slowdown,
                          az_clock_t clock);

// Return a number that cycles from zero up to (modulus - 1) and back down
// again, with the number advancing by one every `slowdown` ticks of the clock.
unsigned int az_clock_zigzag(unsigned int modulus, unsigned int slowdown,
                             az_clock_t clock);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_CLOCK_H_
