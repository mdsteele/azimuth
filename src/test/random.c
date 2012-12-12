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

#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "test/test.h"

/*===========================================================================*/

void test_random(void) {
  int counts[10] = {0};
  for (int i = 0; i < 10000; ++i) {
    double r = az_random(0.0, 1.0);
    ASSERT_TRUE(r >= 0.0);
    ASSERT_TRUE(r < 1.0);
    int index = (int)(r * 10);
    ASSERT_TRUE(0 <= index && index < AZ_ARRAY_SIZE(counts));
    ++counts[index];
  }
  AZ_ARRAY_LOOP(count, counts) {
    // These could fail even for correct code, of course, but it's somewhat
    // unlikely.
    EXPECT_TRUE(*count >= 200);
    EXPECT_TRUE(*count <= 5000);
  }
}

void test_randint(void) {
  int counts[13] = {0};
  for (int i = 0; i < 13000; ++i) {
    int r = az_randint(-7, 5);
    ASSERT_TRUE(r >= -7);
    ASSERT_TRUE(r <= 5);
    ++counts[r + 7];
  }
  AZ_ARRAY_LOOP(count, counts) {
    // These could fail even for correct code, of course, but it's somewhat
    // unlikely.
    EXPECT_TRUE(*count >= 200);
    EXPECT_TRUE(*count <= 5000);
  }
}

/*===========================================================================*/
