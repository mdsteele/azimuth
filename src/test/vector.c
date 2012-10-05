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

#include "test/vector.h"

#include <math.h>

#include "azimuth/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_mod2pi(void) {
  for (double t = -20.0; t < 20.0; t += 0.1) {
    const double t2 = az_mod2pi(t);
    EXPECT_TRUE(t2 >= -AZ_PI);
    EXPECT_TRUE(t2 <= AZ_PI);
    EXPECT_APPROX(sin(t), sin(t2));
    EXPECT_APPROX(cos(t), cos(t2));
  }
}

void test_vector_polar(void) {
  const double m = 2342.2908;
  const double t = 4.3901;
  const az_vector_t v = az_vpolar(m, t);
  EXPECT_APPROX(m, az_vnorm(v));
  EXPECT_APPROX(az_mod2pi(t), az_vtheta(v));
}

/*===========================================================================*/
