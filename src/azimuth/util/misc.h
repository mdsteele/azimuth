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
#ifndef AZIMUTH_UTIL_MISC_H_
#define AZIMUTH_UTIL_MISC_H_

#include <string.h> // for size_t

/*===========================================================================*/

// Get the length of a statically-sized array, as a compile-time constant.
#define AZ_ARRAY_SIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / !(sizeof(a) % sizeof(*(a))))

// Loop over a statically-sized array.
#define AZ_ARRAY_LOOP(var_name, array) \
  for (__typeof__(&*(array)) var_name = (array); \
       var_name != (array) + AZ_ARRAY_SIZE(array); ++var_name)

// Signal a fatal error and exit the program.
#define AZ_FATAL(...) _az_fatal(__func__, __VA_ARGS__)
void _az_fatal(const char *funcname, const char *format, ...)
  __attribute__((__format__(__printf__,2,3),__noreturn__));

// Allocate a block of memory large enough to fit a type[n] array.  If you only
// want to allocate a single object rather than an array, simply use n=1.  This
// will never return NULL; instead, if memory allocation fails, this will
// signal a fatal error and exit the program.
#define AZ_ALLOC(n, type) ((type *)_az_alloc(__func__, (n), sizeof(type)))
void *_az_alloc(const char *funcname, size_t n, size_t size)
  __attribute__((__malloc__));

// Use this macro to indicate that this point in the code should never be
// reached at runtime.  If it is reached anyway (presumably due to a bug), it
// will terminate the program with a fatal error.
// TODO: Consider using __builtin_unreachable or somesuch here for NDEBUG.
#define AZ_ASSERT_UNREACHABLE() AZ_FATAL("line %d: unreachable\n", __LINE__)

// Use this macro to check at compile time that a (compile-time-constant)
// expression is true.  It is legal at top-level or within a function.
#define AZ_STATIC_ASSERT(condition) \
  typedef int AZ_SA_JOIN(az_static_assertion_, __LINE__)[(condition) ? 1 : -1]
// Helper macros for AZ_STATIC_ASSERT:
#define AZ_SA_JOIN(a, b) AZ_SA_JOIN2(a, b)
#define AZ_SA_JOIN2(a, b) a##b

/*===========================================================================*/

#endif // AZIMUTH_UTIL_MISC_H_
