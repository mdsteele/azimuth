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

#include "azimuth/util/warning.h"

#include <stdarg.h>
#include <stdio.h>

/*===========================================================================*/

void az_print_warning_(const char *funcname, const char *filename,
                       int line, const char *format, ...) {
  va_list args;
  fprintf(stderr, "WARNING: function %s, file %s, line %d: ",
          funcname, filename, line);
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

/*===========================================================================*/
