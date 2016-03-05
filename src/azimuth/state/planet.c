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

#include "azimuth/constants.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/room.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/string.h"

/*===========================================================================*/

// Arbitrary limits to enforce sanity:
#define AZ_MAX_NUM_HINTS 500
#define AZ_MAX_NUM_PARAGRAPHS 50000

typedef struct {
  az_reader_t *reader;
  jmp_buf jump;
  int num_zones, num_hints, num_paragraphs;
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

#define READ(...) do { \
    if (az_rscanf(loader->reader, __VA_ARGS__) < \
        AZ_COUNT_ARGS(__VA_ARGS__) - 1) { \
      FAIL(); \
    } \
  } while (false)

// Read the next non-whitespace character.  If it is '!' or if we reach EOF,
// do nothing more; otherwise, fail parsing.
static void scan_to_bang(az_load_planet_t *loader) {
  char ch;
  if (az_rscanf(loader->reader, " %c", &ch) < 1) return;
  if (ch != '!') FAIL();
}

static void parse_planet_header(az_load_planet_t *loader) {
  int num_zones, num_hints, num_rooms, num_paragraphs, start_room_num;
  READ("@P z%d h%d r%d t%d s%d\n",
       &num_zones, &num_hints, &num_rooms, &num_paragraphs, &start_room_num);
  if (num_zones < 1 || num_zones > AZ_MAX_NUM_ZONES ||
      num_hints < 0 || num_hints > AZ_MAX_NUM_HINTS ||
      num_rooms < 1 || num_rooms > AZ_MAX_NUM_ROOMS ||
      num_paragraphs < 0 || num_paragraphs > AZ_MAX_NUM_PARAGRAPHS ||
      start_room_num < 0 || start_room_num >= num_rooms) FAIL();
  loader->num_zones = num_zones;
  loader->num_hints = num_hints;
  loader->num_paragraphs = num_paragraphs;
  loader->planet->num_zones = 0;
  loader->planet->zones = AZ_ALLOC(num_zones, az_zone_t);
  loader->planet->num_hints = 0;
  loader->planet->hints = AZ_ALLOC(num_hints, az_hint_t);
  loader->planet->num_rooms = num_rooms;
  loader->planet->rooms = AZ_ALLOC(num_rooms, az_room_t);
  loader->planet->num_paragraphs = 0;
  loader->planet->paragraphs = AZ_ALLOC(num_paragraphs, char*);
  loader->planet->start_room = start_room_num;
  char ch = '\0';
  if (az_rscanf(loader->reader, " $s%c", &ch) < 1 || ch != ':') FAIL();
  loader->planet->on_start = az_read_script(loader->reader);
  if (loader->planet->on_start == NULL) FAIL();
  scan_to_bang(loader);
}

static void parse_hint_directive(az_load_planet_t *loader) {
  if (loader->planet->num_hints >= loader->num_hints) FAIL();
  az_hint_t *hint = &loader->planet->hints[loader->planet->num_hints];
  char op1, op2, rtype;
  int prereq1 = 0, prereq2 = 0, result, target_room;
  az_hint_flags_t properties = 0;
  READ("%c", &op1);
  switch (op1) {
    case '|': break;
    case '&': properties |= AZ_HINTF_OP1_IS_AND; break;
    default: FAIL();
  }
  if (properties & AZ_HINTF_OP1_IS_AND) {
    char type;
    READ(" %c%d ", &type, &prereq1);
    switch (type) {
      case 'u': break;
      case 'f': properties |= AZ_HINTF_PREREQ1_IS_FLAG; break;
      default: FAIL();
    }
  }
  READ("%c", &op2);
  switch (op2) {
    case '|': break;
    case '&': properties |= AZ_HINTF_OP2_IS_AND; break;
    default: FAIL();
  }
  if ((properties & AZ_HINTF_OP1_IS_AND) ||
      (properties & AZ_HINTF_OP2_IS_AND)) {
    char type;
    READ(" %c%d", &type, &prereq2);
    switch (type) {
      case 'u': break;
      case 'f': properties |= AZ_HINTF_PREREQ2_IS_FLAG; break;
      default: FAIL();
    }
  }
  READ(" = %c%d r%d\n", &rtype, &result, &target_room);
  switch (rtype) {
    case 'u': break;
    case 'f': properties |= AZ_HINTF_RESULT_IS_FLAG; break;
    default: FAIL();
  }
  if (prereq1 < 0 || prereq2 < 0 || result < 0 ||
      ((properties & AZ_HINTF_PREREQ1_IS_FLAG) ?
       (prereq1 >= AZ_MAX_NUM_FLAGS) : (prereq1 >= AZ_NUM_UPGRADES)) ||
      ((properties & AZ_HINTF_PREREQ2_IS_FLAG) ?
       (prereq2 >= AZ_MAX_NUM_FLAGS) : (prereq2 >= AZ_NUM_UPGRADES)) ||
      ((properties & AZ_HINTF_RESULT_IS_FLAG) ?
       (result >= AZ_MAX_NUM_FLAGS) : (result >= AZ_NUM_UPGRADES)) ||
      target_room < 0 || target_room >= loader->planet->num_rooms) FAIL();
  hint->properties = properties;
  hint->prereq1 = prereq1;
  hint->prereq2 = prereq2;
  hint->result = result;
  hint->target_room = target_room;
  ++loader->planet->num_hints;
  scan_to_bang(loader);
}

