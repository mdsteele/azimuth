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

#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFURL.h>

/*===========================================================================*/

const char *az_get_resource_directory(void) {
  static unsigned char path[1000];
  if (path[0] == '\0') {
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
    if (!CFURLGetFileSystemRepresentation(url, true, path, sizeof(path))) {
      path[0] = '\0';
      return NULL;
    }
  }
  return (char *)path;
}

/*===========================================================================*/
