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
#ifndef AZIMUTH_VIEW_UTIL_H_
#define AZIMUTH_VIEW_UTIL_H_

#include "azimuth/util/color.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

// Set the current GL color.
void az_gl_color(az_color_t color);

// Rotate the GL camera by the given angle in radians.
void az_gl_rotated(double radians);

// Translate the GL camera by the given vector.
void az_gl_translated(az_vector_t v);

// Place a GL vertex at the given position.
void az_gl_vertex(az_vector_t v);

void az_draw_cracks(az_random_seed_t *seed, az_vector_t origin, double angle,
                    double length);

/*===========================================================================*/

#endif // AZIMUTH_VIEW_UTIL_H_