static void parse_paragraph_directive(az_load_planet_t *loader) {
  if (loader->planet->num_paragraphs >= loader->num_paragraphs) FAIL();
  int paragraph_index;
  READ("%d", &paragraph_index);
  if (paragraph_index != loader->planet->num_paragraphs) FAIL();
  char *paragraph = az_read_paragraph(loader->reader);
  if (paragraph == NULL) FAIL();
  loader->planet->paragraphs[loader->planet->num_paragraphs] = paragraph;
  ++loader->planet->num_paragraphs;
  scan_to_bang(loader);
}

static void parse_zone_directive(az_load_planet_t *loader) {
  if (loader->planet->num_zones >= loader->num_zones) FAIL();
  az_zone_t *zone = &loader->planet->zones[loader->planet->num_zones];
  zone->name = az_read_paragraph(loader->reader);
  if (zone->name == NULL) FAIL();
  int red, green, blue;
  READ(" c(%d,%d,%d)\n", &red, &green, &blue);
  if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 ||
      blue > 255) FAIL();
  zone->color = (az_color_t){red, green, blue, 255};
  zone->entering_message = az_strprintf("Entering: $X%02x%02x%02x%s",
                                        red, green, blue, zone->name);
  ++loader->planet->num_zones;
  scan_to_bang(loader);
}

static bool parse_directive(az_load_planet_t *loader) {
  switch (az_rgetc(loader->reader)) {
    case 'H': parse_hint_directive(loader); return true;
    case 'T': parse_paragraph_directive(loader); return true;
    case 'Z': parse_zone_directive(loader); return true;
    case EOF: return false;
    default: FAIL();
  }
  AZ_ASSERT_UNREACHABLE();
}

static void validate_planet_basis(az_load_planet_t *loader) {
  if (loader->planet->num_zones != loader->num_zones ||
      loader->planet->num_hints != loader->num_hints ||
      loader->planet->num_paragraphs != loader->num_paragraphs) FAIL();
}

#undef READ
#undef FAIL

static bool parse_planet_basis(az_load_planet_t *loader) {
  if (setjmp(loader->jump) != 0) {
    az_destroy_planet(loader->planet);
    return false;
  }
  parse_planet_header(loader);
  while (parse_directive(loader));
  validate_planet_basis(loader);
  return true;
}

static bool read_planet_basis(az_reader_t *reader, az_planet_t *planet_out) {
  assert(planet_out != NULL);
  az_load_planet_t loader = {.reader = reader, .planet = planet_out};
  return parse_planet_basis(&loader);
}

bool az_read_planet(az_resource_reader_fn_t resource_reader,
                    az_planet_t *planet_out) {
  assert(planet_out != NULL);

  az_reader_t reader;
  if (!resource_reader("rooms/planet.txt", &reader)) return false;
  bool success = read_planet_basis(&reader, planet_out);
  az_rclose(&reader);
  if (!success) return false;

  for (int i = 0; i < planet_out->num_rooms; ++i) {
    char *room_name = az_strprintf("rooms/room%03d.txt", i);
    resource_reader(room_name, &reader);
    success = az_read_room(&reader, &planet_out->rooms[i]) &&
      planet_out->rooms[i].zone_key < planet_out->num_zones;
    az_rclose(&reader);
    free(room_name);
    if (!success) {
      az_destroy_planet(planet_out);
      return false;
    }
  }

  return true;
}

/*===========================================================================*/

#define WRITE(...) do { \
    if (!az_wprintf(writer, __VA_ARGS__)) return false; \
  } while (false)

