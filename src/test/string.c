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

#include "azimuth/util/string.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_strdup(void) {
  EXPECT_TRUE(az_strdup(NULL) == NULL);

  const char *str = "Hello, world!";
  char *copy = az_strdup(str);
  EXPECT_STRING_EQ(str, copy);
  free(copy);
}

void test_strprintf(void) {
  char *str = az_strprintf("Hello, %s number %.2f!", "purple world", AZ_PI);
  EXPECT_STRING_EQ("Hello, purple world number 3.14!", str);
  free(str);
}

/*===========================================================================*/
