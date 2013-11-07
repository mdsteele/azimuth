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

#include "azimuth/state/camera.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_position_visible(void) {
  const az_camera_bounds_t bounds = {
    .min_r = 10000.0, .r_span = 500.0,
    .min_theta = AZ_DEG2RAD(90), .theta_span = AZ_DEG2RAD(45)
  };
  // The center should of course be visible.
  EXPECT_TRUE(az_position_visible(&bounds, az_bounds_center(&bounds)));
  // Check points just inside and just outside the r limits.
  EXPECT_TRUE(az_position_visible(&bounds, (az_vector_t){-1, 9761}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){-1, 9750}));
  EXPECT_TRUE(az_position_visible(&bounds, (az_vector_t){-1, 10739}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){-1, 10741}));
  // Check points inside and outside the theta limits.
  EXPECT_TRUE(az_position_visible(
      &bounds, az_vpolar(10250, AZ_DEG2RAD(134))));
  EXPECT_FALSE(az_position_visible(
      &bounds, az_vpolar(10250, AZ_DEG2RAD(-90))));
  // Check points inside and outside the endcaps.
  EXPECT_TRUE(az_position_visible(&bounds, (az_vector_t){319, 10739}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){321, 10739}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){319, 10741}));
  EXPECT_TRUE(az_position_visible(&bounds, (az_vector_t){319, 9761}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){321, 9761}));
  EXPECT_FALSE(az_position_visible(&bounds, (az_vector_t){319, 9759}));
  EXPECT_TRUE(az_position_visible(&bounds, az_vrotate(
      (az_vector_t){10250, 319}, AZ_DEG2RAD(135))));
  EXPECT_FALSE(az_position_visible(&bounds, az_vrotate(
      (az_vector_t){10250, 321}, AZ_DEG2RAD(135))));
}

/*===========================================================================*/
