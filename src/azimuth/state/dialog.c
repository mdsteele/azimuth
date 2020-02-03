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
#include "azimuth/util/key.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

#define WRITE(...) do { \
    if (!az_wprintf(writer, __VA_ARGS__)) return false; \
  } while (false)

bool az_write_paragraph(const char *paragraph, az_writer_t *writer) {
  WRITE("\"\n  ");
  for (const char *ch = paragraph; *ch != '\0'; ++ch) {
    switch (*ch) {
      case '\n': WRITE(*(ch + 1) == '\n' ? "\n" : "\n  "); break;
      case '"': WRITE("\\\""); break;
      case '\\': WRITE("\\\\"); break;
      default: WRITE("%c", *ch); break;
    }
  }
  WRITE("\"");
  return true;
}

#undef WRITE

/*===========================================================================*/

char *az_read_paragraph(az_reader_t *reader) {
  if (az_rgetc(reader) != '"') return NULL;
  az_rw_pos_t pos;
  if (!az_rgetpos(reader, &pos)) return NULL;
  bool skip_spaces = true, skip_linebreaks = true;
  int paragraph_size = 1;  // Include 1 for terminating NUL.
  for (int ch = az_rgetc(reader); ch != '"'; ch = az_rgetc(reader)) {
    switch (ch) {
      case EOF: return NULL;
      case '\n':
        if (skip_linebreaks) continue;
        skip_spaces = true;
        break;
      case ' ':
        if (skip_spaces) continue;
        break;
      case '\\':
        ch = az_rgetc(reader);
        if (ch == EOF) return NULL;
        // fallthrough
      default:
        skip_spaces = skip_linebreaks = false;
        break;
    }
    ++paragraph_size;
  }
  if (!az_rsetpos(reader, &pos)) return NULL;
  char *paragraph = AZ_ALLOC(paragraph_size, char);
  skip_spaces = skip_linebreaks = true;
  int p_index = 0;
  for (int ch = az_rgetc(reader); ch != '"'; ch = az_rgetc(reader)) {
    switch (ch) {
      case '\n':
        if (skip_linebreaks) continue;
        skip_spaces = true;
        break;
      case ' ':
        if (skip_spaces) continue;
        break;
      case '\\':
        ch = az_rgetc(reader);
        // fallthrough
      default:
        assert(ch != EOF);
        skip_spaces = skip_linebreaks = false;
        break;
    }
    paragraph[p_index++] = ch;
  }
  assert(paragraph[paragraph_size - 1] == '\0');
  return paragraph;
}

/*===========================================================================*/

int az_paragraph_num_lines(const char *paragraph) {
  int num_lines = 1;
  for (const char *ch = paragraph; *ch != '\0'; ++ch) {
    if (*ch == '\n') ++num_lines;
  }
  return num_lines;
}

