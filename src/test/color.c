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

#include "azimuth/util/color.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_hsva_color(void) {
  const az_color_t color0 = az_hsva_color(0, 0, 0, 0);
  EXPECT_INT_EQ(  0, color0.r);
  EXPECT_INT_EQ(  0, color0.g);
  EXPECT_INT_EQ(  0, color0.b);
  EXPECT_INT_EQ(  0, color0.a);

  const az_color_t color1 = az_hsva_color(0.0, 1, 1, 1);
  EXPECT_INT_EQ(255, color1.r);
  EXPECT_INT_EQ(  0, color1.g);
  EXPECT_INT_EQ(  0, color1.b);
  EXPECT_INT_EQ(255, color1.a);

  const az_color_t color2 = az_hsva_color(AZ_PI, 1, 0.5, 0.25);
  EXPECT_INT_EQ(  0, color2.r);
  EXPECT_INT_EQ(127, color2.g);
  EXPECT_INT_EQ(127, color2.b);
  EXPECT_INT_EQ( 63, color2.a);

  const az_color_t color3 = az_hsva_color(-1.3, 0.42, 0.71, 0.39);
  EXPECT_INT_EQ(162, color3.r);
  EXPECT_INT_EQ(105, color3.g);
  EXPECT_INT_EQ(181, color3.b);
  EXPECT_INT_EQ( 99, color3.a);
}

/*===========================================================================*/
