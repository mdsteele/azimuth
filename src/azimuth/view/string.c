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

#include <stdarg.h>
#include <stdio.h>

#include <GL/gl.h>

#include <azimuth/util/misc.h> // for AZ_ARRAY_SIZE

/*===========================================================================*/

#define FONT_SIZE 8

static struct {
  GLenum mode;
  size_t num_points;
  az_vector_t points[8];
} char_specs[] = {
  ['!'] = {GL_LINES, 4, {{2,0}, {2,3}, {2,5}, {2,6}}},
  ['"'] = {GL_LINES, 4, {{1,0}, {1,3}, {4,0}, {4,3}}},
  ['#'] = {GL_LINES, 8, {{2,0}, {2,6}, {4,0}, {4,6},
                         {1,2}, {5,2}, {1,4}, {5,4}}},
  ['$'] = {GL_LINE_STRIP, 8, {{6,1}, {0,1}, {0,3}, {6,3}, {6,5}, {0,5},
                              {1,6}, {5,0}}},
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
    glBegin(char_specs[c].mode);
    const az_vector_t *points = char_specs[c].points;
    for (int i = 0; i < char_specs[c].num_points; ++i) {
      glVertex2d(points[i].x, points[i].y);
    }
    glEnd();
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
    case AZ_ALIGN_CENTER: left -= 0.5 * len * height; break;
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
