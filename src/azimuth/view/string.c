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

#include "azimuth/view/string.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <GL/gl.h>

#include "azimuth/util/key.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/prefs.h"
#include "azimuth/util/warning.h"

/*===========================================================================*/

#define FONT_SIZE 8

static struct {
  GLenum mode;
  int num_points;
  struct { GLfloat x, y; } points[8];
} char_specs[] = {
  [0x11] = {GL_LINES, 6, {{3,0}, {3,6}, {0,3}, {3,0}, {6,3}, {3,0}}},
  [0x12] = {GL_LINES, 6, {{3,0}, {3,6}, {0,3}, {3,6}, {6,3}, {3,6}}},
  [0x13] = {GL_LINES, 6, {{0,3}, {6,3}, {3,0}, {6,3}, {3,6}, {6,3}}},
  [0x14] = {GL_LINES, 6, {{0,3}, {6,3}, {3,0}, {0,3}, {3,6}, {0,3}}},
  ['!'] = {GL_LINES, 4, {{2,0}, {2,3}, {2,5}, {2,6}}},
  ['"'] = {GL_LINES, 4, {{1,0}, {1,3}, {4,0}, {4,3}}},
  ['#'] = {GL_LINES, 8, {{2,0}, {2,6}, {4,0}, {4,6},
                         {1,2}, {5,2}, {1,4}, {5,4}}},
  ['$'] = {GL_LINE_LOOP, 8, {{6,1}, {0,1}, {0,3}, {6,3}, {6,5}, {0,5},
                             {2,6}, {4,0}}},
  ['%'] = {GL_LINES, 6, {{0,6}, {6,0}, {1,0}, {1,2}, {5,4}, {5,6}}},
  ['&'] = {GL_LINE_STRIP, 8, {{5,6}, {1,2}, {3,0}, {4,1},
                              {1,4}, {1,6}, {3,6}, {5,4}}},
  ['\''] = {GL_LINES, 2, {{3,0}, {3,3}}},
  ['('] = {GL_LINE_STRIP, 4, {{4,0}, {2,2}, {2,4}, {4,6}}},
  [')'] = {GL_LINE_STRIP, 4, {{2,0}, {4,2}, {4,4}, {2,6}}},
  ['*'] = {GL_LINES, 6, {{3,1}, {3,5}, {1,2}, {5,4}, {1,4}, {5,2}}},
  ['+'] = {GL_LINES, 4, {{1,3}, {5,3}, {3,1}, {3,5}}},
  [','] = {GL_LINES, 2, {{3,5}, {1,7}}},
  ['-'] = {GL_LINES, 2, {{1,3}, {5,3}}},
  ['.'] = {GL_POINTS, 1, {{2,6}}},
  ['/'] = {GL_LINES, 2, {{0,6}, {6,0}}},
  ['0'] = {GL_LINE_LOOP, 8, {{0,2}, {2,0}, {4,0}, {6,2},
                             {6,4}, {4,6}, {2,6}, {0,4}}},
  ['1'] = {GL_LINES, 6, {{2,1}, {3,0}, {3,0}, {3,6}, {1,6}, {5,6}}},
  ['2'] = {GL_LINE_STRIP, 6, {{0,0}, {6,0}, {6,3}, {0,3}, {0,6}, {6,6}}},
  ['3'] = {GL_LINES, 8, {{0,0}, {6,0}, {6,0}, {6,6}, {6,6}, {0,6},
                         {6,3}, {2,3}}},
  ['4'] = {GL_LINES, 6, {{0,0}, {0,3}, {0,3}, {6,3}, {6,0}, {6,6}}},
  ['5'] = {GL_LINE_STRIP, 6, {{6,0}, {0,0}, {0,3}, {6,3}, {6,6}, {0,6}}},
  ['6'] = {GL_LINE_STRIP, 6, {{6,0}, {0,0}, {0,6}, {6,6}, {6,3}, {0,3}}},
  ['7'] = {GL_LINE_STRIP, 3, {{0,0}, {6,0}, {3,6}}},
  ['8'] = {GL_LINE_STRIP, 7, {{6,3}, {6,0}, {0,0}, {0,6},
                              {6,6}, {6,3}, {0,3}}},
  ['9'] = {GL_LINE_STRIP, 6, {{0,6}, {6,6}, {6,0}, {0,0}, {0,3}, {6,3}}},
  [':'] = {GL_POINTS, 2, {{2,2}, {2,5}}},
  [';'] = {GL_LINES, 4, {{3,5}, {1,7}, {2,1.5}, {2,2.5}}},
  ['<'] = {GL_LINE_STRIP, 3, {{6,0}, {0,3}, {6,6}}},
  ['='] = {GL_LINES, 4, {{1,2}, {5,2}, {1,4}, {5,4}}},
  ['>'] = {GL_LINE_STRIP, 3, {{0,0}, {6,3}, {0,6}}},
  ['?'] = {GL_LINES, 8, {{0,0}, {3,0}, {3,0}, {4,1}, {4,1}, {2,3},
                         {2,5}, {2,6}}},
  ['@'] = {GL_LINE_STRIP, 8, {{4.5,2}, {2,2}, {2,4}, {6,4},
                              {6,0}, {0,0}, {0,6}, {6,6}}},
  ['A'] = {GL_LINES, 6, {{0,6}, {3,0}, {3,0}, {6,6}, {1,4}, {5,4}}},
  ['B'] = {GL_LINE_STRIP, 7, {{5,3}, {5,0}, {0,0}, {0,6},
                              {6,6}, {6,3}, {0,3}}},
  ['C'] = {GL_LINE_STRIP, 4, {{6,0}, {0,0}, {0,6}, {6,6}}},
  ['D'] = {GL_LINE_LOOP, 6, {{0,0}, {4,0}, {6,2}, {6,4}, {4,6}, {0,6}}},
  ['E'] = {GL_LINES, 8, {{6,0}, {0,0}, {0,0}, {0,6}, {0,6}, {6,6},
                         {0,3}, {4,3}}},
  ['F'] = {GL_LINES, 6, {{6,0}, {0,0}, {0,0}, {0,6}, {0,3}, {4,3}}},
  ['G'] = {GL_LINE_STRIP, 6, {{6,0}, {0,0}, {0,6}, {6,6}, {6,3}, {3,3}}},
  ['H'] = {GL_LINES, 6, {{0,0}, {0,6}, {0,3}, {6,3}, {6,0}, {6,6}}},
  ['I'] = {GL_LINES, 6, {{1,0}, {5,0}, {3,0}, {3,6}, {1,6}, {5,6}}},
  ['J'] = {GL_LINE_STRIP, 4, {{6,0}, {6,6}, {0,6}, {0,4}}},
  ['K'] = {GL_LINES, 6, {{0,0}, {0,6}, {6,0}, {0,3}, {0,3}, {6,6}}},
  ['L'] = {GL_LINE_STRIP, 3, {{0,0}, {0,6}, {6,6}}},
  ['M'] = {GL_LINE_STRIP, 5, {{0,6}, {0,0}, {3,3}, {6,0}, {6,6}}},
  ['N'] = {GL_LINE_STRIP, 4, {{0,6}, {0,0}, {6,6}, {6,0}}},
  ['O'] = {GL_LINE_LOOP, 8, {{0,2}, {2,0}, {4,0}, {6,2},
                             {6,4}, {4,6}, {2,6}, {0,4}}},
  ['P'] = {GL_LINE_STRIP, 5, {{0,6}, {0,0}, {6,0}, {6,3}, {0,3}}},
  ['Q'] = {GL_LINE_STRIP, 6, {{6,6}, {0,6}, {0,0}, {6,0}, {6,6}, {3,3}}},
  ['R'] = {GL_LINE_STRIP, 6, {{0,6}, {0,0}, {6,0}, {6,3}, {0,3}, {6,6}}},
  ['S'] = {GL_LINE_STRIP, 6, {{6,0}, {0,0}, {0,3}, {6,3}, {6,6}, {0,6}}},
  ['T'] = {GL_LINES, 4, {{0,0}, {6,0}, {3,0}, {3,6}}},
  ['U'] = {GL_LINE_STRIP, 4, {{0,0}, {0,6}, {6,6}, {6,0}}},
  ['V'] = {GL_LINE_STRIP, 3, {{0,0}, {3,6}, {6,0}}},
  ['W'] = {GL_LINE_STRIP, 5, {{0,0}, {0,6}, {3,3}, {6,6}, {6,0}}},
  ['X'] = {GL_LINES, 4, {{0,0}, {6,6}, {0,6}, {6,0}}},
  ['Y'] = {GL_LINES, 6, {{0,0}, {3,3}, {3,3}, {6,0}, {3,3}, {3,6}}},
  ['Z'] = {GL_LINE_STRIP, 4, {{0,0}, {6,0}, {0,6}, {6,6}}},
  ['['] = {GL_LINE_STRIP, 4, {{4,0}, {2,0}, {2,6}, {4,6}}},
  ['\\'] = {GL_LINES, 2, {{0,0}, {6,6}}},
  [']'] = {GL_LINE_STRIP, 4, {{2,0}, {4,0}, {4,6}, {2,6}}},
  ['^'] = {GL_LINE_STRIP, 3, {{0,3}, {3,0}, {6,3}}},
  ['_'] = {GL_LINES, 2, {{0,6}, {6,6}}},
  ['`'] = {GL_LINES, 2, {{2,0}, {4,2}}},
  ['a'] = {GL_LINE_STRIP, 6, {{1,2}, {5,2}, {5,6}, {1,6}, {1,4}, {5,4}}},
  ['b'] = {GL_LINE_STRIP, 5, {{1,0}, {1,6}, {5,6}, {5,3}, {1,3}}},
  ['c'] = {GL_LINE_STRIP, 4, {{5,2}, {1,2}, {1,6}, {5,6}}},
  ['d'] = {GL_LINE_STRIP, 5, {{5,0}, {5,6}, {1,6}, {1,3}, {5,3}}},
  ['e'] = {GL_LINE_STRIP, 6, {{5,6}, {1,6}, {1,2}, {5,2}, {5,4}, {1,4}}},
  ['f'] = {GL_LINES, 6, {{5,0}, {3,0}, {3,0}, {3,6}, {1,3}, {5,3}}},
  ['g'] = {GL_LINE_STRIP, 6, {{1,7}, {5,7}, {5,2}, {1,2}, {1,5}, {5,5}}},
  ['h'] = {GL_LINES, 6, {{1,0}, {1,6}, {1,3}, {5,3}, {5,3}, {5,6}}},
  ['i'] = {GL_LINES, 8, {{1,6}, {5,6}, {3,6}, {3,3}, {3,3}, {1,3},
                         {3,0}, {3,1}}},
  ['j'] = {GL_LINES, 8, {{0,7}, {3,7}, {3,7}, {3,3}, {3,3}, {1,3},
                         {3,0}, {3,1}}},
  ['k'] = {GL_LINES, 6, {{1,0}, {1,6}, {5,2}, {1,4}, {1,4}, {5,6}}},
  ['l'] = {GL_LINE_STRIP, 3, {{3,0}, {3,6}, {4,6}}},
  ['m'] = {GL_LINES, 8, {{1,6}, {1,2}, {1,2}, {5,2}, {5,2}, {5,6},
                         {3,2}, {3,5}}},
  ['n'] = {GL_LINE_STRIP, 4, {{1,6}, {1,2}, {5,2}, {5,6}}},
  ['o'] = {GL_LINE_LOOP, 4, {{1,6}, {1,2}, {5,2}, {5,6}}},
  ['p'] = {GL_LINE_STRIP, 5, {{1,7}, {1,2}, {5,2}, {5,5}, {1,5}}},
  ['q'] = {GL_LINE_STRIP, 5, {{5,7}, {5,2}, {1,2}, {1,5}, {5,5}}},
  ['r'] = {GL_LINE_STRIP, 3, {{1,6}, {1,2}, {5,2}}},
  ['s'] = {GL_LINE_STRIP, 6, {{5,2}, {1,2}, {1,4}, {5,4}, {5,6}, {1,6}}},
  ['t'] = {GL_LINES, 4, {{1,2}, {5,2}, {3,0}, {3,6}}},
  ['u'] = {GL_LINE_STRIP, 4, {{1,2}, {1,6}, {5,6}, {5,2}}},
  ['v'] = {GL_LINE_STRIP, 3, {{1,2}, {3,6}, {5,2}}},
  ['w'] = {GL_LINES, 8, {{1,2}, {1,6}, {1,6}, {5,6}, {5,6}, {5,2},
                         {3,6}, {3,3}}},
  ['x'] = {GL_LINES, 4, {{1,2}, {5,6}, {1,6}, {5,2}}},
  ['y'] = {GL_LINES, 8, {{1,7}, {5,7}, {5,7}, {5,2},
                         {1,2}, {1,5}, {1,5}, {5,5}}},
  ['z'] = {GL_LINE_STRIP, 4, {{1,2}, {5,2}, {1,6}, {5,6}}},
  ['{'] = {GL_LINE_STRIP, 7, {{4.5,0}, {3,0}, {3,2}, {2,3},
                              {3,4}, {3,6}, {4.5,6}}},
  ['|'] = {GL_LINES, 2, {{3,0}, {3,6}}},
  ['}'] = {GL_LINE_STRIP, 7, {{1.5,0}, {3,0}, {3,2}, {4,3},
                              {3,4}, {3,6}, {1.5,6}}},
  ['~'] = {GL_LINE_STRIP, 4, {{0,4}, {2,2}, {4,4}, {6,2}}}
};

