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

#pragma once
#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include <stdbool.h>

/*===========================================================================*/

#define RUN_TEST(fn) _run_test(#fn, fn)

#define EXPECT_TRUE(condition) do {             \
    _expect_true(!!(condition), #condition);    \
  } while (false)

#define EXPECT_FALSE(condition) EXPECT_TRUE(!(condition))

#define EXPECT_APPROX(expected, actual) do {                    \
    _expect_approx(expected, actual, #expected " == " #actual); \
  } while (false)

#define ASSERT_TRUE(condition) do {                             \
    if (!_expect_true(!!(condition), #condition)) return;       \
  } while (false)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define TESTS_EXIT_CODE() (_any_test_failed ? 1 : 0)

/*===========================================================================*/

extern bool _current_test_failed;
extern bool _any_test_failed;

void _run_test(const char *name, void (*function)(void));

bool _expect_true(bool condition, const char *message);

bool _expect_approx(double expected, double actual, const char *message);

/*===========================================================================*/

#endif // TEST_TEST_H_
