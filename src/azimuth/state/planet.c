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

#include "azimuth/state/planet.h"

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/constants.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/room.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

// Arbitrary limit to enforce sanity:
#define AZ_MAX_NUM_TEXTS 50000

typedef struct {
  FILE *file;
  bool success;
  jmp_buf jump;
  int num_zones, num_texts;
  az_planet_t *planet;
} az_load_planet_t;

#ifdef NDEBUG
#define FAIL() longjmp(loader->jump, 1)
#else
#define FAIL() do{ \
    fprintf(stderr, "planet.c: failure at line %d\n", __LINE__); \
    longjmp(loader->jump, 1); \
  } while (0)
#endif // NDEBUG

// Read the next non-whitespace character.  If it is '!' or if we reach EOF,
// do nothing more; otherwise, fail parsing.
static void scan_to_bang(az_load_planet_t *loader) {
  char ch;
  if (fscanf(loader->file, " %c", &ch) < 1) return;
  if (ch != '!') FAIL();
}

static char *scan_string(az_load_planet_t *loader) {
  if (fgetc(loader->file) != '"') FAIL();
  fpos_t pos;
  if (fgetpos(loader->file, &pos) != 0) FAIL();
  size_t length = 0;
  while (true) {
    const int ch = fgetc(loader->file);
    if (ch == EOF) FAIL();
    else if (ch == '"') break;
    else if (ch == '\\') fgetc(loader->file);
    ++length;
  }
  if (fsetpos(loader->file, &pos) != 0) FAIL();
  char *string = AZ_ALLOC(length + 1, char);
  for (size_t i = 0; i < length; ++i) {
    const int ch = fgetc(loader->file);
    if (ch == EOF) FAIL();
    string[i] = (ch == '\\' ? fgetc(loader->file) : ch);
  }
  assert(string[length] == '\0');
  if (fgetc(loader->file) != '"') {
    free(string);
    FAIL();
  }
  return string;
}

static void parse_planet_header(az_load_planet_t *loader) {
  int num_zones, num_rooms, num_texts, start_room_num;
  double start_x, start_y, start_angle;
  if (fscanf(loader->file, "@P z%d r%d t%d s%d x%lf y%lf a%lf\n",
             &num_zones, &num_rooms, &num_texts, &start_room_num,
             &start_x, &start_y, &start_angle) < 7) FAIL();
  if (num_zones < 1 || num_zones > num_rooms ||
      num_rooms < 1 || num_rooms > AZ_MAX_NUM_ROOMS ||
      num_texts < 0 || num_texts > AZ_MAX_NUM_TEXTS ||
      start_room_num < 0 || start_room_num >= num_rooms) FAIL();
  loader->num_zones = num_zones;
  loader->num_texts = num_texts;
  loader->planet->num_zones = 0;
  loader->planet->zones = AZ_ALLOC(num_zones, az_zone_t);
  loader->planet->num_rooms = num_rooms;
  loader->planet->rooms = AZ_ALLOC(num_rooms, az_room_t);
  loader->planet->num_texts = 0;
  loader->planet->texts = AZ_ALLOC(num_texts, az_text_t);
  loader->planet->start_room = start_room_num;
  loader->planet->start_position = (az_vector_t){.x = start_x, .y = start_y};
  loader->planet->start_angle = start_angle;
  scan_to_bang(loader);
}

static void parse_text_directive(az_load_planet_t *loader) {
  if (loader->planet->num_texts >= loader->num_texts) FAIL();
  int text_index;
  if (fscanf(loader->file, "%d", &text_index) < 1) FAIL();
  if (text_index != loader->planet->num_texts) FAIL();
  az_text_t *text = &loader->planet->texts[loader->planet->num_texts];
  char *string = scan_string(loader);
  const bool ok = az_sscan_text(string, strlen(string), text);
  free(string);
  if (!ok) FAIL();
  ++loader->planet->num_texts;
  scan_to_bang(loader);
}

static void parse_zone_directive(az_load_planet_t *loader) {
  if (loader->planet->num_zones >= loader->num_zones) FAIL();
  az_zone_t *zone = &loader->planet->zones[loader->planet->num_zones];
  zone->name = scan_string(loader);
  int red, green, blue, music;
  if (fscanf(loader->file, " c(%d,%d,%d) m%d\n",
             &red, &green, &blue, &music) < 4) FAIL();
  if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 ||
      blue > 255 || music < 0 || music >= AZ_NUM_MUSIC_KEYS) FAIL();
  zone->color = (az_color_t){red, green, blue, 255};
  zone->music = (az_music_key_t)music;
  ++loader->planet->num_zones;
  scan_to_bang(loader);
}

