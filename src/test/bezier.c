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

#include "azimuth/util/bezier.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_cubic_bezier_point(void) {
  const az_vector_t start = {2,  2};
  const az_vector_t ctrl1 = {4,  5};
  const az_vector_t ctrl2 = {6, -4};
  const az_vector_t end   = {7, -1};
  EXPECT_VAPPROX(start, az_cubic_bezier_point(start, ctrl1, ctrl2, end, 0.0));
  EXPECT_VAPPROX(((az_vector_t){3.484375, 2.375}),
                 az_cubic_bezier_point(start, ctrl1, ctrl2, end, 0.25));
  EXPECT_VAPPROX(((az_vector_t){4.875, 0.5}),
                 az_cubic_bezier_point(start, ctrl1, ctrl2, end, 0.5));
  EXPECT_VAPPROX(((az_vector_t){6.671, -1.564}),
                 az_cubic_bezier_point(start, ctrl1, ctrl2, end, 0.9));
  EXPECT_VAPPROX(end, az_cubic_bezier_point(start, ctrl1, ctrl2, end, 1.0));
}

void test_cubic_bezier_angle(void) {
  const az_vector_t start = {2,  2};
  const az_vector_t ctrl1 = {4,  5};
  const az_vector_t ctrl2 = {6, -4};
  const az_vector_t end   = {7, -1};
  EXPECT_APPROX(0.98279372324732905408,
                az_cubic_bezier_angle(start, ctrl1, ctrl2, end, 0.0));
  EXPECT_APPROX(-0.65880603611747623471,
                az_cubic_bezier_angle(start, ctrl1, ctrl2, end, 0.25));
  EXPECT_APPROX(-1.0427218783685370251,
                az_cubic_bezier_angle(start, ctrl1, ctrl2, end, 0.5));
  EXPECT_APPROX(0.61466295192216546805,
                az_cubic_bezier_angle(start, ctrl1, ctrl2, end, 0.9));
  EXPECT_APPROX(1.249045772398254428,
                az_cubic_bezier_angle(start, ctrl1, ctrl2, end, 1.0));
}

void test_cubic_bezier_arc_length(void) {
  const az_vector_t start = {2,  2};
  const az_vector_t ctrl1 = {4,  5};
  const az_vector_t ctrl2 = {6, -4};
  const az_vector_t end   = {7, -1};
  // The below expectations were calculated with num_steps=1e8, and we
  // are currently able to calculate close-enough values with a num_steps of
  // only 4600.  If the implementation is later changed to improve accuracy, we
  // can perhaps reduce the value of num_steps; however, an implementation
  // change that causes this to fail with this num_steps should be rejected.
  const int num_steps = 4600;
  EXPECT_APPROX(1.7873668985173249979, az_cubic_bezier_arc_length(
      start, ctrl1, ctrl2, end, num_steps, 0.0, 0.25));
  EXPECT_APPROX(2.3467717805730901048, az_cubic_bezier_arc_length(
      start, ctrl1, ctrl2, end, num_steps, 0.25, 0.5));
  EXPECT_APPROX(2.9164505887022889041, az_cubic_bezier_arc_length(
      start, ctrl1, ctrl2, end, num_steps, 0.5, 0.9));
  EXPECT_APPROX(0.66258911918009899544, az_cubic_bezier_arc_length(
      start, ctrl1, ctrl2, end, num_steps, 0.9, 1.0));
}

void test_cubic_bezier_arc_param(void) {
  const az_vector_t start = {2,  2};
  const az_vector_t ctrl1 = {4,  5};
  const az_vector_t ctrl2 = {6, -4};
  const az_vector_t end   = {7, -1};
  // The below expectations were calculated with num_steps=1e8, and with
  // num_steps=1e5 we are currently able to calculate values within 5e-6 of the
  // correct value.  If the implementation is later changed to improve
  // accuracy, we can perhaps reduce the value of threshold; however, an
  // implementation change that causes this to fail with this threshold should
  // be rejected.
  const int num_steps = 1e5;
  const double threshold = 5e-6;
  EXPECT_WITHIN(0.20855430999999999275, az_cubic_bezier_arc_param(
      start, ctrl1, ctrl2, end, num_steps, 0.0, 1.5), threshold);
  EXPECT_WITHIN(0.41811705999999998484, az_cubic_bezier_arc_param(
      start, ctrl1, ctrl2, end, num_steps, 0.25, 1.5), threshold);
  EXPECT_WITHIN(0.65293279500000001025, az_cubic_bezier_arc_param(
      start, ctrl1, ctrl2, end, num_steps, 0.5, 1.5), threshold);
  EXPECT_WITHIN(1.0, az_cubic_bezier_arc_param(
      start, ctrl1, ctrl2, end, num_steps, 0.9, 1.5), threshold);
}

/*===========================================================================*/
