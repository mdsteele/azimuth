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

#include "azimuth/util/vector.h"

/*===========================================================================*/

#define RUN_TEST(fn) do { extern void fn(void); _run_test(#fn, fn); } while (0)

// Test that the condition is true.
#define EXPECT_TRUE(condition) _expect_true(condition, #condition)

// Test that the condition is false.
#define EXPECT_FALSE(condition) EXPECT_TRUE(!(condition))

// Test that the two doubles are (approximately) equal.
#define EXPECT_APPROX(expected, actual) \
  _expect_approx(expected, actual, #expected " == " #actual)

// Test that the two vectors are (approximately) equal.
#define EXPECT_VAPPROX(expected, actual) \
  _expect_vapprox(expected, actual, #expected " == " #actual)

// Test that the two ints are exactly equal.
#define EXPECT_INT_EQ(expected, actual) \
  _expect_int_eq(expected, actual, #expected " == " #actual)

// Test that the two NUL-terminated strings are exactly equal.
#define EXPECT_STRING_EQ(expected, actual) \
  _expect_string_eq(expected, actual, #actual)

// Test that the actual char-array (with explicit length) is equal to the
// expected (NUL-terminated) string.
#define EXPECT_STRING_N_EQ(expected, actual, actual_length) \
  _expect_string_n_eq(expected, actual, actual_length, #actual)

// Test that the condition is true, and return immediately if not.
#define ASSERT_TRUE(condition) do { \
    if (!_expect_true(condition, #condition)) return; \
  } while (false)

// Test that the condition is false, and return immediately if not.
#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

// Test that the two doubles are (approximately) equal, and return immediately
// if not.
#define ASSERT_APPROX(expected, actual) do { \
    if (!_expect_approx(expected, actual, #expected " == " #actual)) return; \
  } while (false)

int final_test_summary(void);

/*===========================================================================*/

extern bool _current_test_failed;
extern unsigned int _num_tests_failed;

void _run_test(const char *name, void (*function)(void));

bool _expect_true(bool condition, const char *message);

bool _expect_approx(double expected, double actual, const char *message);

bool _expect_vapprox(az_vector_t expected, az_vector_t actual,
                     const char *message);

bool _expect_int_eq(int expected, int actual, const char *message);

bool _expect_string_eq(const char *expected, const char *actual,
                       const char *message);

bool _expect_string_n_eq(const char *expected, const char *actual,
                         int actual_length, const char *message);

/*===========================================================================*/

#endif // TEST_TEST_H_
