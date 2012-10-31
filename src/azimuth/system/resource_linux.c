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

#include <errno.h>
#include <linux/limits.h> // for PATH_MAX
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>

/*===========================================================================*/

const char *az_get_app_data_directory(void) {
  static char path_buffer[PATH_MAX];
  if (path_buffer[0] == '\0') {
    // First, do tilde-expansion so we get a path in the user's homedir.
    wordexp_t words;
    wordexp("~/.azimuth-game", &words, 0);
    if (words.we_wordc < 1) {
      wordfree(&words);
      return NULL;
    }
    strncpy(path_buffer, words.we_wordv[0], sizeof(path_buffer) - 1);
    wordfree(&words);
    struct stat stat_buffer;
    // Try to stat the desired path.
    if (stat(path_buffer, &stat_buffer) == 0) {
      // If the path exists but isn't a directory, we fail.
      if (!S_ISDIR(stat_buffer.st_mode)) {
        path_buffer[0] = '\0';
        return NULL;
      }
    }
    // If the directory doesn't exist, try to create it.  If we can't create
    // it, or if stat failed for some other reason, we fail.
    else if (errno != ENOENT || mkdir(path_buffer, 0700) != 0) {
      path_buffer[0] = '\0';
      return NULL;
    }
  }
  return path_buffer;
}

const char *az_get_resource_directory(void) {
  static char path_buffer[PATH_MAX];
  if (path_buffer[0] == '\0') {
    int len = readlink("/proc/self/exe", path_buffer, sizeof(path_buffer) - 1);
    if (len < 0) {
      path_buffer[0] = '\0';
      return NULL;
    }
    while (--len > 0 && path_buffer[len] != '/');
    path_buffer[len] = '\0';
  }
  return path_buffer;
}

/*===========================================================================*/
