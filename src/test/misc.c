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

#include <stdlib.h>

#include "azimuth/util/misc.h"
#include "test/test.h"

/*===========================================================================*/

void test_array_size(void) {
  short a[3];
  double b[7];
  struct { int x; char y; } c[11];
  EXPECT_INT_EQ(3, AZ_ARRAY_SIZE(a));
  EXPECT_INT_EQ(7, AZ_ARRAY_SIZE(b));
  EXPECT_INT_EQ(11, AZ_ARRAY_SIZE(c));
}

void test_zero_array(void) {
  int a = 0x5bababab;
  int b[13] = {0};
  int c = 0x5dcdcdcd;
  for (int i = 0; i < 13; ++i) {
    b[i] = 5 + i * i;
  }
  AZ_ZERO_ARRAY(b);
  EXPECT_INT_EQ(0x5bababab, a);
  EXPECT_INT_EQ(0x5dcdcdcd, c);
  for (int i = 0; i < AZ_ARRAY_SIZE(b); ++i) {
    EXPECT_INT_EQ(0, b[i]);
  }
}

void test_alloc(void) {
  // Try a small allocation.  The memory should be zeroed.
  const int length1 = 17;
  double *array1 = AZ_ALLOC(length1, double);
  for (int i = 0; i < length1; ++i) {
    EXPECT_TRUE(array1[i] == 0.0);
  }
  free(array1);

  // Try a larger allocation.  The memory should be zeroed.
  const int length2 = 20000;
  int *array2 = AZ_ALLOC(length2, int);
  for (int i = 0; i < length2; ++i) {
    EXPECT_INT_EQ(0, array2[i]);
  }
  free(array2);
}

// Test out some static assertions:
AZ_STATIC_ASSERT(1 + 2 == 3);
AZ_STATIC_ASSERT(sizeof(char) == 1);
AZ_STATIC_ASSERT((intptr_t)NULL == 0);

/*===========================================================================*/
