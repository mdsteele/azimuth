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

#include "test/util.h"

#include "azimuth/util.h"
#include "test/test.h"

/*===========================================================================*/

void test_clock_mod(void) {
  EXPECT_TRUE(az_clock_mod(5, 3,  0) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3,  1) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3,  2) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3,  3) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3,  4) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3,  5) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3,  6) == 2);
  EXPECT_TRUE(az_clock_mod(5, 3,  7) == 2);
  EXPECT_TRUE(az_clock_mod(5, 3,  8) == 2);
  EXPECT_TRUE(az_clock_mod(5, 3,  9) == 3);
  EXPECT_TRUE(az_clock_mod(5, 3, 10) == 3);
  EXPECT_TRUE(az_clock_mod(5, 3, 11) == 3);
  EXPECT_TRUE(az_clock_mod(5, 3, 12) == 4);
  EXPECT_TRUE(az_clock_mod(5, 3, 13) == 4);
  EXPECT_TRUE(az_clock_mod(5, 3, 14) == 4);
  EXPECT_TRUE(az_clock_mod(5, 3, 15) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3, 16) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3, 17) == 0);
  EXPECT_TRUE(az_clock_mod(5, 3, 18) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3, 19) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3, 20) == 1);
  EXPECT_TRUE(az_clock_mod(5, 3, 21) == 2);
}

void test_clock_zigzag(void) {
  EXPECT_TRUE(az_clock_zigzag(5, 3,  0) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  1) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  2) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  3) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  4) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  5) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  6) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  7) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  8) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3,  9) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 10) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 11) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 12) == 4);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 13) == 4);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 14) == 4);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 15) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 16) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 17) == 3);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 18) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 19) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 20) == 2);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 21) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 22) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 23) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 24) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 25) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 26) == 0);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 27) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 28) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 29) == 1);
  EXPECT_TRUE(az_clock_zigzag(5, 3, 30) == 2);
}

/*===========================================================================*/
