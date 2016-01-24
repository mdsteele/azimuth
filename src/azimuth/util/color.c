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

#include "azimuth/util/color.h"

#include <assert.h>
#include <math.h>

#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

AZ_STATIC_ASSERT(sizeof(az_color_t) == sizeof(int32_t));

az_color_t az_color3f(float r, float g, float b) {
  assert(r >= 0.0f && r <= 1.0f);
  assert(g >= 0.0f && g <= 1.0f);
  assert(b >= 0.0f && b <= 1.0f);
  return (az_color_t){(uint8_t)(r * UINT8_MAX), (uint8_t)(g * UINT8_MAX),
                      (uint8_t)(b * UINT8_MAX), UINT8_MAX};
}

az_color_t az_color4f(float r, float g, float b, float a) {
  assert(r >= 0.0f && r <= 1.0f);
  assert(g >= 0.0f && g <= 1.0f);
  assert(b >= 0.0f && b <= 1.0f);
  assert(a >= 0.0f && a <= 1.0f);
  return (az_color_t){(uint8_t)(r * UINT8_MAX), (uint8_t)(g * UINT8_MAX),
                      (uint8_t)(b * UINT8_MAX), (uint8_t)(a * UINT8_MAX)};
}

const az_color_t AZ_WHITE = {255, 255, 255, 255};
const az_color_t AZ_RED = {255, 0, 0, 255};
const az_color_t AZ_GREEN = {0, 255, 0, 255};
const az_color_t AZ_BLUE = {0, 0, 255, 255};

az_color_t az_hsva_color(double hue_radians, double saturation, double value,
                         double alpha) {
  assert(isfinite(hue_radians));
  assert(0.0 <= saturation && saturation <= 1.0);
  assert(0.0 <= value && value <= 1.0);
  assert(0.0 <= alpha && alpha <= 1.0);
  // The below was adapted from http://www.cs.rit.edu/~ncs/color/t_convert.html
  const double hh = AZ_RAD2DEG(az_mod2pi_nonneg(hue_radians)) / 60.0;
  const int ii = (int)hh;
  const double ff = hh - ii;
  const uint8_t vv = (uint8_t)(value * 255);
  const uint8_t pp = (uint8_t)(value * (1.0 - saturation) * 255);
  const uint8_t qq = (uint8_t)(value * (1.0 - saturation * ff) * 255);
  const uint8_t tt = (uint8_t)(value * (1.0 - saturation * (1.0 - ff)) * 255);
  const uint8_t aa = (uint8_t)(255 * alpha);
  switch (ii) {
    case 0: return (az_color_t){vv, tt, pp, aa};
    case 1: return (az_color_t){qq, vv, pp, aa};
    case 2: return (az_color_t){pp, vv, tt, aa};
    case 3: return (az_color_t){pp, qq, vv, aa};
    case 4: return (az_color_t){tt, pp, vv, aa};
    case 5: return (az_color_t){vv, pp, qq, aa};
    default: AZ_ASSERT_UNREACHABLE();
  }
}

az_color_t az_transition_color(az_color_t color0, az_color_t color1,
                               double param) {
  assert(0.0 <= param && param <= 1.0);
  return (az_color_t){
    (uint8_t)(color0.r + param * ((int)color1.r - (int)color0.r)),
    (uint8_t)(color0.g + param * ((int)color1.g - (int)color0.g)),
    (uint8_t)(color0.b + param * ((int)color1.b - (int)color0.b)),
    (uint8_t)(color0.a + param * ((int)color1.a - (int)color0.a))
  };
}

/*===========================================================================*/
