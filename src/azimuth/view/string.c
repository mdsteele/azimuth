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

#include <string.h>

#include <OpenGL/gl.h>

/*===========================================================================*/

#define FONT_SIZE 8

static struct {
  GLenum mode;
  size_t num_points;
  az_vector_t points[8];
} char_specs[128] = {
  ['!'] = {GL_LINES, 4, {{2,0}, {2,3}, {2,5}, {2,6}}},
  ['.'] = {GL_POINTS, 1, {{2,6}}},
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
  // 8
  ['9'] = {GL_LINE_STRIP, 6, {{0,6}, {6,6}, {6,0}, {0,0}, {0,3}, {6,3}}},
  [':'] = {GL_POINTS, 2, {{2,2}, {2,5}}},
  ['A'] = {GL_LINES, 6, {{0,6}, {3,0}, {3,0}, {6,6}, {1,4}, {5,4}}},
  ['B'] = {GL_LINE_LOOP, 5, {{0,0}, {6,0}, {0,3}, {6,6}, {0,6}}},
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
  ['Z'] = {GL_LINE_STRIP, 4, {{0,0}, {6,0}, {0,6}, {6,6}}}
};

static void draw_char(int c) {
  if (c >= 0 && c < 128 && char_specs[c].num_points > 0) {
    glBegin(char_specs[c].mode);
    const az_vector_t *points = char_specs[c].points;
    for (int i = 0; i < char_specs[c].num_points; ++i) {
      glVertex2d(points[i].x, points[i].y);
    }
    glEnd();
  }
}

/*===========================================================================*/

void az_draw_string(az_vector_t topleft, double height, const char* string) {
  az_draw_chars(topleft, height, string, strlen(string));
}

void az_draw_chars(az_vector_t topleft, double height,
                   const char* chars, size_t len) {
  glPushMatrix(); {
    glTranslated(topleft.x, topleft.y, 0);
    glScaled(height / FONT_SIZE, height / FONT_SIZE, 1);
    glTranslated(0.5, 0.5, 0);
    for (size_t i = 0; i < len; ++i) {
      draw_char(chars[i]);
      glTranslated(FONT_SIZE, 0, 0);
    }
  } glPopMatrix();
}

/*===========================================================================*/
