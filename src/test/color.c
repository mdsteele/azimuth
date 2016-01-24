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

void test_color3f(void) {
  const az_color_t color0 = az_color3f(0, 0, 0);
  EXPECT_INT_EQ(  0, color0.r);
  EXPECT_INT_EQ(  0, color0.g);
  EXPECT_INT_EQ(  0, color0.b);
  EXPECT_INT_EQ(255, color0.a);

  const az_color_t color1 = az_color3f(1, 1, 1);
  EXPECT_INT_EQ(255, color1.r);
  EXPECT_INT_EQ(255, color1.g);
  EXPECT_INT_EQ(255, color1.b);
  EXPECT_INT_EQ(255, color1.a);

  const az_color_t color2 = az_color3f(0.25f, 0.5f, 0.75f);
  EXPECT_INT_EQ(63, color2.r);
  EXPECT_INT_EQ(127, color2.g);
  EXPECT_INT_EQ(191, color2.b);
  EXPECT_INT_EQ(255, color2.a);
}

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

void test_transition_color(void) {
  const az_color_t color0 = {0, 213, 255, 100};
  const az_color_t color1 = {255, 213, 0, 200};

  const az_color_t color_000 = az_transition_color(color0, color1, 0.00);
  EXPECT_INT_EQ(  0, color_000.r);
  EXPECT_INT_EQ(213, color_000.g);
  EXPECT_INT_EQ(255, color_000.b);
  EXPECT_INT_EQ(100, color_000.a);

  const az_color_t color_025 = az_transition_color(color0, color1, 0.25);
  EXPECT_INT_EQ( 63, color_025.r);
  EXPECT_INT_EQ(213, color_025.g);
  EXPECT_INT_EQ(191, color_025.b);
  EXPECT_INT_EQ(125, color_025.a);

  const az_color_t color_050 = az_transition_color(color0, color1, 0.50);
  EXPECT_INT_EQ(127, color_050.r);
  EXPECT_INT_EQ(213, color_050.g);
  EXPECT_INT_EQ(127, color_050.b);
  EXPECT_INT_EQ(150, color_050.a);

  const az_color_t color_100 = az_transition_color(color0, color1, 1.00);
  EXPECT_INT_EQ(255, color_100.r);
  EXPECT_INT_EQ(213, color_100.g);
  EXPECT_INT_EQ(  0, color_100.b);
  EXPECT_INT_EQ(200, color_100.a);
}

/*===========================================================================*/
