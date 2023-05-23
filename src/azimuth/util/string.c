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

#include "azimuth/util/string.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "azimuth/util/misc.h"

/*===========================================================================*/

char *az_strdup(const char *str) {
  if (str == NULL) return NULL;
  char *copy = AZ_ALLOC(strlen(str) + 1, char); // add 1 for trailing '\0'
  strcpy(copy, str);
  return copy;
}

char *az_strprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  const size_t size = vsnprintf(NULL, 0, format, args);
  va_end(args);
  char *out = AZ_ALLOC(size + 1, char); // add 1 for trailing '\0'
  va_start(args, format);
  vsnprintf(out, size + 1, format, args);
  va_end(args);
  return out;
}

/*===========================================================================*/
