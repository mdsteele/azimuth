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
#include <string.h>

#include <SDL_filesystem.h>

#include "azimuth/util/rw.h"
#include "azimuth/util/string.h"

/*===========================================================================*/

#ifdef WIN32
#define _binary_resources_start binary_resources_start
#endif

// Keep this declaration in sync with generate_blob_index.sh
extern const struct resource_entry {
  const char *name;
  size_t offset, length;
} resource_index[];
extern const size_t resource_index_size;
extern const char _binary_resources_start[];

#if defined(__APPLE__)
// Use app-bundle resources on macOS
bool az_system_resource_reader(const char *name, az_reader_t *reader) {
  char *resource_dir = SDL_GetBasePath();
  if (resource_dir == NULL) return false;
  char *path = az_strprintf("%s/%s", resource_dir, name);
  SDL_free(resource_dir);
  const bool success = az_file_reader(path, reader);
  free(path);
  return success;
}
#else
// Comparison function for bsearch.
static int compare_resource_entries(const void *key_ptr,
                                    const void *value_ptr) {
  const char *name = key_ptr;
  const struct resource_entry *entry = value_ptr;
  return strcmp(name, entry->name);
}

bool az_system_resource_reader(const char *name, az_reader_t *reader) {
  struct resource_entry *entry =
    bsearch(name, resource_index, resource_index_size,
            sizeof(struct resource_entry), &compare_resource_entries);
  if (entry == NULL) return false;
  az_charbuf_reader(_binary_resources_start + entry->offset, entry->length,
                    reader);
  return true;
}
#endif

/*===========================================================================*/
