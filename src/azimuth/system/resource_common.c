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

#include "azimuth/system/resource.h"

#include <stdbool.h>
#include <stdlib.h>

#include "azimuth/util/rw.h"
#include "azimuth/util/string.h"

/*===========================================================================*/

bool az_system_resource_reader(const char *name, az_reader_t *reader) {
  const char *resource_dir = az_get_resource_directory();
  if (resource_dir == NULL) return false;
  char *path = az_strprintf("%s/%s", resource_dir, name);
  const bool success = az_file_reader(path, reader);
  free(path);
  return success;
}

/*===========================================================================*/
