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

#include "azimuth/util/key.h"
#include "test/test.h"

/*===========================================================================*/

void test_is_number_key(void) {
  EXPECT_TRUE(az_is_number_key(AZ_KEY_0));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_1));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_2));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_3));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_4));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_5));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_6));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_7));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_8));
  EXPECT_TRUE(az_is_number_key(AZ_KEY_9));
  EXPECT_FALSE(az_is_number_key(AZ_KEY_A));
  EXPECT_FALSE(az_is_number_key(AZ_KEY_Z));
  EXPECT_FALSE(az_is_number_key(AZ_KEY_LEFT_ARROW));
  EXPECT_FALSE(az_is_number_key(AZ_KEY_UNKNOWN));
}

/*===========================================================================*/