static void draw_char(int c) {
  if (c >= 0 && c < AZ_ARRAY_SIZE(char_specs) &&
      char_specs[c].num_points > 0) {
    glBegin(char_specs[c].mode); {
      for (int i = 0; i < char_specs[c].num_points; ++i) {
        glVertex2f(char_specs[c].points[i].x, char_specs[c].points[i].y);
      }
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_string(double height, az_alignment_t align, double x, double top,
                    const char* string) {
  az_draw_chars(height, align, x, top, string, strlen(string));
}

void az_draw_chars(double height, az_alignment_t align, double x, double top,
                   const char* chars, size_t len) {
  double left = x;
  switch (align) {
    case AZ_ALIGN_LEFT: break;
    case AZ_ALIGN_CENTER: left -= 0.5 * (len * height - 0.25 * height); break;
    case AZ_ALIGN_RIGHT: left -= len * height; break;
  }
  glPushMatrix(); {
    glTranslated(left + 0.5, top + 0.5, 0);
    glScaled(height / FONT_SIZE, height / FONT_SIZE, 1);
    for (size_t i = 0; i < len; ++i) {
      draw_char(chars[i]);
      glTranslated(FONT_SIZE, 0, 0);
    }
  } glPopMatrix();
}

void az_draw_printf(double height, az_alignment_t align, double x, double top,
                    const char *format, ...) {
  va_list args;
  va_start(args, format);
  const size_t size = vsnprintf(NULL, 0, format, args);
  va_end(args);
  char buffer[size + 1]; // add one for trailing '\0'
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  az_draw_chars(height, align, x, top, buffer, size);
}

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

void az_draw_paragraph(
    double height, az_alignment_t align, double x, double top, double spacing,
    int end_row, int end_col, const az_preferences_t *prefs,
    const char *string) {
  assert(prefs != NULL);
  assert(string != NULL);
  // Start out with white text.
  glColor3f(1, 1, 1);
  // Draw each line of text, one per outer loop iteration.  We will return from
  // this function when we reach the end (NUL character) of the string, or when
  // we reach the (end_row, end_col) position.
  int row = 0; // current line number
  int line_start = 0; // index into string for first character of current line
  double line_top = top; // y-position of top of the current line
  while (true) {
    // Determine the x-position of the left side of this line.  For alignments
    // other than AZ_ALIGN_LEFT, this will require calculating the length of
    // the line.
    double line_left = x;
    if (align != AZ_ALIGN_LEFT) {
      int line_end = line_start;
      int line_length = 0;
      while (string[line_end] != '\0' && string[line_end] != '\n') {
        ++line_end;
        // If we see a $-escape, we need to determine its expanded length.
        if (string[line_end - 1] == '$') {
          az_key_id_t key_id = AZ_KEY_UNKNOWN;
          switch (string[line_end]) {
            case '\0': break;
            // A "$$" escape inserts a literal '$' character, so that adds 1 to
            // the length of the line.
            case '$': ++line_length; break;
            // A "$X" escape adds no length, but we need to skip over the six
            // hex digits that come after it.  However, we must take care not
            // to advance line_end past the end of the string if the escape is
            // incomplete.
            case 'X':
              ++line_end;
              for (int stop = line_end + 6; line_end < stop &&
                     string[line_end] != '\0'; ++line_end) {}
              break;
            // The key name escapes add the length of the key name to the line.
            // Note for now which key_id we need to insert the name of, and
            // we'll handle it below.
            case 'u': key_id = prefs->up_key; break;
            case 'd': key_id = prefs->down_key; break;
            case 'r': key_id = prefs->right_key; break;
            case 'l': key_id = prefs->left_key; break;
            case 'f': key_id = prefs->fire_key; break;
            case 'o': key_id = prefs->ordn_key; break;
            case 't': key_id = prefs->util_key; break;
            // All other escapes are just a single letter after the '$', which
            // we should skip over without increasing the line length.
            default:
              ++line_end;
              break;
          }
          // If the escape we just saw was for a key name, look up the name of
          // that key and add its length to the line length.  Also, increment
          // line_end to skip over the escape, since we didn't do that above
          // for the key name escapes.
          if (key_id != AZ_KEY_UNKNOWN) {
            ++line_end;
            line_length += strlen(az_key_name(key_id));
          }
        }
        // Each literal character (i.e. not a $-escape) increments the line
        // length.
        else ++line_length;
      }
      // Now that we know the length of the line, we can calculate the
      // x-position of the left side of the line.
      if (align == AZ_ALIGN_RIGHT) line_left -= line_length * height;
      else {
        assert(align == AZ_ALIGN_CENTER);
        line_left -= 0.5 * (line_length * height - 0.25 * height);
      }
    }
    // Draw the individual fragments of text making up this line, one per loop
    // iteration.  Fragments are bounded by $-escapes.
    int col = 0;
    int fragment_start = line_start;
    while (true) {
      const double fragment_left = line_left + height * col;
      // Determine where the fragment ends.  It ends at the next $-escape, at
      // the end of the line (or of the whole string), or when we reach our
      // end_row/end_col position -- whichever comes first.
      int fragment_end = fragment_start;
      while (string[fragment_end] != '\0' && string[fragment_end] != '\n' &&
             string[fragment_end] != '$') {
        if (row == end_row && col == end_col) break;
        ++col;
        ++fragment_end;
      }
      // Draw the fragment (if it's non-empty).
      if (fragment_end > fragment_start) {
        az_draw_chars(height, AZ_ALIGN_LEFT, fragment_left, line_top,
                      string + fragment_start, fragment_end - fragment_start);
      }
      // If we've reached the final row/col, or the end of the string, we're
      // completely done.
      if (row == end_row && col == end_col) return;
      if (string[fragment_end] == '\0') return;
      // Otherwise, check if this is the end of the line; if so, the next line
      // will begin at fragment_start.
      fragment_start = fragment_end + 1;
      if (string[fragment_end] == '\n') break;
      // Otherwise, we must be at a $-escape.  Interpret the escape by setting
      // the current color or inserting a key name or whatever's needed.
      assert(string[fragment_end] == '$');
      az_key_id_t key_id = AZ_KEY_UNKNOWN;
      const char escape = string[fragment_start];
      ++fragment_start;
      switch (escape) {
        // If the opening '$' of the escape was the last character in the
        // string, print a warning and stop.
        case '\0':
          AZ_WARNING_ONCE("Incomplete $-escape\n");
          return;
        // The escape "$$" means to insert a single literal '$' character.  We
        // can just back up fragment_start by one so that the second '$' is
        // included as the beginning of the next fragment.
        case '$': --fragment_start; break;
        // Handle color escapes:
        case 'W': glColor3f(1, 1, 1); break;
        case 'A': glColor3f(0.5, 0.5, 0.5); break;
        case 'R': glColor3f(1, 0, 0); break;
        case 'G': glColor3f(0, 1, 0); break;
        case 'B': glColor3f(0, 0, 1); break;
        case 'M': glColor3f(1, 0, 1); break;
        case 'Y': glColor3f(1, 1, 0); break;
        case 'C': glColor3f(0, 1, 1); break;
        case 'X':
          // First, make sure that we won't hit the end of the string trying to
          // read the next six characters after the "$X".  If we will, print a
          // warning and quit.
          for (int i = 0; i < 6; ++i) {
            if (string[fragment_start + i] == '\0') {
              AZ_WARNING_ONCE("Incomplete $X escape: $X%s\n",
                              string + fragment_start);
              return;
            }
          }
          // Parse out the RGB hex values.  If they're valid, set the current
          // color; if not, print a warning and then just ignore the whole
          // $Xrrggbb escape.
          {
            uint8_t red, green, blue;
            if (hex_parse(string[fragment_start + 0],
                          string[fragment_start + 1], &red) &&
                hex_parse(string[fragment_start + 2],
                          string[fragment_start + 3], &green) &&
                hex_parse(string[fragment_start + 4],
                          string[fragment_start + 5], &blue)) {
              glColor3ub(red, green, blue);
            } else {
              AZ_WARNING_ONCE("Malformed $X escape: $X%.6s\n",
                              string + fragment_start);
            }
            fragment_start += 6;
          }
          break;
        // For key name escapes, note for now which key_id we need to insert
        // the name of, and we'll handle it below.
        case 'u': key_id = prefs->up_key; break;
        case 'd': key_id = prefs->down_key; break;
        case 'r': key_id = prefs->right_key; break;
        case 'l': key_id = prefs->left_key; break;
        case 'f': key_id = prefs->fire_key; break;
        case 'o': key_id = prefs->ordn_key; break;
        case 't': key_id = prefs->util_key; break;
        // If we see an unknown escape (e.g. "$Q"), skip over both characters,
        // print a warning, and keep going.
        default:
          AZ_WARNING_ONCE("Invalid $-escape: $%c\n", escape);
          break;
      }
      // If the escape we just saw was for a key name, look up the name of that
      // key and print it, advancing the current col by the length of the name.
      // Of course, if our end_col falls in the middle of the key name, we'll
      // have to cut the key name short.
      if (key_id != AZ_KEY_UNKNOWN) {
        const char *key_name = az_key_name(key_id);
        int len = strlen(key_name);
        if (row == end_row && col < end_col && end_col - col < len) {
          len = end_col - col;
        }
        az_draw_chars(height, AZ_ALIGN_LEFT, line_left + height * col,
                      line_top, key_name, len);
        col += len;
      }
    }
    // Advance to the next line.
    ++row;
    line_start = fragment_start;
    line_top += spacing;
  }
}

/*===========================================================================*/