static bool write_planet_basis(const az_planet_t *planet,
                               az_writer_t *writer) {
  WRITE("@P z%d h%d r%d t%d s%d\n",
        planet->num_zones, planet->num_hints, planet->num_rooms,
        planet->num_paragraphs, planet->start_room);
  if (planet->on_start != NULL) {
    WRITE("$s:");
    if (!az_write_script(planet->on_start, writer)) return false;
    WRITE("\n");
  }
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
    WRITE("\" c(%d,%d,%d)\n", (int)zone->color.r, (int)zone->color.g,
          (int)zone->color.b);
  }
  for (int i = 0; i < planet->num_hints; ++i) {
    const az_hint_t *hint = &planet->hints[i];
    if (hint->properties & AZ_HINTF_OP1_IS_AND) {
      WRITE("!H& %c%d %c %c%d",
            ((hint->properties & AZ_HINTF_PREREQ1_IS_FLAG) ? 'f' : 'u'),
            (int)hint->prereq1,
            ((hint->properties & AZ_HINTF_OP2_IS_AND) ? '&' : '|'),
            ((hint->properties & AZ_HINTF_PREREQ2_IS_FLAG) ? 'f' : 'u'),
            (int)hint->prereq2);
    } else if (hint->properties & AZ_HINTF_OP2_IS_AND) {
      WRITE("!H|& %c%d",
            ((hint->properties & AZ_HINTF_PREREQ2_IS_FLAG) ? 'f' : 'u'),
            (int)hint->prereq2);
    } else {
      WRITE("!H||");
    }
    WRITE(" = %c%d r%d\n",
          ((hint->properties & AZ_HINTF_RESULT_IS_FLAG) ? 'f' : 'u'),
          (int)hint->result, hint->target_room);
  }
  for (int i = 0; i < planet->num_paragraphs; ++i) {
    const char *paragraph = planet->paragraphs[i];
    WRITE("!T%d", i);
    if (!az_write_paragraph(paragraph, writer)) return false;
    WRITE("\n");
  }
  return true;
}

#undef WRITE

bool az_write_planet(const az_planet_t *planet,
                     az_resource_writer_fn_t resource_writer,
                     const az_room_key_t *rooms_to_write,
                     int num_rooms_to_write) {
  assert(planet != NULL);

  for (int i = 0; i < num_rooms_to_write; ++i) {
    const int key = rooms_to_write[i];
    char *room_name = az_strprintf("rooms/room%03d.txt", key);
    bool success = false;
    az_writer_t writer;
    if (resource_writer(room_name, &writer)) {
      success = az_write_room(&planet->rooms[key], &writer);
      az_wclose(&writer);
    }
    free(room_name);
    if (!success) return false;
  }

  bool success = false;
  az_writer_t writer;
  if (resource_writer("rooms/planet.txt", &writer)) {
    success = write_planet_basis(planet, &writer);
    az_wclose(&writer);
  }
  return success;
}

/*===========================================================================*/

void az_destroy_planet(az_planet_t *planet) {
  assert(planet != NULL);
  az_free_script(planet->on_start);
  for (int i = 0; i < planet->num_paragraphs; ++i) {
    free(planet->paragraphs[i]);
  }
  free(planet->paragraphs);
  for (int i = 0; i < planet->num_zones; ++i) {
    free(planet->zones[i].name);
    free(planet->zones[i].entering_message);
  }
  free(planet->zones);
  for (int i = 0; i < planet->num_rooms; ++i) {
    az_destroy_room(&planet->rooms[i]);
  }
  free(planet->rooms);
  AZ_ZERO_OBJECT(planet);
}

void az_clone_zone(const az_zone_t *original, az_zone_t *clone_out) {
  clone_out->name = az_strdup(original->name);
  clone_out->entering_message = az_strdup(original->entering_message);
  clone_out->color = original->color;
}

/*===========================================================================*/

static bool has_hint_item(const az_player_t *player, int item, bool is_flag) {
  return (is_flag ? az_test_flag(player, (az_flag_t)item) :
          az_has_upgrade(player, (az_upgrade_t)item));
}

bool az_hint_matches(const az_hint_t *hint, const az_player_t *player) {
  if (has_hint_item(player, hint->result,
                    (hint->properties & AZ_HINTF_RESULT_IS_FLAG))) {
    return false;
  }
  const bool or1 = !(hint->properties & AZ_HINTF_OP1_IS_AND);
  const bool or2 = !(hint->properties & AZ_HINTF_OP2_IS_AND);
  if (or1 && or2) return true;
  const bool match2 =
    has_hint_item(player, hint->prereq2,
                  (hint->properties & AZ_HINTF_PREREQ2_IS_FLAG));
  if (or1) return match2;
  const bool match1 =
    has_hint_item(player, hint->prereq1,
                  (hint->properties & AZ_HINTF_PREREQ1_IS_FLAG));
  if (or2) return match1 || match2;
  else return match1 && match2;
}

const az_hint_t *az_get_hint(const az_planet_t *planet,
                             const az_player_t *player) {
  for (int i = 0; i < planet->num_hints; ++i) {
    const az_hint_t *hint = &planet->hints[i];
    if (az_hint_matches(hint, player)) return hint;
  }
  return NULL;
}

/*===========================================================================*/
