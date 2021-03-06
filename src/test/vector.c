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

#include <math.h>

#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "test/test.h"

/*===========================================================================*/

void test_modulo(void) {
  EXPECT_INT_EQ(3, az_modulo(10, 7));
  EXPECT_INT_EQ(4, az_modulo(-10, 7));
  EXPECT_INT_EQ(-4, az_modulo(10, -7));
  EXPECT_INT_EQ(-3, az_modulo(-10, -7));

  EXPECT_INT_EQ(0, az_modulo(0, 7));
  EXPECT_INT_EQ(0, az_modulo(0, -7));
  EXPECT_INT_EQ(0, az_modulo(14, 7));
  EXPECT_INT_EQ(0, az_modulo(14, -7));
  EXPECT_INT_EQ(0, az_modulo(-14, 7));
  EXPECT_INT_EQ(0, az_modulo(-14, -7));
}

void test_signmod(void) {
  EXPECT_APPROX( 3.0, az_signmod( 10.0,  7.0,  1.0));
  EXPECT_APPROX(-4.0, az_signmod( 10.0,  7.0, -1.0));
  EXPECT_APPROX( 4.0, az_signmod(-10.0,  7.0,  1.0));
  EXPECT_APPROX(-3.0, az_signmod(-10.0,  7.0, -1.0));
  EXPECT_APPROX( 3.0, az_signmod( 10.0, -7.0,  1.0));
  EXPECT_APPROX(-4.0, az_signmod( 10.0, -7.0, -1.0));
  EXPECT_APPROX( 4.0, az_signmod(-10.0, -7.0,  1.0));
  EXPECT_APPROX(-3.0, az_signmod(-10.0, -7.0, -1.0));

  EXPECT_APPROX(0.0, az_signmod(  0.0,  7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod(  0.0,  7.0, -1.0));
  EXPECT_APPROX(0.0, az_signmod(  0.0, -7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod(  0.0, -7.0, -1.0));
  EXPECT_APPROX(0.0, az_signmod( 14.0,  7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod( 14.0,  7.0, -1.0));
  EXPECT_APPROX(0.0, az_signmod( 14.0, -7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod( 14.0, -7.0, -1.0));
  EXPECT_APPROX(0.0, az_signmod(-14.0,  7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod(-14.0,  7.0, -1.0));
  EXPECT_APPROX(0.0, az_signmod(-14.0, -7.0,  1.0));
  EXPECT_APPROX(0.0, az_signmod(-14.0, -7.0, -1.0));
}

void test_mod2pi(void) {
  for (double t = -20.0; t < 20.0; t += 0.1) {
    const double t2 = az_mod2pi(t);
    EXPECT_TRUE(t2 >= -AZ_PI);
    EXPECT_TRUE(t2 <= AZ_PI);
    EXPECT_APPROX(sin(t), sin(t2));
    EXPECT_APPROX(cos(t), cos(t2));
  }
  for (double t = -20.0; t < 20.0; t += 0.1) {
    const double t2 = az_mod2pi_nonneg(t);
    EXPECT_TRUE(t2 >= 0.0);
    EXPECT_TRUE(t2 <= AZ_TWO_PI);
    EXPECT_APPROX(sin(t), sin(t2));
    EXPECT_APPROX(cos(t), cos(t2));
  }
  for (double t = -20.0; t < 20.0; t += 0.1) {
    const double t2 = az_mod2pi_nonpos(t);
    EXPECT_TRUE(t2 >= -AZ_TWO_PI);
    EXPECT_TRUE(t2 <= 0.0);
    EXPECT_APPROX(sin(t), sin(t2));
    EXPECT_APPROX(cos(t), cos(t2));
  }
}

void test_vpolar(void) {
  const double m = 2342.2908;
  const double t = 4.3901;
  const az_vector_t v = az_vpolar(m, t);
  EXPECT_APPROX(m, az_vnorm(v));
  EXPECT_APPROX(az_mod2pi(t), az_vtheta(v));
}

void test_vproj(void) {
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t vec = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    EXPECT_VAPPROX(AZ_VZERO, az_vproj(AZ_VZERO, vec));
    EXPECT_VAPPROX(AZ_VZERO, az_vflatten(AZ_VZERO, vec));
    EXPECT_VAPPROX(AZ_VZERO, az_vproj(vec, AZ_VZERO));
    EXPECT_VAPPROX(vec, az_vflatten(vec, AZ_VZERO));
    const az_vector_t vec2 = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    EXPECT_APPROX(0, az_vcross(az_vproj(vec, vec2), vec2));
    EXPECT_APPROX(0, az_vdot(az_vflatten(vec, vec2), vec2));
    RETURN_IF_FAILED();
  }
}

void test_vreflect(void) {
  EXPECT_VAPPROX(((az_vector_t){5, -2}),
                 az_vreflect((az_vector_t){5, 2}, (az_vector_t){1, 0}));
  EXPECT_VAPPROX(((az_vector_t){-5, 2}),
                 az_vreflect((az_vector_t){5, 2}, (az_vector_t){0, 1}));
  EXPECT_VAPPROX(((az_vector_t){2, 5}),
                 az_vreflect((az_vector_t){5, 2}, (az_vector_t){1, 1}));
}

void test_vrotate(void) {
  EXPECT_VAPPROX(AZ_VZERO, az_vrotate(AZ_VZERO, 47.3));
  EXPECT_VAPPROX(((az_vector_t){-1, 1}),
                 az_vrotate((az_vector_t){1, 1}, AZ_HALF_PI));
  EXPECT_VAPPROX(((az_vector_t){1.0, sqrt(3)}),
                 az_vrotate((az_vector_t){-1, sqrt(3)}, AZ_DEG2RAD(-60.0)));
}

void test_vunit(void) {
  EXPECT_APPROX(1.0, az_vnorm(az_vunit(AZ_VZERO)));
  EXPECT_APPROX(az_vtheta(AZ_VZERO), az_vtheta(az_vunit(AZ_VZERO)));
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t vec = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    ASSERT_APPROX(1.0, az_vnorm(az_vunit(vec)));
    ASSERT_APPROX(az_vtheta(vec), az_vtheta(az_vunit(vec)));
  }
}

void test_vwithlen(void) {
  EXPECT_APPROX(3.0, az_vnorm(az_vwithlen(AZ_VZERO, 3.0)));
  EXPECT_APPROX(0.0, az_vtheta(az_vwithlen(AZ_VZERO, 3.0)));
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t v1 = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    const double length = az_random(0.0, 1.3);
    const az_vector_t v2 = az_vwithlen(v1, length);
    ASSERT_APPROX(length, az_vnorm(v2));
    ASSERT_APPROX(az_vtheta(v1), az_vtheta(v2));
  }
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t v1 = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    const double length = az_random(0.0, 1.3);
    const az_vector_t v2 = az_vwithlen(v1, -length);
    ASSERT_APPROX(length, az_vnorm(v2));
    ASSERT_APPROX(az_vtheta(v1), az_mod2pi(az_vtheta(v2) + AZ_PI));
  }
}

void test_vcaplen(void) {
  EXPECT_APPROX(0.0, az_vnorm(az_vcaplen(AZ_VZERO, 3.0)));
  EXPECT_APPROX(0.0, az_vnorm(az_vcaplen(az_vpolar(3, 3), 0)));
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t v1 = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    const double length = az_random(0.0, 1.3);
    const az_vector_t v2 = az_vcaplen(v1, length);
    ASSERT_APPROX(fmin(length, az_vnorm(v1)), az_vnorm(v2));
    ASSERT_APPROX(az_vtheta(v1), az_vtheta(v2));
  }
}

void test_vaddlen(void) {
  EXPECT_APPROX(3.0, az_vnorm(az_vaddlen(AZ_VZERO, 3.0)));
  EXPECT_APPROX(0.0, az_vtheta(az_vaddlen(AZ_VZERO, 3.0)));
  EXPECT_APPROX(4.0, az_vnorm(az_vaddlen(AZ_VZERO, -4.0)));
  EXPECT_APPROX(AZ_PI, az_vtheta(az_vaddlen(AZ_VZERO, -4.0)));
  for (int i = 0; i < 1000; ++i) {
    const az_vector_t v1 = {az_random(-1.5, 1.5), az_random(-1.5, 1.5)};
    const double length = az_random(0.0, 1.3);
    const az_vector_t v2 = az_vaddlen(v1, length);
    ASSERT_APPROX((length + az_vnorm(v1)), az_vnorm(v2));
    ASSERT_APPROX(az_vtheta(v1), az_vtheta(v2));
  }
}

/*===========================================================================*/