static int paragraph_line_length_internal(
    const az_preferences_t *prefs, const char *paragraph, int *start) {
  int line_length = 0;
  int line_end = *start;
  const az_key_id_t *key_for_control = prefs->key_for_control;
  while (paragraph[line_end] != '\0' && paragraph[line_end] != '\n') {
    ++line_end;
    // If we see a $-escape, we need to determine its expanded length.
    if (paragraph[line_end - 1] == '$') {
      az_key_id_t key_id = AZ_KEY_UNKNOWN;
      switch (paragraph[line_end]) {
        case '\0': break;
        // A "$$" escape inserts a literal '$' character, so that adds 1 to
        // the length of the line.
        case '$': ++line_end; ++line_length; break;
        // A "$_" escape prints nothing, but artificially increases the
        // calculated length of the line by the two-digit number specified
        // after it.
        case '_': {
          ++line_end;
          int pause = 0, stop = line_end + 2;
          for (; line_end < stop && paragraph[line_end] != '\0'; ++line_end) {
            if (paragraph[line_end] >= '0' && paragraph[line_end] <= '9' &&
                pause >= 0) {
              pause = pause * 10 + (paragraph[line_end] - '0');
            } else pause = -1;
          }
          if (pause >= 0 && line_end == stop) line_length += pause;
        } break;
        // A "$X" escape adds no length, but we need to skip over the six hex
        // digits that come after it.  However, we must take care not to
        // advance line_end past the end of the paragraph if the escape is
        // incomplete.
        case 'X':
          ++line_end;
          for (int stop = line_end + 6; line_end < stop &&
                 paragraph[line_end] != '\0'; ++line_end) {}
          break;
        // The key name escapes add the length of the key name to the line.
        // Note for now which key_id we need to insert the name of, and we'll
        // handle it below.
        case 'u': key_id = key_for_control[AZ_CONTROL_UP]; break;
        case 'd': key_id = key_for_control[AZ_CONTROL_DOWN]; break;
        case 'r': key_id = key_for_control[AZ_CONTROL_RIGHT]; break;
        case 'l': key_id = key_for_control[AZ_CONTROL_LEFT]; break;
        case 'f': key_id = key_for_control[AZ_CONTROL_FIRE]; break;
        case 'o': key_id = key_for_control[AZ_CONTROL_ORDN]; break;
        case 't': key_id = key_for_control[AZ_CONTROL_UTIL]; break;
        case '0': key_id = key_for_control[AZ_CONTROL_BOMBS]; break;
        case '1': key_id = key_for_control[AZ_CONTROL_CHARGE]; break;
        case '2': key_id = key_for_control[AZ_CONTROL_FREEZE]; break;
        case '3': key_id = key_for_control[AZ_CONTROL_TRIPLE]; break;
        case '4': key_id = key_for_control[AZ_CONTROL_HOMING]; break;
        case '5': key_id = key_for_control[AZ_CONTROL_PHASE]; break;
        case '6': key_id = key_for_control[AZ_CONTROL_BURST]; break;
        case '7': key_id = key_for_control[AZ_CONTROL_PIERCE]; break;
        case '8': key_id = key_for_control[AZ_CONTROL_BEAM]; break;
        case '9': key_id = key_for_control[AZ_CONTROL_ROCKETS]; break;
        // All other escapes are just a single character after the '$', which
        // we should skip over without increasing the line length.
        default:
          ++line_end;
          break;
      }
      // If the escape we just saw was for a key name, look up the name of that
      // key and add its length to the line length.  Also, increment line_end
      // to skip over the escape, since we didn't do that above for the key
      // name escapes.
      if (key_id != AZ_KEY_UNKNOWN) {
        ++line_end;
        line_length += strlen(az_key_name(key_id));
      }
    }
    // Each literal character (i.e. not a $-escape) increments the line length.
    else ++line_length;
  }
  if (paragraph[line_end] == '\n') ++line_end;
  else assert(paragraph[line_end] == '\0');
  *start = line_end;
  return line_length;
}

int az_paragraph_line_length(
    const az_preferences_t *prefs, const char *paragraph, int start) {
  return paragraph_line_length_internal(prefs, paragraph, &start);
}

int az_paragraph_total_length(
    const az_preferences_t *prefs, const char *paragraph) {
  int total_length = 0;
  int start = 0;
  while (paragraph[start] != '\0') {
    total_length += paragraph_line_length_internal(prefs, paragraph, &start);
  }
  return total_length;
}

void az_set_dialogue_text(
    az_dialogue_state_t *dialogue, const az_preferences_t *prefs,
    const char *paragraph, bool top_turn) {
  const int paragraph_length = az_paragraph_total_length(prefs, paragraph);
  dialogue->step = (paragraph_length == 0 ? AZ_DLS_WAIT : AZ_DLS_TALK);
  dialogue->progress = 0.0;
  dialogue->hidden = false;
  dialogue->top_turn = top_turn;
  dialogue->paragraph = paragraph;
  dialogue->paragraph_length = paragraph_length;
  dialogue->chars_to_print = 0;
}

/*===========================================================================*/
