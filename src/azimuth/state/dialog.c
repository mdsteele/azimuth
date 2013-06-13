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

#include "azimuth/state/dialog.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

static bool same_color(az_color_t color1, az_color_t color2) {
  return (color1.r == color2.r && color1.g == color2.g &&
          color1.b == color2.b && color1.a == color2.a);
}

static char color_escape(az_color_t color) {
  if (same_color(color, AZ_WHITE)) return 'W';
  if (same_color(color, AZ_GRAY)) return 'A';
  if (same_color(color, AZ_RED)) return 'R';
  if (same_color(color, AZ_GREEN)) return 'G';
  if (same_color(color, AZ_BLUE)) return 'B';
  if (same_color(color, AZ_MAGENTA)) return 'M';
  if (same_color(color, AZ_YELLOW)) return 'Y';
  if (same_color(color, AZ_CYAN)) return 'C';
  return '\0';
}

#define WRITE(...) do { \
    if (fprintf(file, __VA_ARGS__) < 0) return false; \
  } while (false)

bool az_fprint_text(const az_text_t *text, FILE *file) {
  az_color_t color = AZ_WHITE;
  for (int i = 0; i < text->num_lines; ++i) {
    const az_text_line_t *line = &text->lines[i];
    WRITE("\n  ");
    for (int j = 0; j < line->num_fragments; ++j) {
      const az_text_fragment_t *fragment = &line->fragments[j];
      if (!same_color(fragment->color, color)) {
        color = fragment->color;
        const char escape = color_escape(color);
        if (escape == '\0') {
          WRITE("$X%02x%02x%02x", (unsigned int)color.r,
                (unsigned int)color.g, (unsigned int)color.b);
        } else {
          WRITE("$%c", escape);
        }
      }
      for (int k = 0; k < fragment->length; ++k) {
        const char ch = fragment->chars[k];
        switch (ch) {
          case '"': WRITE("\\\""); break;
          case '\\': WRITE("\\\\"); break;
          default: WRITE("%c", ch); break;
        }
      }
    }
  }
  return true;
}

#undef WRITE

/*===========================================================================*/

static bool hex_parse(char c1, char c2, uint8_t *out) {
  int value = 0;
  if ('0' <= c1 && c1 <= '9') value += c1 - '0';
  else if ('a' <= c1 && c1 <= 'f') value += 10 + c1 - 'a';
  else return false;
  value <<= 4;
  if ('0' <= c2 && c2 <= '9') value += c2 - '0';
  else if ('a' <= c2 && c2 <= 'f') value += 10 + c2 - 'a';
  else return false;
  assert(value >= 0);
  assert(value <= 255);
  *out = value;
  return true;
}

bool az_sscan_text(const char *string, int length, az_text_t *text_out) {
  assert(text_out != NULL);
  memset(text_out, 0, sizeof(*text_out));
  // Skip leading whitespace:
  int start = 0;
  for (; start < length; ++start) {
    if (string[start] != ' ' && string[start] != '\n') break;
  }
  // Count lines:
  text_out->num_lines = 1;
  for (int i = start; i < length; ++i) {
    if (string[i] == '\n') ++text_out->num_lines;
  }
  // Construct lines:
  text_out->lines = AZ_ALLOC(text_out->num_lines, az_text_line_t);
  az_color_t color = AZ_WHITE;
  for (int line_num = 0; line_num < text_out->num_lines; ++line_num) {
    az_text_line_t *line = &text_out->lines[line_num];
    // Skip leading whitespace:
    for (; start < length; ++start) {
      if (string[start] != ' ') break;
    }
    // Count fragments:
    line->num_fragments = (start >= length || string[start] == '$' ||
                           string[start] == '\n' ? 0 : 1);
    int line_end = start;
    for (; line_end < length && string[line_end] != '\n'; ++line_end) {
      if (string[line_end] == '$') ++line->num_fragments;
    }
    // Construct fragments:
    assert(line->total_length == 0);
    line->fragments = AZ_ALLOC(line->num_fragments, az_text_fragment_t);
    for (int frag_num = 0; frag_num < line->num_fragments; ++frag_num) {
      assert(start < length);
      if (string[start] == '$') {
        ++start;
        if (start >= length) goto error;
        switch (string[start]) {
          case 'W': color = AZ_WHITE; break;
          case 'A': color = AZ_GRAY; break;
          case 'R': color = AZ_RED; break;
          case 'G': color = AZ_GREEN; break;
          case 'B': color = AZ_BLUE; break;
          case 'M': color = AZ_MAGENTA; break;
          case 'Y': color = AZ_YELLOW; break;
          case 'C': color = AZ_CYAN; break;
          case 'X':
            if (start + 6 < length &&
                hex_parse(string[start + 1], string[start + 2], &color.r) &&
                hex_parse(string[start + 3], string[start + 4], &color.g) &&
                hex_parse(string[start + 5], string[start + 6], &color.b)) {
              color.a = 255;
              start += 6;
            } else goto error;
            break;
          default: goto error;
        }
        ++start;
      }
      az_text_fragment_t *fragment = &line->fragments[frag_num];
      fragment->color = color;
      int frag_end = start;
      while (frag_end < line_end && string[frag_end] != '$') ++frag_end;
      fragment->length = frag_end - start;
      line->total_length += fragment->length;
      fragment->chars = AZ_ALLOC(fragment->length, char);
      memcpy(fragment->chars, string + start, fragment->length);
      start = frag_end;
    }
    assert(start == line_end);
    assert(start == length || (start < length && string[start] == '\n'));
    ++start;
  }
  assert(start == length + 1);
  return true;
 error:
  az_destroy_text(text_out);
  return false;
}

/*===========================================================================*/

void az_clone_text(const az_text_t *text,
                   az_text_t *copy_out) {
  assert(text != NULL);
  assert(copy_out != NULL);
  assert(text != copy_out);
  copy_out->num_lines = text->num_lines;
  copy_out->lines = AZ_ALLOC(text->num_lines, az_text_line_t);
  for (int i = 0; i < text->num_lines; ++i) {
    const az_text_line_t *line = &text->lines[i];
    az_text_line_t *line_out = &copy_out->lines[i];
    line_out->total_length = line->total_length;
    line_out->num_fragments = line->num_fragments;
    line_out->fragments = AZ_ALLOC(line->num_fragments, az_text_fragment_t);
    for (int j = 0; j < line->num_fragments; ++j) {
      const az_text_fragment_t *frag = &line->fragments[j];
      az_text_fragment_t *frag_out = &line_out->fragments[j];
      frag_out->color = frag->color;
      frag_out->length = frag->length;
      frag_out->chars = AZ_ALLOC(frag->length, char);
      memcpy(frag_out->chars, frag->chars, frag->length);
    }
  }
}

void az_destroy_text(az_text_t *text) {
  assert(text != NULL);
  assert(text->num_lines == 0 || text->lines != NULL);
  for (int i = 0; i < text->num_lines; ++i) {
    az_text_line_t *line = &text->lines[i];
    assert(line->num_fragments == 0 || line->fragments != NULL);
    for (int j = 0; j < line->num_fragments; ++j) {
      free(line->fragments[j].chars);
    }
    free(line->fragments);
  }
  free(text->lines);
  memset(text, 0, sizeof(*text));
}

/*===========================================================================*/
