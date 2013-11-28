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

#include "azimuth/util/clock.h"

#include <assert.h>

/*===========================================================================*/

int az_clock_mod(int modulus, int slowdown, az_clock_t clock) {
  assert(modulus >= 1);
  assert(slowdown >= 1);
  return (clock % (modulus * slowdown)) / slowdown;
}

int az_clock_zigzag(int modulus, int slowdown, az_clock_t clock) {
  assert(modulus >= 2);
  assert(slowdown >= 1);
  const int m = modulus - 1;
  const int d = az_clock_mod(2 * m, slowdown, clock);
  return (d <= m ? d : 2 * m - d);
}

/*===========================================================================*/