static bool parse_directive(az_load_planet_t *loader) {
  switch (fgetc(loader->file)) {
    case 'T': parse_text_directive(loader); return true;
    case 'Z': parse_zone_directive(loader); return true;
    case EOF: return false;
    default: FAIL();
  }
  AZ_ASSERT_UNREACHABLE();
}

static void validate_planet_basis(az_load_planet_t *loader) {
  if (loader->planet->num_zones != loader->num_zones ||
      loader->planet->num_texts != loader->num_texts) FAIL();
}

#undef FAIL

static void parse_planet_basis(az_load_planet_t *loader) {
  if (setjmp(loader->jump) != 0) {
    az_destroy_planet(loader->planet);
    return;
  }
  parse_planet_header(loader);
  while (parse_directive(loader));
  validate_planet_basis(loader);
  loader->success = true;
}

static bool load_planet_basis(const char *filepath, az_planet_t *planet_out) {
  assert(planet_out != NULL);
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  az_load_planet_t loader = {.file = file, .planet = planet_out};
  parse_planet_basis(&loader);
  fclose(file);
  return loader.success;
}

bool az_load_planet(const char *resource_dir, az_planet_t *planet_out) {
  assert(resource_dir != NULL);
  assert(planet_out != NULL);
  const size_t dirlen = strlen(resource_dir);
  char path_buffer[dirlen + 20u];
  memset(planet_out, 0, sizeof(*planet_out));

  sprintf(path_buffer, "%s/rooms/planet.txt", resource_dir);
  if (!load_planet_basis(path_buffer, planet_out)) return false;

  for (int i = 0; i < planet_out->num_rooms; ++i) {
    sprintf(path_buffer, "%s/rooms/room%03d.txt", resource_dir, i);
    if (!az_load_room_from_file(path_buffer, &planet_out->rooms[i]) ||
        planet_out->rooms[i].zone_index >= planet_out->num_zones) {
      az_destroy_planet(planet_out);
      return false;
    }
  }

  return true;
}

/*===========================================================================*/

#define WRITE(...) do { \
    if (fprintf(file, __VA_ARGS__) < 0) return false; \
  } while (false)

static bool write_planet_header(const az_planet_t *planet, FILE *file) {
  WRITE("@P z%d r%d t%d s%d x%.02f y%.02f a%f\n",
        planet->num_zones, planet->num_rooms, planet->num_texts,
        planet->start_room, planet->start_position.x, planet->start_position.y,
        planet->start_angle);
  for (int i = 0; i < planet->num_zones; ++i) {
    const az_zone_t *zone = &planet->zones[i];
    WRITE("!Z\"");
    for (int j = 0; zone->name[j] != '\0'; ++j) {
      switch (zone->name[j]) {
        case '"': WRITE("\\\""); break;
        case '\\': WRITE("\\\\"); break;
        default: WRITE("%c", zone->name[j]); break;
      }
    }
    WRITE("\" c(%d,%d,%d) m%d\n", (int)zone->color.r, (int)zone->color.g,
          (int)zone->color.b, (int)zone->music);
  }
  for (int i = 0; i < planet->num_texts; ++i) {
    const az_text_t *text = &planet->texts[i];
    WRITE("!T%d\"", i);
    if (!az_fprint_text(text, file)) return false;
    WRITE("\"\n");
  }
  return true;
}

#undef WRITE

static bool save_planet_header(const az_planet_t *planet,
                               const char *filepath) {
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool success = write_planet_header(planet, file);
  fclose(file);
  return success;
}

bool az_save_planet(const az_planet_t *planet, const char *resource_dir) {
  assert(planet != NULL);
  assert(resource_dir != NULL);
  const size_t dirlen = strlen(resource_dir);
  char path_buffer[dirlen + 20u];

  for (int i = 0; i < planet->num_rooms; ++i) {
    sprintf(path_buffer, "%s/rooms/room%03d.txt", resource_dir, i);
    if (!az_save_room_to_file(&planet->rooms[i], path_buffer)) return false;
  }

  sprintf(path_buffer, "%s/rooms/planet.txt", resource_dir);
  return save_planet_header(planet, path_buffer);
}

/*===========================================================================*/

void az_destroy_planet(az_planet_t *planet) {
  assert(planet != NULL);
  for (int i = 0; i < planet->num_texts; ++i) {
    az_destroy_text(&planet->texts[i]);
  }
  free(planet->texts);
  for (int i = 0; i < planet->num_zones; ++i) {
    free(planet->zones[i].name);
  }
  free(planet->zones);
  for (int i = 0; i < planet->num_rooms; ++i) {
    az_destroy_room(&planet->rooms[i]);
  }
  free(planet->rooms);
  memset(planet, 0, sizeof(*planet));
}

/*===========================================================================*/
