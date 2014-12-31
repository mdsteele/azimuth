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
#ifndef AZIMUTH_UTIL_COLOR_H_
#define AZIMUTH_UTIL_COLOR_H_

#include <stdint.h>

/*===========================================================================*/

// Represents a color, with red, green, blue, and alpha components.
typedef struct {
  uint8_t r, g, b, a;
} az_color_t;

// Color constants:
extern const az_color_t AZ_WHITE;
extern const az_color_t AZ_RED;
extern const az_color_t AZ_GREEN;
extern const az_color_t AZ_BLUE;

// Convert HSVA values to an az_color_t value.  The hue_radians argument can be
// any angle, measured in radians; the other three arguments must be in the
// range [0, 1].
az_color_t az_hsva_color(double hue_radians, double saturation, double value,
                         double alpha);

// Return a color that transitions smoothly from color0 to color1 as param
// transitions from 0 to 1.  The param argument must be in the range [0, 1].
az_color_t az_transition_color(az_color_t color0, az_color_t color1,
                               double param);

/*===========================================================================*/

#endif // AZIMUTH_UTIL_COLOR_H_
