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

#include <stdlib.h>
#include <string.h>

// require Windows Vista or later
#if _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#define INITGUID
#include <windows.h>
#include <knownfolders.h>
#include <objbase.h>
#include <shlobj.h>

/*===========================================================================*/

const char *az_get_app_data_directory(void) {
  static char path_buffer[MAX_PATH];
  if (path_buffer[0] == '\0') {
    PWSTR path = NULL;
    const HRESULT result = SHGetKnownFolderPath(&FOLDERID_RoamingAppData,
                                                KF_FLAG_CREATE, NULL, &path);
    if (FAILED(result)) return NULL;
    const size_t length = wcstombs(path_buffer, path, MAX_PATH - 1);
    CoTaskMemFree(path);
    const char suffix[] = "\\Azimuth";
    if (length > MAX_PATH - sizeof(suffix)) {
      path_buffer[0] = '\0';
      return NULL;
    }
    strcpy(path_buffer + length, suffix);
  }
  return path_buffer;
}

/*===========================================================================*/
