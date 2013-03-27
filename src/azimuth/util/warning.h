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
#ifndef AZIMUTH_UTIL_WARNING_H_
#define AZIMUTH_UTIL_WARNING_H_

#include <stdbool.h>

/*===========================================================================*/

// In debug mode, print a warning message, but only the first time that this
// line is executed.  Do nothing in release mode.
#ifdef NDEBUG
#define AZ_WARNING_ONCE(...)
#else
#define AZ_WARNING_ONCE(...) do { \
    static bool AZ_JOIN(_az_warn_, __LINE__) = false; \
    if (!AZ_JOIN(_az_warn_, __LINE__)) { \
      _az_print_warning(__func__, __FILE__, __LINE__, __VA_ARGS__); \
      AZ_JOIN(_az_warn_, __LINE__) = true; \
    } \
  } while (false)
#endif

void _az_print_warning(const char *funcname, const char *filename,
                       int line, const char *format, ...)
  __attribute__((__format__(__printf__,4,5)));

/*===========================================================================*/

#endif // AZIMUTH_UTIL_WARNING_H_
