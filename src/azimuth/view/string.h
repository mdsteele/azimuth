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

#pragma once
#ifndef AZIMUTH_VIEW_STRING_H_
#define AZIMUTH_VIEW_STRING_H_

#include <stddef.h> // for size_t

#include "azimuth/util/prefs.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

typedef enum {
  AZ_ALIGN_LEFT,
  AZ_ALIGN_CENTER,
  AZ_ALIGN_RIGHT
} az_alignment_t;

// Draw a (null-terminated) string.  You must set the current color before
// calling this.
void az_draw_string(double height, az_alignment_t align, double x, double top,
                    const char *string);

// Draw a sequence of characters (with an explicit length).  You must set the
// current color before calling this.
void az_draw_chars(double height, az_alignment_t align, double x, double top,
                   const char *chars, size_t len);

// Draw a string based on a printf format string and args.  You must set the
// current color before calling this.
void az_draw_printf(double height, az_alignment_t align, double x, double top,
                    const char *format, ...)
  __attribute__((__format__(__printf__,5,6)));

// Draw a paragraph of text, and interpret special $-escapes to set colors and
// insert keyboard key names.  The initial color is white.  Stops just before
// the end_col-th character on the end_row-th line; set these to -1 to print
// the whole paragraph.  The permitted escapes are:
//   $$ - insert a literal '$' character
//   $W - set color to white
//   $A - set color to gray
//   $R - set color to red
//   $G - set color to green
//   $B - set color to blue
//   $M - set color to magenta
//   $Y - set color to yellow
//   $C - set color to cyan
//   $Xrrggbb - set color to {rr, gg, bb}, where rrggbb are hex digits
//   $u - insert name of prefs->up_key
//   $d - insert name of prefs->down_key
//   $r - insert name of prefs->right_key
//   $l - insert name of prefs->left_key
//   $f - insert name of prefs->fire_key
//   $o - insert name of prefs->ordn_key
//   $t - insert name of prefs->util_key
void az_draw_paragraph(
    double height, az_alignment_t align, double x, double top, double spacing,
    int end_row, int end_col, const az_preferences_t *prefs,
    const char *string);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_STRING_H_
