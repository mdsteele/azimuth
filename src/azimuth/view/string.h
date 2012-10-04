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

#include <stdlib.h> // for size_t

#include "azimuth/vector.h"

/*===========================================================================*/

// Draw a (null-terminated) string.  You must set the current color before
// calling this.
void az_draw_string(az_vector_t topleft, double height, const char* string);

// Draw a sequence of characters (with an explicit length).  You must set the
// current color before calling this.
void az_draw_chars(az_vector_t topleft, double height,
                   const char* chars, size_t len);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_STRING_H_
