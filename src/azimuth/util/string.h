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
#ifndef AZIMUTH_UTIL_STRING_H_
#define AZIMUTH_UTIL_STRING_H_

/*===========================================================================*/

// Allocates and returns a copy of the given NUL-terminated string.  If memory
// allocation fails, this will signal a fatal error and exit the program.
// Returns NULL if the argument is NULL.
char *az_strdup(const char *str)
  __attribute__((__malloc__));

// Allocates a new string with the same text that would be produced by a call
// to printf with the same format and arguments.  If memory allocation fails,
// this will signal a fatal error and exit the program.
char *az_strprintf(const char *format, ...)
  __attribute__((__malloc__,__format__(__printf__,1,2)));

/*===========================================================================*/

#endif // AZIMUTH_UTIL_STRING_H_
